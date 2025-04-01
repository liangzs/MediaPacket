//
// Created by DELLQ on 16/7/2024.
//

#include "video_concat.h"


VideoConcat::VideoConcat(std::vector<char *> inputPaths, char *outputPath, int width, int height) {
    this->outWidth = width;
    this->outHeight = height;
    this->outPath = static_cast<char *>(malloc(strlen(outputPath) + 1));
    strcpy(this->outPath, outputPath);
    this->inputPaths = inputPaths;

    //音频参数初始化
    avSampleFormat = AVSampleFormat::AV_SAMPLE_FMT_S16;
    sampleRate = 44100;
    channelLayout = AV_CH_LAYOUT_STEREO;
    channels = av_get_channel_layout_nb_channels(channelLayout);

    //时间戳初始化
    frameRate = 25;
    videoFrameCount = 0;
    audioFrameCount = 0;
    //转成微秒
    videoTimebase = AV_TIME_BASE * av_q2d({1, frameRate});
    audioTimebase = AV_TIME_BASE * av_q2d({1, sampleRate});

    //pcm缓冲区
    pcmOutBuffer = static_cast<uint8_t *>(av_malloc(sampleRate * 2 * channels));
    readEnd = false;

}

VideoConcat::~VideoConcat() {
    free(this->outPath);
    for (int i = 0; i < this->inputPaths.size(); i++) {
        free(this->inputPaths[i]);
    }
    inputPaths.clear();
}

int VideoConcat::startConcat() {
    this->start();//执行run，再run中进行解码
    if (initOuput() < 0) {
        LOGE("initOuput error");
        return -1;
    }
    for (int i = 0; i < inputPaths.size(); ++i) {
        char *inputPath = inputPaths[i];
        //先释放，再创建
        releaseInput();
        initInput(inputPath);
        //开始解码
        startDecode();

        //等待所以队列清理完
        while (queueAudio.size() > 0 || queueVideo.size() > 0) {
            av_usleep(2000);
        }
        av_audio_fifo_reset(audioFifo);
        //清空所有队列
        clearQueue();
    }
    readEnd = true;

    return 1;
}

/**
 * 要初始化sws和swr
 * @param inputPath
 * @return
 */
int VideoConcat::initInput(char *inputPath) {
    int ret = avformat_open_input(&inFormatContext, inputPath, NULL, NULL);
    if (ret < 0) {
        LOGE("avformat_open_input error");
        return -1;
    }
    ret = avformat_find_stream_info(inFormatContext, NULL);
    if (ret < 0) {
        LOGE("avformat_find_stream_info error");
        return -1;
    }
    //查询信道
    for (int i = 0; i < inFormatContext->nb_streams; ++i) {
        if (inFormatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            inVideoStreamIndex = i;
            //宽高等其他信息赋值
            inWidth = inFormatContext->streams[i]->codecpar->width;
            inHeight = inFormatContext->streams[i]->codecpar->height;
            inVideoCodec = avcodec_find_decoder(inFormatContext->streams[i]->codecpar->codec_id);
        } else if (inFormatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            inAudioStreamIndex = i;
            inAudioCodec = avcodec_find_decoder(inFormatContext->streams[i]->codecpar->codec_id);

        }
    }
    //video
    inVideoCodecContext = avcodec_alloc_context3(inVideoCodec);
    ret = avcodec_parameters_to_context(inVideoCodecContext,
                                        inFormatContext->streams[inVideoStreamIndex]->codecpar);
    if (ret < 0) {
        LOGE("avcodec_parameters_to_context error");
        return -1;
    }
    //打开解码器
    ret = avcodec_open2(inVideoCodecContext, inVideoCodec, NULL);
    if (ret < 0) {
        LOGE("avcodec_open2 error");
        return -1;
    }
    //video sws
    releaseSwsContext();
    if (initSwsContext() < 0) {
        LOGE("initSwsContext error");
        return -1;
    }

    //audio
    inAudioCodecContext = avcodec_alloc_context3(inAudioCodec);
    if (inAudioCodecContext == NULL) {
        LOGE("avcodec_alloc_context3 error");
        return -1;
    }
    ret = avcodec_parameters_to_context(inAudioCodecContext,
                                        inFormatContext->streams[inAudioStreamIndex]->codecpar);
    if (ret < 0) {
        LOGE("inAudioCodecContext->avcodec_parameters_to_context error");
        return -1;
    }
    ret = avcodec_open2(inAudioCodecContext, inAudioCodec, NULL);
    if (ret < 0) {
        LOGE("inAudioCodecContext->avcodec_open2 error");
        return -1;
    }
    //audio swr
    releaseSwrContext();
    if (initSwrContext() < 0) {
        LOGE("initSwrContext error");
        return -1;
    }
    return 0;
}

/**
 * 这里再decode里执行，循环inputPath,每个文件都需要进行检测
 * @return
 */
int VideoConcat::initOuput() {
    //检测文件，写入头文件
    int ret = avformat_alloc_output_context2(&outFormatContext, NULL, NULL, outPath);
    if (ret < 0) {
        LOGE("avformat_alloc_output_context2 error");
        return -1;
    }
    //adVideoStream,addAudioStream
    if (addVideoStream() < 0) {
        LOGE("addVideoStream error");
        return -1;
    }
    if (addAudioStream() < 0) {
        LOGE("addAudioStream error");
        return -1;
    }
//检测文件写入是否成功
    if (!(oformat->flags & AVFMT_NOFILE)) {
        ret = avio_open(&outFormatContext->pb, outPath, AVIO_FLAG_WRITE);
        if (ret < 0) {
            LOGE("avio_open error");
            return -1;
        }
    }
//写入头文件
    ret = avformat_write_header(outFormatContext, NULL);
    if (ret < 0) {
        LOGE("avformat_write_header error");
        return -1;
    }

    //创建outVideoFrame对象
    outVideoFrame = av_frame_alloc();
    outVideoFrame->width = outWidth;
    outVideoFrame->height = outHeight;
    outVideoFrame->format = AV_PIX_FMT_YUV420P;

    //分配av_get_buffer和检测是否可写write
    ret = av_frame_get_buffer(outVideoFrame, 32);//对齐用0也可以,有些硬件加速需要指定32等
    if (ret < 0) {
        LOGE("outVideoFrame av_frame_get_buffer error");
        return -1;
    }
    //也可以在设置avframe可写的时候进行判断av_frame_is_writable 如果不可写在执行下面这句
    ret = av_frame_make_writable(outVideoFrame);
    if (ret < 0) {
        LOGE("outVideoFrame av_frame_make_writable error");
        return -1;
    }

    return 0;
}

/**
 * 遍历queue，判读时间戳，把时间比较早的先写入数据
 */
void VideoConcat::run() {
    int result = 0;
    while (!isExist) {
        if (isPause) {
            av_usleep(1000);
            continue;
        }
        //取数据的时候用锁进行防
        if (queueAudio.empty() || queueVideo.empty()) {
            if (readEnd) {
                break;
            }
            continue;
        }
        pthread_mutex_lock(&mutex);
        AVPacket *videoPack = queueVideo.front();
        AVPacket *audioPack = queueAudio.front();
        pthread_mutex_unlock(&mutex);
        //比较时间戳
        if (av_compare_ts(audioPts, outAudioStream->time_base, videoPts,
                          outVideoStream->time_base) < 0) {
            //音频先
            audioPts = audioPack->pts;
            result = av_interleaved_write_frame(outFormatContext, audioPack);
            if (result < 0) {
                LOGE("audio av_interleaved_write_frame error");
            }
            av_packet_free(&audioPack);
            queueAudio.pop();
        } else {
            //视频先
            videoPts = videoPack->pts;
            result = av_interleaved_write_frame(outFormatContext, videoPack);
            if (result < 0) {
                LOGE("video av_interleaved_write_frame error");
            }
            av_packet_free(&videoPack);
            queueVideo.pop();
        }
        //写入尾巴
        av_write_trailer(outFormatContext);
        LOGE("CONCAT-------------FINISH------------")
    }


}

int VideoConcat::addVideoStream() {
    outVideoStream = avformat_new_stream(outFormatContext, NULL);
    if (outVideoStream == NULL) {
        LOGE("avformat_new_stream error");
        return -1;
    }
    oformat = outFormatContext->oformat;
    if (oformat->video_codec == AV_CODEC_ID_NONE) {
        LOGE("oformat->video_codec==AV_CODEC_ID_NONE");
        return -1;
    }
    outVideoCodec = avcodec_find_encoder(oformat->video_codec);
    if (outVideoCodec == NULL) {
        LOGE("avcodec_find_encoder error");
        return -1;
    }
    outVideoCodecContext = avcodec_alloc_context3(outVideoCodec);
    if (outVideoCodecContext == NULL) {
        LOGE("avcodec_alloc_context3 error");
        return -1;
    }
    outVideoCodecContext->width = outWidth;
    outVideoCodecContext->height = outHeight;
    outVideoCodecContext->framerate = AVRational{frameRate, 1};
    outVideoCodecContext->time_base = AVRational{1, frameRate};
    outVideoCodecContext->pix_fmt = AV_PIX_FMT_YUV420P;
    if (avcodec_parameters_from_context(outVideoStream->codecpar, outVideoCodecContext) < 0) {
        LOGE("addVideoStream->avcodec_parameters_from_context error");
        return -1;
    }
    //打开编码
    if (avcodec_open2(outVideoCodecContext, outVideoCodec, NULL) < 0) {
        LOGE("addVideoStream->avcodec_open2 error");
        return -1;
    }
    return 0;
}

int VideoConcat::addAudioStream() {
    outAudioStream = avformat_new_stream(outFormatContext, NULL);
    if (outAudioStream == NULL) {
        LOGE("avformat_new_stream error");
        return -1;
    }
    if (oformat->audio_codec == AV_CODEC_ID_NONE) {
        LOGE("oforamt->audio_codec==AV_CODEC_ID_NONE");
        return -1;
    }
    outAudioCodec = avcodec_find_decoder(oformat->audio_codec);
    if (outAudioCodec == NULL) {
        LOGE("avcodec_find_decoder error");
        return -1;
    }
    outAudioCodecContext = avcodec_alloc_context3(outAudioCodec);
    if (outAudioCodecContext == NULL) {
        LOGE("avcodec_alloc_context3 error");
        return -1;
    }
    outAudioCodecContext->sample_fmt = avSampleFormat;
    outAudioCodecContext->sample_rate = sampleRate;
    outAudioCodecContext->time_base = AVRational{1, sampleRate};
    outAudioCodecContext->channel_layout = channelLayout;
    //av_get_channel_layout_nb_channels(AV_CH_LAYOUT_STEREO)其实就是2
    outAudioCodecContext->channels = channels;
    outAudioCodecContext->codec_type = AVMEDIA_TYPE_AUDIO;
    if (avcodec_parameters_from_context(outAudioStream->codecpar, outAudioCodecContext) < 0) {
        LOGE("addAudioStream->avcodec_parameters_from_context error");
        return -1;
    }
    if (avcodec_open2(outAudioCodecContext, outAudioCodec, NULL) < 0) {
        LOGE("addAudioStream->avcodec_open2 error");
        return -1;
    }



    return 0;
}

void VideoConcat::releaseInput() {
    if (inFormatContext != NULL) {
        avformat_free_context(inFormatContext);
        inFormatContext = NULL;
    }
    if (inVideoCodecContext != NULL) {
        avcodec_free_context(&inVideoCodecContext);
        inVideoCodecContext = NULL;
    }
    if (inAudioCodecContext != NULL) {
        avcodec_free_context(&inAudioCodecContext);
    }

}

/**
 * video sws,如果是要再android屏幕显示，则需要重现转成rgb格式，然后用新的buffer取承载数据
 * eg: uint8_t *outBuffer = static_cast<uint8_t *>(av_malloc(sizeof(uint8_t) * numbytes));
 * av_image_fill_arrays(rgbFrame->data, rgbFrame->linesize, outBuffer,
 *                           AVPixelFormat::AV_PIX_FMT_RGBA, avCodecContext->width,
 *                            avCodecContext->height, 1);
 * @return
 */
int VideoConcat::initSwrContext() {

    inSwsContext = sws_getContext(inWidth, inHeight, inVideoCodecContext->pix_fmt, outWidth,
                                  outHeight, AV_PIX_FMT_YUV420P, SWS_FAST_BILINEAR, NULL, NULL,
                                  NULL);
    if (inSwsContext == NULL) {
        LOGE("sws_getContext NULL");
        return -1;
    }
    return 0;
}

/**
 * audio的swr过程为alloc，set_option init ,最后使用convert
 * @return
 */
int VideoConcat::initSwsContext() {
    inSwrContext = swr_alloc();
    if (inSwrContext == NULL) {
        LOGE("swr_alloc error");
        return -1;
    }
    AVCodecParameters *parameters = inFormatContext->streams[inAudioStreamIndex]->codecpar;
    inSwrContext = swr_alloc_set_opts(inSwrContext,
                                      av_get_default_channel_layout(channels), avSampleFormat,
                                      sampleRate,
                                      av_get_default_channel_layout(parameters->channels),
                                      static_cast<AVSampleFormat>(parameters->format),
                                      parameters->sample_rate, 0,
                                      0);
    if (inSwrContext == NULL) {
        LOGE("swr_alloc_set_opts error");
        return -1;
    }
    if (swr_init(inSwrContext) < 0) {
        LOGE("swr_init error");
        return -1;
    }
    return 0;
}

void VideoConcat::releaseSwsContext() {
    if (inSwsContext != NULL) {
        sws_freeContext(inSwsContext);
        inSwsContext = NULL;
    }
}

void VideoConcat::releaseSwrContext() {
    swr_free(&inSwrContext);
    inSwrContext = NULL;
}

/**
 * 这里需要重新解释一下对outVideoFrame的缓冲区的用法，如果是要自己创建缓冲，需要调用av_frame_get_buffer创建缓冲区
 * 如果有现有的缓冲区，可以使用av_image_fill_arrays直接使用缓冲区，不用创建avFrame的缓冲区了
 * 内存分配：
    av_frame_get_buffer 会为 AVFrame 分配内存缓冲区。
    av_image_fill_arrays 不分配内存，而是使用用户提供的缓冲区。
    使用场景：
    av_frame_get_buffer 适用于需要为 AVFrame 分配内存的情况。
    av_image_fill_arrays 适用于已经有预先分配的缓冲区，并希望将其设置到 AVFrame 的情况。
    功能侧重点：
    av_frame_get_buffer 侧重于内存分配和初始化。
    av_image_fill_arrays 侧重于填充数据指针和行跨度。
 * @return
 */
int VideoConcat::startDecode() {
    int ret;
    while (!isExist) {
        if (queueAudio.size() > QUEUE_MAX_SIZE || queueVideo.size() > QUEUE_MAX_SIZE) {
            av_usleep(1000);
            continue;
        }
        ret = av_read_frame(inFormatContext, inPacket);
        if (ret < 0) {
            if (ret == AVERROR_EOF) {
                isExist = true;
                break;
            }
        }
        //分开视频和音频进行编解码
        if (inPacket->stream_index == inVideoStreamIndex) {
            AVFrame *frame = decodePacket(inVideoCodecContext);
            if (frame == NULL) {
                continue;
            }
            //根据原始的avFrame进行编码成packet
            sws_scale(inSwsContext, frame->data, frame->linesize, 0, frame->height,
                      outVideoFrame->data, outVideoFrame->linesize);
            //计算时间戳
            outVideoFrame->pts = videoTimebase * videoFrameCount;
            videoFrameCount++;
            av_frame_free(&frame);
            //编码
            AVPacket *newPacket = encodePacket(outVideoCodecContext, outVideoFrame);
            if (newPacket == NULL) {
                continue;
            }
            //把生成好的packet放到队列中
            //时间戳进行转化
            av_packet_rescale_ts(newPacket, inVideoCodecContext->time_base,
                                 outVideoCodecContext->time_base);
            queueVideo.push(newPacket);
            av_frame_free(&outVideoFrame);

        } else if (inPacket->stream_index == inAudioStreamIndex) {
            AVFrame *frame = decodePacket(inAudioCodecContext);
            if (frame == NULL) {
                continue;
            }
            int size = swr_convert(inSwrContext, &pcmOutBuffer, sampleRate * 2 * channels,
                                   reinterpret_cast<const uint8_t **>(&frame->data),
                                   frame->nb_samples);

            //处理音频完整帧的时候这里有两种做法，一种是开辟出足够的空间，然后存储满数据:sampleRate*channels*位数，这时候就是不需要fifo队列的
            //如果不采用完整帧的话，比如使用1024这个固定小的长度获取音频，那么就会出现取不了完整的数据，这就需要audiofifo进行缓存数据了
            //方案一
//            int dataSize = size * channels * av_get_bytes_per_sample(avSampleFormat);
//            //根据pcmoutbuffer和大小，重新创建avFrame以及avFrame的data
//            AVFrame *newFrame = createAudioFrame(dataSize);
//            av_frame_free(&frame);
//            audioFrameCount += inAudioCodecContext->frame_size;
//            AVPacket *newPacket = encodePacket(inAudioCodecContext, newFrame);
//            //转化packet中的pts
//            av_packet_rescale_ts(newPacket, inAudioCodecContext->time_base,
//                                 outAudioCodecContext->time_base);
//
//            queueAudio.push(newPacket);
//            av_frame_free(&newFrame);
            //针对音频，引入avAudioFIFo 队列，主要作用是解析音频时候，在实时处理音频时，可能会出现不连续，或者不完整的帧
            //这就需要audiofifo来进行缓存了，然后取出整个完整帧进行处理
            //方案二 用固定长frame_size做计算
            uint8_t *audioFifoBuffer = static_cast<uint8_t *>(av_malloc(
                    outAudioCodecContext->frame_size *
                    av_get_bytes_per_sample(avSampleFormat) * channels));
            //nb_samples 采用数，size=nb_samples*channels*sampleFormat
            memcpy(audioFifoBuffer, pcmOutBuffer,
                   size * av_get_bytes_per_sample(avSampleFormat) * channels);
            av_audio_fifo_write(audioFifo, reinterpret_cast<void **>(&audioFifoBuffer), size);
            //检测长度是否足够
            size = av_audio_fifo_size(audioFifo);
            if (size < inAudioCodecContext->frame_size) {
                continue;
            }
            //从fifo中获取,这里取足inAudioCodecContext->frame_size
            size = av_audio_fifo_read(audioFifo, reinterpret_cast<void **>(&audioFifoBuffer),
                                      inAudioCodecContext->frame_size);
            //size现在size=nb_samples
            int datasize = size * channels * av_get_bytes_per_sample(avSampleFormat);
            //根据pcmoutbuffer和大小，重新创建avFrame以及avFrame的data
            AVFrame *newFrame = createAudioFrame(datasize);
            audioFrameCount += inAudioCodecContext->frame_size;
            AVPacket *newPacket = encodePacket(inAudioCodecContext, newFrame);
            //转化packet中的pts
            av_packet_rescale_ts(newPacket, inAudioCodecContext->time_base,
                                 outAudioCodecContext->time_base);
            queueAudio.push(newPacket);
            av_frame_free(&newFrame);
        }
    }
    av_audio_fifo_free(audioFifo);
    av_packet_free(&inPacket);
    return 0;
}

AVFrame *VideoConcat::decodePacket(AVCodecContext *avCodecContext) {
    int ret = avcodec_send_packet(avCodecContext, inPacket);
    if (ret < 0) {
        av_packet_free(&inPacket);
        return NULL;
    }
    AVFrame *frame = av_frame_alloc();
    ret = avcodec_receive_frame(avCodecContext, frame);
    if (ret < 0) {
        av_frame_free(&frame);
        av_packet_free(&inPacket);
        LOGE("avcodec_receive_frame error: %s,avcontext: %s", av_err2str(ret),
             avCodecContext->codec->name);
        return NULL;
    }
    return frame;
}

AVPacket *VideoConcat::encodePacket(AVCodecContext *avCodecContext, AVFrame *avFrame) {
    int ret = avcodec_send_frame(avCodecContext, avFrame);
    if (ret < 0) {
        LOGE("avcodec_send_packet fail");
        return NULL;
    }
    AVPacket *newPacket = av_packet_alloc();
    ret = avcodec_receive_packet(avCodecContext, newPacket);
    if (ret < 0) {
        av_packet_free(&newPacket);
        LOGE("avcodec_receive_packet fail");
        return NULL;
    }
    return newPacket;
}

AVFrame *VideoConcat::createAudioFrame(int size) {
    AVFrame *frame = av_frame_alloc();
    //取帧的内容数据
    frame->nb_samples = outAudioCodecContext->frame_size;
    frame->channels = channels;
    frame->channel_layout = channelLayout;
    frame->sample_rate = sampleRate;
    //data赋值和pts时间
    memcpy(frame->data[0], pcmOutBuffer, size);
    frame->pts = audioFrameCount * audioTimebase;
    return frame;
}

void VideoConcat::clearQueue() {
    pthread_mutex_lock(&mutex);
    while (!queueAudio.empty()) {
        AVPacket *packet = queueAudio.front();
        av_packet_free(&packet);
        queueAudio.pop();
    }
    while (!queueVideo.empty()) {
        AVPacket *packet = queueVideo.front();
        av_packet_free(&packet);
        queueVideo.pop();
    }
    pthread_mutex_unlock(&mutex);
}
