//
// Created by DELLQ on 28/8/2024.
//

#include "video_trim.h"

VideoTrim::VideoTrim(const char *inputPath, const char *output, long start, long end) {
    this->inputPath = static_cast<char *>(malloc(strlen(inputPath) + 1));
    strcpy(this->inputPath, inputPath);
    this->outPath = static_cast<char *>(malloc(strlen(output) + 1));
    strcpy(this->outPath, output);
    this->startTime = start;
    this->endTime = end;

}

//void *pthreadrun(void *data) {
//    VideoTrim *videoTrim = static_cast<VideoTrim *>(data);
//    videoTrim->trimImpl();
//    pthread_exit(&videoTrim->pthread);
//}

VideoTrim::~VideoTrim() {
//    pthread_create(&pthread, nullptr, pthreadrun, this);
    if (avFormatContextIn != NULL) {
        avformat_close_input(&avFormatContextIn);
        avformat_free_context(avFormatContextIn);
    }
    if (avFormatContextOut != NULL) {
        avformat_free_context(avFormatContextOut);
    }
    if (avCtxVideoOut != NULL) {
        avcodec_free_context(&avCtxVideoOut);
    }
    if (avCtxVideoIn != NULL) {
        avcodec_free_context(&avCtxVideoIn);
    }
    free(startDts);
    free(startPts);

}


void VideoTrim::trimImpl() {
    int ret;
    ret = buildInput();
    if (ret < 0) {
        LOGE("buildInput fail");
    }
    ret = buildOutput();
    if (ret < 0) {
        LOGE("buildOutput fail");
    }

    if (ret < 0) {
        LOGE("buildOutput fail");
        return;
    }

    ret = writeOutoutHeader(avFormatContextOut, outPath);
    if (ret < 0) {
        LOGE("writeOutoutHeader fail");
        return;
    }
    //开始编码
    startDecode();
    if (isCancel) {
        return;
    }
    writeTrail(avFormatContextOut);
    callProgress(100);


}

void VideoTrim::copyContext(AVCodecContext *vCtxIn, AVCodecContext **vCtxE) {

    const AVCodec *codec = avcodec_find_encoder(vCtxIn->codec_id);
    if (codec == NULL) {
        LOGE("copyContext fail");
    }
    //给outVideoStreamIndex赋值
    *vCtxE = avcodec_alloc_context3(codec);
    (*vCtxE)->bit_rate = vCtxIn->bit_rate;
    (*vCtxE)->framerate = vCtxIn->framerate;
    (*vCtxE)->time_base = vCtxIn->time_base;
    (*vCtxE)->gop_size = 100;
    (*vCtxE)->max_b_frames = 1;
    (*vCtxE)->pix_fmt = AV_PIX_FMT_YUV420P;
    (*vCtxE)->codec_type = AVMEDIA_TYPE_VIDEO;
    (*vCtxE)->width = vCtxIn->width;
    (*vCtxE)->height = vCtxIn->height;
    if ((*vCtxE)->codec_id == AV_CODEC_ID_MPEG2VIDEO) {
        (*vCtxE)->max_b_frames = 2;
    }
    if ((*vCtxE)->codec_id == AV_CODEC_ID_MPEG1VIDEO) {
        /* Needed to avoid using macroblocks in which some coeffs overflow.
         * This does not happen with normal video, it just happens here as
         * the motion of the chroma plane does not match the luma plane. */
        (*vCtxE)->mb_decision = 2;
    }
    if ((*vCtxE)->codec_id == AV_CODEC_ID_H264)
        av_opt_set((*vCtxE)->priv_data, "preset", "slow", 0);
    int ret = avcodec_parameters_from_context(avStreamVideoOut->codecpar, *vCtxE);
    if (ret < 0) {
        LOGE("avcodec_parameters_from_context fail");
        return;
    }
    ret = avcodec_open2(*vCtxE, codec, NULL);
    if (ret < 0) {
        LOGE("addOutputVideoStream->avcodec_open2 fail");
        return;
    }
}

/**
 * 开始编码
 */
void VideoTrim::startDecode() {
    //先seek,pts=timeNow=avFrame*pts*av_q2d(time_base))
    //pts=timeNow/q2d
    offsetTime = ((float) startTime / 1000.0) * AV_TIME_BASE;
    LOGE("av_seek_frame offsetTime:%d,time:%d", offsetTime, time);
    //如果用-1就是用了默认流，然后时间戳用的是AV_TIME_BASE
    int ret = av_seek_frame(avFormatContextIn, -1, offsetTime, AVSEEK_FLAG_BACKWARD);
    if (ret < 0) {
        LOGE("av_seek_frame fail");
        return;
    }
    int currentTime;
    AVFrame *frame = NULL;
    hadDrawKeyFrame = false;
    while (!isCancel) {
        avPacket = av_packet_alloc();
        ret = av_read_frame(avFormatContextIn, avPacket);
        if (ret < 0) {
            LOGE("av_read_frame fail");
            av_packet_free(&avPacket);
            break;
        }
        curRational = avFormatContextIn->streams[avPacket->stream_index]->time_base;
        currentTime = (int64_t) avPacket->pts * av_q2d(curRational) * 1000;
//        LOGI("currentTime:%d,startTime:%d,endTime:%d", currentTime, startTime, endTime);
        if (avPacket->stream_index == audioStreamIndex) {
            if (currentTime >= startTime && currentTime <= endTime) {
                if (startPts[audioStreamIndex] == 0) {
                    startPts[audioStreamIndex] = avPacket->pts;
                }
                if (startDts[audioStreamIndex] == 0) {
                    startDts[audioStreamIndex] = avPacket->dts;
                }
                audioPtsIndex++;
                writePackPage(avFormatContextOut, avStreamAudioIn, avStreamAudioOut, avPacket);
            }
        } else {//视频帧，重新走解码，编码流程
            if (currentTime > endTime) {//结束
                LOGE("currentTime > endTime:finish %d,%d", currentTime, endTime);
                av_packet_free(&avPacket);
                break;
            }

            if (currentTime >= startTime && currentTime <= endTime) {
                videoPtsIndex++;
                //计算最初时间
                if (startPts[videoStreamIndex] == 0) {
                    startPts[videoStreamIndex] = avPacket->pts;
                }
                if (startDts[videoStreamIndex] == 0) {
                    startDts[videoStreamIndex] = avPacket->dts;
                }
                if (!hadDrawKeyFrame) {
                    avPacket->flags |= AV_PKT_FLAG_KEY;
                    frame = decodePacket(avCtxVideoIn, avPacket);
                    if (frame != NULL) {
                        LOGI("frame not null  go encodeFrame");
                        AVPacket *newPackage = encodeFrame(frame, avCtxVideoIn);
                        if (newPackage != NULL) {
                            LOGI("newPackage not null  go writePackPage");
                            hadDrawKeyFrame = true;
                            writePackPage(avFormatContextOut, avStreamVideoIn, avStreamVideoOut,
                                          newPackage);
                        }
                        av_packet_free(&newPackage);
                    }
                    av_frame_free(&frame);
                    av_packet_free(&avPacket);
                    if (videoPtsIndex > 5) {
                        LOGE("videoPtsIndex > 5  finish hadDrawKeyFrame");
                        hadDrawKeyFrame = true;
                    }
                    continue;
                }
                writePackPage(avFormatContextOut, avStreamAudioIn, avStreamAudioOut, avPacket);
                //计算progress
                callProgress((currentTime - startTime) * 1.0f / (endTime - startTime) * 100);

            } else {  //在未到达指定帧之前一直进行sendFrame进行解码，获取到完整的帧
                frame = decodePacket(avCtxVideoIn, avPacket);
                if (frame != NULL) {
//                    AVPacket *newPackage = encodeFrame(frame, avCtxVideoOut);
//                    writePackPage(avFormatContextOut, avStreamVideoIn, avStreamVideoOut,
//                                  newPackage);
//                    av_packet_free(&newPackage);
                }
                av_frame_free(&frame);
            }


        }
    }
    av_packet_free(&avPacket);

}


int VideoTrim::buildInput() {
    int ret = 0;
    LOGI("inputPath:%s", inputPath);
    ret = open_input_file(inputPath, &avFormatContextIn);
    if (ret < 0) {
        LOGE("open_input_file fail:%s", inputPath);
        return ret;
    }

    ret = getVideoDecodeContext(avFormatContextIn, &avCtxVideoIn);
    if (ret < 0) {
        LOGE("getVideoDecodeContext fail:%s", inputPath);
        return ret;
    }
    //这里初始化startpts时间
    startPts = (int64_t *) malloc(sizeof(int64_t) * avFormatContextIn->nb_streams);
    memset(startPts, 0, sizeof(int64_t) * avFormatContextIn->nb_streams);
    startDts = (int64_t *) malloc(sizeof(int64_t) * avFormatContextIn->nb_streams);
    memset(startDts, 0, sizeof(int64_t) * avFormatContextIn->nb_streams);
    return 0;
}


int VideoTrim::buildOutput() {
    int ret = 0;
    ret = initOutput(outPath, &avFormatContextOut);
    if (ret < 0) {
        LOGE("initOutput fail:%s", outPath);
        return ret;
    }

//    ret = addOutputVideoStream(avFormatContextOut, &avCtxVideoOut,
//                               *avFormatContextIn->streams[videoStreamIndex]->codecpar);

    //用同一个编解码
    avStreamVideoOut = avformat_new_stream(avFormatContextOut, NULL);
    if (avStreamVideoOut == NULL) {
        LOGE(" VIDEO STREAM NULL ");
        return -1;
    }
    if (avFormatContextOut->oformat->video_codec == AV_CODEC_ID_NONE) {
        LOGE(" VIDEO AV_CODEC_ID_NONE ");
        return -1;
    }
   ret= avcodec_parameters_copy(avStreamVideoOut->codecpar,
                            avFormatContextIn->streams[videoStreamIndex]->codecpar);
//    avFormatContextOut->bit_rate = 4000000;
    avStreamVideoOut->r_frame_rate = avFormatContextIn->streams[videoStreamIndex]->codecpar->framerate;
    avStreamVideoOut->time_base = avFormatContextIn->streams[videoStreamIndex]->time_base;
    if (ret < 0) {
        LOGE("avcodec_parameters_copy video fail:%s", av_err2str(ret));
        return ret;
    }

    //音频直接赋值
    avStreamAudioOut = avformat_new_stream(avFormatContextOut, NULL);
    if (avStreamAudioOut == NULL) {
        LOGE(" VIDEO STREAM NULL ");
        return -1;
    }
    if (avFormatContextOut->oformat->audio_codec == AV_CODEC_ID_NONE) {
        LOGE(" VIDEO AV_CODEC_ID_NONE ");
        return -1;
    }
    ret=avcodec_parameters_copy(avStreamAudioOut->codecpar,
                            avFormatContextIn->streams[audioStreamIndex]->codecpar);
    avStreamAudioOut->codecpar->codec_tag = 0;
    avStreamAudioOut->r_frame_rate = avFormatContextIn->streams[audioStreamIndex]->codecpar->framerate;
    avStreamAudioOut->time_base = avFormatContextIn->streams[audioStreamIndex]->time_base;
    if (ret < 0) {
        LOGE("avcodec_parameters_copy audio fail:%s", av_err2str(ret));
        return ret;
    }


    return ret;

}

void VideoTrim::writeData() {

}

void VideoTrim::readData() {

}

/**
 * 编码包输出
 * @param avPacket
 * @return
 */
int
VideoTrim::writePackPage(AVFormatContext *avFormatContext, AVStream *inStream, AVStream *outStream,
                         AVPacket *avPacket) {
    LOGI("writePackPage avPacket->pts:%ld,stream_index:%d,real pts:%ld",
         avPacket->pts, avPacket->stream_index, avPacket->pts - startPts[avPacket->stream_index]);
    //duration也要转换
    avPacket->pts = avPacket->pts - startPts[avPacket->stream_index];
    avPacket->dts = avPacket->dts - startPts[avPacket->stream_index];
//    avPacket->pts = av_rescale_q(avPacket->pts - startPts[avPacket->stream_index],
//                                 inStream->time_base,
//                                 outStream->time_base);
//    avPacket->dts = av_rescale_q(avPacket->dts - startDts[avPacket->stream_index],
//                                 inStream->time_base,
//                                 outStream->time_base);
//    avPacket->duration = av_rescale_q(avPacket->duration, inStream->time_base,
//                                      outStream->time_base);

    int ret = av_interleaved_write_frame(avFormatContext, avPacket);
    if (ret < 0) {
        LOGE("av_interleaved_write_frame fail:%s,avPacket->pts:%ld,avPacket->dts:%ld,avPacket->duration:%ld",
             av_err2str(ret), avPacket->pts, avPacket->dts, avPacket->duration);
    }
    return ret;
}

void VideoTrim::cancel() {
    this->isCancel = true;
    progress = 0;
}




