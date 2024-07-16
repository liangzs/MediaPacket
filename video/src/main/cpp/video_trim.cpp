//
// Created by DELLQ on 11/7/2024.
//

#include "video_trim.h"

/**
 * 重新赋值吧，inputPath在外部可能会回收
 * @param inputPath
 * @param outputpath
 * @param startTime
 * @param endTime
 */
VideoTrim::VideoTrim(char *inputPath, char *outputpath, long startTime, long endTime) {
    int charlen = strlen(inputPath);
    this->inputPath = static_cast<char *>(malloc(charlen + 1));
    strcpy(this->inputPath, inputPath);
    charlen = strlen(outputpath);
    this->outputPath = static_cast<char *>(malloc(charlen + 1));
    strcpy(this->outputPath, outputpath);
    this->startTime = startTime;
    this->endTime = endTime;

}

/**
 *
 */
VideoTrim::~VideoTrim() {
    avformat_free_context(inputFormatContext);
    avformat_free_context(outputFormatContext);
    avcodec_free_context(&avCtxD);
    avcodec_free_context(&avCtxE);
    av_free(avCodecD);
    av_free(avCodecE);
    free(inputPath);
    free(outputPath);
}

/**
 * c方法，执行thread方法
 * @param data
 * @return
 */
void *threadTrim(void *data) {
    VideoTrim *trim = static_cast<VideoTrim *>(data);
    pthread_exit(&trim->trimThread);
}

void VideoTrim::startTrim() {

    pthread_create(&trimThread, NULL, threadTrim, this);
}

void VideoTrim::trimImpl() {
    avcodec_register_all();
    if (initInput() < 0) {
        return;
    }
    if (initOutput() < 0) {
        return;
    }
    //开始写入文件
    if (av_seek_frame(outputFormatContext, -1, startTime / av_q2d(videoStream->time_base),
                      AVSEEK_FLAG_ANY) < 0) {
        LOGE("av_seek_frame fail");
        return;
    }
    decodeEncode();
    //写入尾部tail
    av_write_trailer(outputFormatContext);
    progress = 100;
}

int VideoTrim::initInput() {
    inputFormatContext = avformat_alloc_context();
    int ret = avformat_open_input(&inputFormatContext, inputPath, NULL, NULL);
    if (ret < 0) {
        LOGE("avformat_open_input fail");
        return -1;
    }
    ret = avformat_find_stream_info(inputFormatContext, NULL);
    if (ret < 0) {
        LOGE("avformat_find_stream_info fail");
        return -1;
    }

    for (int i = 0; i < inputFormatContext->nb_streams; i++) {
        if (inputFormatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            videoStreamIndex = i;
            avCodecD = avcodec_find_decoder(inputFormatContext->streams[i]->codecpar->codec_id);
            width = inputFormatContext->streams[i]->codecpar->width;
            height = inputFormatContext->streams[i]->codecpar->height;
            videoStream = inputFormatContext->streams[i];
            break;
        } else if (inputFormatContext->streams[i]->codecpar->codec_type ==
                   AVMediaType::AVMEDIA_TYPE_AUDIO) {
            audioStreamIndex = i;
            audioStream = inputFormatContext->streams[i];
            break;
        }
    }
    if (videoStreamIndex == -1) {
        LOGE("could not find videoStreamIndex");
        return -1;
    }
    //已经不推荐这种做法了,而是由contexparamter进行赋值
//    avCtxD = inputFormatContext->streams[videoStreamIndex]->codec;
    avCtxD = avcodec_alloc_context3(avCodecD);
    ret = avcodec_parameters_to_context(avCtxD,
                                        inputFormatContext->streams[videoStreamIndex]->codecpar);
    if (ret < 0) {
        LOGE("avcodec_parameters_to_context fail");
        return -1;
    }
    ret = avcodec_open2(avCtxD, avCodecD, NULL);
    if (ret < 0) {
        LOGE("avcodec_open2 fail");
        return -1;
    }
    return 0;
}

/**
 * 输出,codec->format->codecParmeterc反向赋值
 * @return
 */
int VideoTrim::initOutput() {
    //oformat: 指定输出格式。如果为 NULL，则函数会根据 format_name 或 filename 自动推断。
    //format_name: 指定输出格式的名称（如 "mp4", "flv" 等）。如果为 NULL，则函数会根据 filename 自动推断。
    int ret = avformat_alloc_output_context2(&outputFormatContext, NULL, NULL, outputPath);
    if (ret < 0) {
        LOGE("avformat_alloc_output_context2 fail: %s", outputPath);
        return -1;
    }
    //有oformat和iformat
    oformat = outputFormatContext->oformat;
    //添加信道音频和视频信道
    ret = addVideoStream(width, height);
    if (ret < 0) {
        LOGE("addVideoStream fail");
        return -1;
    }
    ret = addAudioStream();
    if (ret < 0) {
        LOGE("addAudioStream fail");
        return -1;
    }
    //检测打开输出文件，然后检测输入头文件，然后输入文件，最后输入尾文件
    if (!(oformat->flags & AVFMT_NOFILE)) {
        ret = avio_open(&outputFormatContext->pb, outputPath, AVIO_FLAG_WRITE);
        if (ret < 0) {
            LOGE("avio_open fail");
            return -1;
        }
    }
    //写入文件头
    ret = avformat_write_header(outputFormatContext, NULL);
    if (ret < 0) {
        LOGE("avformat_write_header fail");
        return -1;
    }
    LOGE(" init output success");
    return 1;

}

int VideoTrim::addVideoStream(int width, int height) {
    //avformat_alloc_output_context2这里初始化outputFormatContext
    outputVideoStream = avformat_new_stream(outputFormatContext, NULL);
    if (outputVideoStream == NULL) {
        LOGE("addVideoStream:avformat_new_stream fail");
        return -1;
    }
    //检测输出文件编码是否none
    if (oformat->video_codec == AV_CODEC_ID_NONE) {
        LOGE("avOutputFormat->video_codec == AV_CODEC_ID_NONE");
        return -1;
    }
    avCodecE = avcodec_find_decoder(oformat->video_codec);
    if (avCodecE == NULL) {
        LOGE("avCodecE->avcodec_find_decoder fail");
        return -1;
    }
    avCtxE = avcodec_alloc_context3(avCodecE);
    if (avCtxE == NULL) {
        LOGE("avCtxE->avcodec_alloc_context3 fail");
        return -1;
    }
    avCtxE->width = width;
    avCtxE->height = height;
    avCtxE->codec_type = AVMediaType::AVMEDIA_TYPE_VIDEO;
    //帧率以及帧率对应的时间戳
    avCtxE->time_base = AVRational{1, 25};
    avCtxE->framerate = AVRational{25, 1};
    avCtxE->pix_fmt = AV_PIX_FMT_YUV420P;
    avCtxE->bit_rate = 400000;
    avCtxE->gop_size = 10;
    avCtxE->max_b_frames = 3;
    //和avcodec_parameters_to_context相反
    if (avcodec_parameters_from_context(outputVideoStream->codecpar, avCtxE) < 0) {
        LOGE("outputVideoStream  avcodec_parameters_from_context fail");
        return -1;
    }

    //打开编码器
    if (avcodec_open2(avCtxE, avCodecE, NULL) < 0) {
        LOGE("avCodecE avcodec_open2 fail");
        return -1;
    }
    return 0;
}

/**
 * 直接复用输入源的数据
 * @return
 */
int VideoTrim::addAudioStream() {
    outputAudioStream = avformat_new_stream(outputFormatContext, NULL);
    if (outputAudioStream == NULL) {
        LOGE("outputAudioStream avformat_new_stream fail");
        return -1;
    }
    if (oformat->audio_codec == AV_CODEC_ID_NONE) {
        LOGE("avOutputFormat->audio_codec==AV_CODEC_ID_NONE");
        return -1;
    }
    //直接复制
    if (avcodec_parameters_copy(outputAudioStream->codecpar, audioStream->codecpar) < 0) {
        LOGE("avOutputFormat avcodec_parameters_copy fail");
        return -1;
    }
    //stream->codecpar->codec_tag = 0; // 设置 codec_tag 为 0，让 FFmpeg 自动选择
    outputAudioStream->codecpar->codec_tag = 0;
    return 0;
}


int VideoTrim::decodeEncode() {
    int ret;
    while (true) {
        ret = av_read_frame(inputFormatContext, avPacket);
        if (ret < 0) {
            LOGE("av_read_frame fail");
            av_packet_free(&avPacket);
            break;
        }
        int pts;
        //判断是否视频帧还是音频帧
        if (avPacket->stream_index == videoStreamIndex) {
            //先解码，再编码，再输出
            AVFrame *frame = decodePackage();
            if (frame == NULL) {
                continue;
            }
            //编码
            AVPacket *newPacket = encodePackage(frame);
            if (newPacket == NULL) {
                continue;
            }
            //pts转化成s
            pts = frame->pts * av_q2d(videoStream->time_base);
            if (pts > startTime && pts < endTime) {
                progress = 100 * (pts - startTime) / (endTime - startTime);
                //输出
                writePacket(newPacket);
            }
            av_frame_free(&frame);
            av_packet_free(&newPacket);
            av_packet_free(&avPacket);

        } else if (avPacket->stream_index == audioStreamIndex) {
            pts = avPacket->pts * av_q2d(videoStream->time_base);
            if (pts > startTime && pts < endTime) {
                //直接用原始数据进行输出即可
                writePacket(avPacket);
            }
            av_packet_free(&avPacket);
        }
        if (pts > endTime) {
            LOGE("pts > endTime  end");
            break;
        }
    }
}

AVFrame *VideoTrim::decodePackage() {
    int ret = avcodec_send_packet(avCtxD, avPacket);
    if (ret < 0) {
        LOGE("avcodec_send_packet fail");
        return NULL;
    }
    AVFrame *avFrame;
    ret = avcodec_receive_frame(avCtxD, avFrame);
    if (ret < 0) {
        av_frame_free(&avFrame);
        LOGE("avcodec_receive_frame fail");
        return NULL;
    }
    return avFrame;
}

AVPacket *VideoTrim::encodePackage(AVFrame *frame) {
    int ret = avcodec_send_frame(avCtxE, frame);
    if (ret < 0) {
        return NULL;
    }
    AVPacket *avPacket = av_packet_alloc();
    ret = avcodec_receive_packet(avCtxE, avPacket);
    if (ret < 0) {
        av_packet_free(&avPacket);
        return NULL;
    }
    return avPacket;
}

/**
 * 要进行时间基数进行转换，因为输入输入的时间基数可能不一样
 * @param packet
 */
void VideoTrim::writePacket(AVPacket *packet) {
    packet->pts = av_rescale_q_rnd(packet->pts, videoStream->time_base,
                                   outputVideoStream->time_base, AV_ROUND_NEAR_INF);
    packet->dts = av_rescale_q_rnd(packet->dts, videoStream->time_base,
                                   outputVideoStream->time_base, AV_ROUND_NEAR_INF);
    //duration也要转换
    packet->duration = av_rescale_q(packet->duration, videoStream->time_base,
                                    outputVideoStream->time_base);
    int ret = av_interleaved_write_frame(outputFormatContext, packet);

    if (ret < 0) {
        LOGE("av_interleaved_write_frame fail");
    }
}


int addVideoStreamTest(AVFormatContext *avFormatContext, AVOutputFormat *avOutputFormat, int width,
                       int height) {

    AVStream *videoStream = avformat_new_stream(avFormatContext, NULL);
    if (videoStream == NULL) {
        LOGE("avformat_new_stream fail");
        return -1;
    }
    if (avOutputFormat->video_codec == AV_CODEC_ID_NONE) {
        LOGE("avOutputFormat->video_codec==AV_CODEC_ID_NONE");
        return -1;
    }
    //videoStream->codecpar 通过avCodecContext输出参数设置
    AVCodec *codec = avcodec_find_encoder(avOutputFormat->video_codec);
    AVCodecContext *avCodecContext = avcodec_alloc_context3(codec);
    avCodecContext->width = width;
    avCodecContext->height = height;
    //帧率和时间戳要匹配
    avCodecContext->framerate = AVRational{25, 1};
    avCodecContext->time_base = AVRational{1, 25};
    avCodecContext->pix_fmt = AV_PIX_FMT_YUV420P;
    avCodecContext->max_b_frames = 5;

    //设置输出参数
    if (avcodec_parameters_from_context(videoStream->codecpar, avCodecContext) < 0) {
        LOGE("avcodec_parameters_from_context fail");
        return -1;
    }
    //打开编码器
    if (avcodec_open2(avCodecContext, codec, NULL) < 0) {
        LOGE("avCodecContext avcodec_open2 fail");
        return -1;
    }

    avCodecContext->bit_rate = 40000;
    return 0;
}

void initOutputTest() {
    char *outPath = "";
    AVFormatContext *outputFormatContext;
    int ret = avformat_alloc_output_context2(&outputFormatContext, NULL, NULL, outPath);
    if (ret < 0) {
        LOGE("avformat_alloc_output_context2 fail: %s", outPath);
    }
    AVOutputFormat *avOutputFormat = outputFormatContext->oformat;
    //输出的视频流，音频流
}


