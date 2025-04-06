
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


/**
 * 查询seek位置和当前gop区间的数据，用编解码，记得编码数据要和原数据。在gop后直接赋值流
 */
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

//    if (ret < 0) {
//        LOGE("buildOutput fail");
//        return;
//    }

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
    LOGE("progress:100");
//    callProgress(100);


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
    hadReachKeyFrame = false;

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
        LOGE("currentTime:%d ,isVideo:%s", currentTime,
             avPacket->stream_index == videoStreamIndex ? "true" : "false");

        //视频帧，重新走解码，编码流程
        if (currentTime > endTime) {//结束
            LOGE("currentTime > endTime:finish %d,%d", currentTime, endTime);
            av_packet_free(&avPacket);
            break;
        }
        //音频流直接复制
        if (avPacket->stream_index == audioStreamIndex) {
            if (startPts[audioStreamIndex] == 0) {
                startPts[audioStreamIndex] = avPacket->pts;
            }
            if (startDts[audioStreamIndex] == 0) {
                startDts[audioStreamIndex] = avPacket->dts;
            }
            audioPtsIndex++;
            writePackPage(avFormatContextOut, avStreamAudioIn, avStreamAudioOut, avPacket);
            av_packet_free(&avPacket);
            continue;
        }
        //这里不应该根据seek进行判断，而是seek时间戳所在的gop都应该重新编解码，gop后直接赋值码流
//            if (currentTime >= startTime && currentTime <= endTime) {
        //计算最初时间
        //如果没到指定位置就一直正常解码就行,为什么要解码，因为后续的b帧搞不好要依赖它才能正常
        if (currentTime < startTime) {
            //如果是视频&&在未到达指定帧之前一直进行sendFrame进行解码，获取到完整的帧
            frame = decodePacket(avCtxVideoIn, avPacket);
            if (frame != NULL) {
                AVPacket *newPackage = encodeFrame(frame, avCtxVideoOut);
                if (newPackage != NULL) {
                    LOGI("< startTime   newPackage not null  go writePackPage");
                    writePackPage(avFormatContextOut, avStreamVideoIn, avStreamVideoOut,
                                  newPackage);
                } else {
                    LOGI("< startTime   newPackage is null...");
                }
                av_packet_free(&newPackage);
            }
            av_frame_free(&frame);
            av_packet_free(&avPacket);
            continue;
        }

        if (startPts[videoStreamIndex] == 0) {
            startPts[videoStreamIndex] = avPacket->pts;
        }
        if (startDts[videoStreamIndex] == 0) {
            startDts[videoStreamIndex] = avPacket->dts;
        }
        if (!(avPacket->flags & AV_PKT_FLAG_KEY) && !hadReachKeyFrame) {
//            videoPtsIndex++;

//                avPacket->flags |= AV_PKT_FLAG_KEY;
            frame = decodePacket(avCtxVideoIn, avPacket);
            if (frame != NULL) {
                AVPacket *newPackage = encodeFrame(frame, avCtxVideoOut);
                if (newPackage != NULL) {
                    LOGI("newPackage not null  go writePackPage");
                    writePackPage(avFormatContextOut, avStreamVideoIn, avStreamVideoOut,
                                  newPackage);
                } else {
                    LOGI("newPackage is null...");
                }
                av_packet_free(&newPackage);
            }
            av_frame_free(&frame);
            av_packet_free(&avPacket);
            continue;
        }
        hadReachKeyFrame = true;
        writePackPage(avFormatContextOut, avStreamVideoIn, avStreamVideoOut, avPacket);
        //计算progress
//        callProgress((currentTime - startTime) * 1.0f / (endTime - startTime) * 100);
        LOGE("progress:%d", (currentTime - startTime) * 100 / (endTime - startTime));
    }

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


/**
 * 这里有一点很诡异avformat_new_stream创建的时候，是根据顺序进行创建的，
 * 所以音频和视频的创建的顺序不一样竟然会影响输入，所最好是输入和输出的stream_index能保持一致
 * @return
 */
int VideoTrim::buildOutput() {
    int ret = 0;
    ret = initOutput(outPath, &avFormatContextOut);
    if (ret < 0) {
        LOGE("initOutput fail:%s", outPath);
        return ret;
    }

//② 用1就不用2
//    //用同一个编解码
//    avStreamVideoOut = avformat_new_stream(avFormatContextOut, NULL);
//    if (avStreamVideoOut == NULL) {
//        LOGE(" VIDEO STREAM NULL ");
//        return -1;
//    }
//    if (avFormatContextOut->oformat->video_codec == AV_CODEC_ID_NONE) {
//        LOGE(" VIDEO AV_CODEC_ID_NONE ");
//        return -1;
//    }
//    ret = avcodec_parameters_copy(avStreamVideoOut->codecpar,
//                                  avFormatContextIn->streams[videoStreamIndex]->codecpar);
////    avFormatContextOut->bit_rate = avFormatContextIn->streams[videoStreamIndex]->codecpar->bit_rate;
////    avStreamVideoOut->r_frame_rate = avFormatContextIn->streams[videoStreamIndex]->r_frame_rate;
//    avStreamVideoOut->time_base = avFormatContextIn->streams[videoStreamIndex]->time_base;
//    if (ret < 0) {
//        LOGE("avcodec_parameters_copy video fail:%s", av_err2str(ret));
//        return ret;
//    }

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
    ret = avcodec_parameters_copy(avStreamAudioOut->codecpar,
                                  avFormatContextIn->streams[audioStreamIndex]->codecpar);
    avStreamAudioOut->codecpar->codec_tag = 0;
//    avStreamAudioOut->r_frame_rate = avFormatContextIn->streams[audioStreamIndex]->r_frame_rate;
    avStreamAudioOut->time_base = avFormatContextIn->streams[audioStreamIndex]->time_base;
    if (ret < 0) {
        LOGE("avcodec_parameters_copy audio fail:%s", av_err2str(ret));
        return ret;
    }

    //①
    ret = addOutputVideoStreamCopy(avFormatContextOut, &avCtxVideoOut, avFormatContextIn);
    if (ret < 0) {
        LOGE("addOutputVideoStream fail:%s", av_err2str(ret));
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
int pts;
int dts;
int duration;

int
VideoTrim::writePackPage(AVFormatContext *avFormatContext, AVStream *inStream, AVStream *outStream,
                         AVPacket *avPacket) {
    LOGI("writePackPage stream_index:%d,real pts:%ld",
         avPacket->stream_index, avPacket->pts - startPts[avPacket->stream_index]);
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
    pts = avPacket->pts;
    dts = avPacket->dts;
    duration = avPacket->duration;
    int ret = av_interleaved_write_frame(avFormatContext, avPacket);
    if (ret < 0) {
        LOGE("av_interleaved_write_frame fail:%s,avPacket->pts:%ld,avPacket->dts:%ld,avPacket->duration:%ld",
             av_err2str(ret), pts, dts, duration);
    } else {
        LOGE("av_interleaved_write_frame success:%s,avPacket->pts:%ld,avPacket->dts:%ld,avPacket->duration:%ld",
             av_err2str(ret), pts, dts, duration);
    }
    return ret;
}

void VideoTrim::cancel() {
    this->isCancel = true;
    progress = 0;
}




