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

VideoTrim::~VideoTrim() {

}

/**
 * c方法，执行thread方法
 * @param data
 * @return
 */
void *threadTrim(void *data) {
    VideoTrim *trim = static_cast<VideoTrim *>(data);
    trim->trimImpl();
    pthread_exit(&trim->trimThread);
}

void VideoTrim::startTrim() {

    pthread_create(&trimThread, NULL, threadTrim, this);
}

int64_t time_to_timestamp(double time_in_seconds, AVRational time_base) {
    // 将秒转换为时间戳
    return (int64_t) (time_in_seconds * time_base.den + 0.5) / time_base.num;
}


void VideoTrim::trimImpl() {
    avcodec_register_all();
    if (initInput() < 0) {
        return;
    }
    if (initOutput() < 0) {
        return;
    }
    //
//    int seekTime = time_to_timestamp(startTime/1000, avCtxD->time_base);//这里是标准计算，也可以通过时间戳进行计算
    int seekTime = startTime / 1000 * AV_TIME_BASE * inputFormatContext->start_time;
    int ret = av_seek_frame(inputFormatContext, -1, seekTime / 1000, AVSEEK_FLAG_BACKWARD);
    if (ret < 0) {
        LOGE("av_seek_frame fail");
        return;
    }

    while (true) {
        avPacket = av_packet_alloc();
        ret = av_read_frame(inputFormatContext, avPacket);
        if (ret < 0) {
            LOGE("av_read_frame break");
            av_packet_unref(avPacket);
            break;
        }
        if (avPacket->stream_index == videoStreamIndex) {
            //视频解析
            AVFrame *frame = decodeAvPackage();
            if (frame != NULL) {

            }
            av_frame_free(&frame);

        } else if (avPacket->stream_index == audioStreamIndex) {
            //音频解析
        }

        //av_write_frame：直接写入帧，不进行交错处理。适用于单流或已经手动处理交错的场景。
        //av_interleaved_write_frame：交错写入帧，确保音视频同步。适用于多流的场景，推荐在包含视频和音频的情况下使用
    }

    //写文件尾,muxer的整理三个步骤就是写文件头，write_frame,写文件尾

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
    avOutputFormat = outputFormatContext->oformat;
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
    //打开输出文件
    if (!(avOutputFormat->flags & AVFMT_NOFILE)) {
        ret = avio_open(&outputFormatContext->pb, outputPath, AVIO_FLAG_WRITE);
        if (ret < 0) {
            LOGE("could not open file avio_open:%s", outputPath);
            return -1;
        }
    }
    //写入文件头
    ret = avformat_write_header(outputFormatContext, NULL);
    if (ret < 0) {
        LOGE("avformat_write_header fail");
        return -1;
    }
    LOGE(" INIT OUTPUT SUCCESS !");
    return 0;

}

int VideoTrim::addVideoStream(int width, int height) {
    //avformat_alloc_output_context2这里初始化outputFormatContext
    outputVideoStream = avformat_new_stream(outputFormatContext, NULL);
    if (outputVideoStream == NULL) {
        LOGE("addVideoStream:avformat_new_stream fail");
        return -1;
    }
    //检测输出文件编码是否none
    if (avOutputFormat->video_codec == AV_CODEC_ID_NONE) {
        LOGE("avOutputFormat->video_codec == AV_CODEC_ID_NONE");
        return -1;
    }
    avCodecE = avcodec_find_decoder(avOutputFormat->video_codec);
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
    if (avOutputFormat->audio_codec == AV_CODEC_ID_NONE) {
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

AVFrame *VideoTrim::decodeAvPackage() {
    int ret = avcodec_send_packet(avCtxD, avPacket);
    if (ret < 0) {
        LOGE("  avcodec_send_packet %s ", av_err2str(ret));
        return NULL;
    }
    AVFrame *avFrame = av_frame_alloc();
    ret = avcodec_receive_frame(avCtxD, avFrame);
    if (ret < 0) {
        av_frame_free(&avFrame);
        LOGE("  avcodec_receive_frame %s ", av_err2str(ret));
        return NULL;
    }
    return avFrame;
}

AVPacket *VideoTrim::encodeAvPackage() {
    return nullptr;
}
