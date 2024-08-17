//
// Created by Administrator on 2024/7/25.
//

#include "video_reverse.h"

VideoReverse::VideoReverse(char *inputPath, char *outputPath) {
    this->inputPath = static_cast<char *>(malloc(strlen(inputPath) + 1));
    strcpy(this->inputPath, inputPath);
    this->outputPath = static_cast<char *>(malloc(strlen(outputPath) + 1));
    strcpy(this->outputPath, outputPath);
    //设置变量的初始值
    initValue();

    buildInput();
    buildOutput();
    yuvSize = inWdith * inHeight * 3 / 2;
    ySize = inWdith * inHeight;
    readBuffer = static_cast<char *>(malloc(yuvSize));
}

VideoReverse::~VideoReverse() {

}

void VideoReverse::initValue() {
    outFrameRate = 25;
    periodTime = AV_TIME_BASE / getVideoOutFrameRate();
}

/**
 * 解封装，得到input数据
 * @return
 */
int VideoReverse::buildInput() {
    int ret = open_input_file(this->inputPath, &inFormatCtx);
    if (ret < 0) {
        LOGE("open_input_file fail");
        return -1;
    }
    videoStreamIndex = getVideoDecodeContext(inFormatCtx, &inVCodecCtx);
    audioStreamIndex = getAudioDecodeContext(inFormatCtx, &inACodecCtx);
    //检测是否初始化成功
    if (inVCodecCtx == NULL || videoStreamIndex == -1) {
        LOGE("getVideoDecodeContext fail");
        return -1;
    }
    if (inACodecCtx == NULL || audioStreamIndex == -1) {
        LOGE("getAudioDecodeContext fail");
        return -1;
    }
    inWdith = inFormatCtx->streams[videoStreamIndex]->codecpar->width;
    inHeight = inFormatCtx->streams[videoStreamIndex]->codecpar->height;
    //获取时长
    int64_t duration = inFormatCtx->duration * av_q2d(timeBaseFFmpeg);
    return 1;
}


/**
 * 初始化输出配置
 * @return
 */
int VideoReverse::buildOutput() {
    initOutput(outputPath, &outFormatCtx);
    //添加视频信道,传入inpu的codecParameter
    addOutputVideoStream(outFormatCtx, &outVCodecCtx,
                         *inFormatCtx->streams[videoStreamIndex]->codecpar);

    //添加音频信道
    addOutputAudioStream(outFormatCtx, &outACodecCtx,
                         *inFormatCtx->streams[audioStreamIndex]->codecpar);

    outFrame = av_frame_alloc();

    writeOutoutHeader(outFormatCtx, this->outputPath);
    AVCodecParameters *codecpar = outFormatCtx->streams[videoStreamIndex]->codecpar;
    outFrame = av_frame_alloc();
    outFrame->width = codecpar->width;
    outFrame->height = codecpar->height;
    outFrame->format = codecpar->format;
//    av_frame_get_buffer 会产生内存泄露
    return 1;
}

const char *tempYuv = "sdcard/FFmpeg/temp.yuv";

/**
 * 这里进行编解码放到缓存区
 */
void VideoReverse::startReverse() {
    readFinish = false;
    this->run();
    //打开文件
    fCache = fopen(tempYuv, "wb+");
    //遍历解码出所有的关键帧时间戳
    int ret = 0;
    while (!isExist) {
        AVPacket *packet = av_packet_alloc();
        ret = av_read_frame(inFormatCtx, packet);
        if (ret < 0) {
            av_packet_free(&packet);
            break;
        }
        if (packet->stream_index == inputVideoStreamIndex) {
            if (packet->flags & AV_PKT_FLAG_KEY) {
                keyFrameTime.push_back(packet->pts);
            }
            nowKeyFramePts = packet->pts;
            av_packet_free(&packet);
        } else if (packet->stream_index == inputAudioStreamIndex) {
            //音频不做处理，直接缓存输出，需要转换一下package的时间戳
            av_packet_rescale_ts(packet, inFormatCtx->streams[audioStreamIndex]->time_base,
                                 outFormatCtx->streams[audioOutputStreamIndex]->time_base);
            queueAudioPackages.push(packet);
        }
    }

    //从最后一帧开始解析，先seek到最后一帧的关键帧，然后得到gop，然后反向写入gop序列
    nowKeyFramePosition = keyFrameTime.size() - 1;
    av_seek_frame(inFormatCtx, videoStreamIndex, keyFrameTime.at(nowKeyFramePosition),
                  AVSEEK_FLAG_BACKWARD);

    while (!isExist) {
        //最后一gop，或者不是最后的gop时，根据时间戳pts来做边界处理
        AVPacket *avPacket = av_packet_alloc();
        ret = av_read_frame(inFormatCtx, avPacket);
        if (ret < 0) {//结束了
            break;
        }
        /**
         * 完成一个gop之后做处理
         */
        if (avPacket->stream_index == inputVideoStreamIndex) {
            if (((nowKeyFramePosition + 1) >= keyFrameTimeStamps.size() &&
                 avPacket->pts > nowKeyFramePts) ||
                (nowKeyFramePosition + 1) < keyFrameTimeStamps.size() &&
                avPacket->pts > keyFrameTimeStamps.at(nowKeyFramePosition + 1)) {
                //完成了一个gop,然后把这个完整的帧进行反向读取FILE
                completeCode(fCache);
                LOGE(" NEXT GOP %d", nowKeyFramePosition);
                //开始倒序读取
                reverseReadFileToPakage();

                if (seekNextFrame() < 0) {
                    av_packet_free(&avPacket);
                    break;
                }
            }
        }
        //GOP中读取每一帧，然后对每一帧进行写入file
        AVFrame *frame = decodePacket(inVCodecCtx, avPacket);
        writeFrameToFile(frame, fCache);
    }
    readFinish = true;
}

int VideoReverse::seekNextFrame() {
    nowKeyFramePosition--;
    if (nowKeyFramePosition < 0) {
        return -1;
    }
    int ret = av_seek_frame(inFormatCtx, videoStreamIndex,
                            keyFrameTimeStamps.at(nowKeyFramePosition), AVSEEK_FLAG_BACKWARD);
    if (ret < 0) {
        LOGE("seekNextFrame->av_seek_frame fail");
        return -1;
    }
    return 0;
}

/**
 * 这里是遍历缓存进行读取数据
 */
void VideoReverse::run() {
    while (!isExist) {
        if (queueAudioPackages.size() <= 0 || queueVideoPackages.size() <= 0) {
            if (readFinish) {
                break;
            }
            sleep(2000);
            continue;
        }
        AVPacket *aPkt = queueAudioPackages.front();
        AVPacket *vPkt = queueVideoPackages.front();
        if (av_compare_ts(apts, outFormatCtx->streams[audioOutputStreamIndex]->time_base,
                          vpts, outFormatCtx->streams[videoOutputStreamIndex]->time_base) < 0) {

            apts = aPkt->pts;
            aPkt->stream_index = audioOutputStreamIndex;
            av_interleaved_write_frame(outFormatCtx, aPkt);
            av_packet_free(&aPkt);
            queueAudioPackages.pop();
        } else {
            vPkt->stream_index = videoOutputStreamIndex;
            vpts = vPkt->pts;

            progress = (int) (1.0 * av_rescale_q(vpts,
                                                 outFormatCtx->streams[videoOutputStreamIndex]->time_base,
                                                 inFormatCtx->streams[videoStreamIndex]->time_base) /
                    nowKeyFramePts * 100);
            if (progress > 0) {
                progress--;
            }
            LOGE(" PROGRESS %d ", progress);
            av_interleaved_write_frame(outFormatCtx, vPkt);
            av_packet_free(&vPkt);
            queueVideoPackages.pop();
        }
    }
    LOGE(" WRITE TRAIL ");
    progress = 100;
    writeTrail(outFormatCtx);
}

void VideoReverse::completeCode(FILE *file) {
    avcodec_flush_buffers(inVCodecCtx);
    int result = 0;
    do {
        AVFrame *frame = av_frame_alloc();
        result = avcodec_receive_frame(inVCodecCtx, frame);
        writeFrameToFile(frame, file);
        av_frame_free(&frame);
    } while (result > 0);
}

/**
 * 写出yuv数据
 * y占w*h
 * uv 各占 w*h* 1/4
 * frame->linesize[0]  *inHeight/2 而不是/4
 * vFrame->linesize[1] 和 vFrame->linesize[2] 表示 U 和 V 分量的每一行占用的字节数，通常等于 Width/2
 * 这里解释一下
 * @param frame
 * @param file
 */
void VideoReverse::writeFrameToFile(AVFrame *frame, FILE *file) {
    //    size_t written = fwrite(array, sizeof(int), num_elements, fp);
    fwrite(frame->data[0], 1, frame->linesize[0] * inHeight, file);
    fwrite(frame->data[1], 1, frame->linesize[1] * inHeight / 2, file);
    fwrite(frame->data[2], 1, frame->linesize[2] * inHeight / 2, file);
}

/**
 * 反向读取文件，然后转化成Frame在编码成package
 */
void VideoReverse::reverseReadFileToPakage() {
    //清理缓存
    fflush(fCache);
    //把指针从最末尾开始读取,每次读取yuvsize大小即一帧缓存
    fseek(fCache, 0, SEEK_END);
    while (ftell(fCache) > 0) {
        //指针往前移动yuvSize前的位置，然后读取yuvSize大小即可
        fseek(fCache, -yuvSize, SEEK_CUR);
        fread(readBuffer, 1, yuvSize, fCache);
        //fread yuvSize之后，光标指针又回到了yuvSize后面的位置，所有需要重新把指针移回yuvsize前的位置
        fseek(fCache, -yuvSize, SEEK_CUR);

        //检测
        if (av_frame_is_writable(outFrame) < 0) {
            LOGE("av_frame_make_writable FAILD !");
        }

        //将readBuffer 开始地址给到y，然后ysize后给u，y+u后的地址给v
        outFrame->data[0] = (uint8_t *) readBuffer;
        outFrame->data[1] = (uint8_t *) (readBuffer + ySize);
        outFrame->data[2] = (uint8_t *) (readBuffer + ySize + ySize / 4);

        //设置每行的大小
        outFrame->linesize[0] = inWdith;
        //从这里可以看出，writeFrameToFile  fwrite(frame->data[1], 1, frame->linesize[1] * inHeight / 2
        outFrame->linesize[1] = inWdith / 2;
        outFrame->linesize[2] = inWdith / 2;
        outFrame->pts = periodTime * currentFrameCount;//每帧的时间戳
        currentFrameCount++;

        //把这一个gop反向读取后，编码成avpacke放到结合中，然后进行写入，
        // 其实可以省略这个文件写入步骤，缓存另外一个队列进行存储，只是存储数量比较大
        AVPacket *packet = encodeFrame(outFrame, outVCodecCtx);
        //不知道要不要进行时间戳转化
        queueVideoPackages.push(packet);
        //解索引，重新使用 结构体的所有字段重置为初始状态，确保该结构体可以安全地再次使用
        av_frame_unref(outFrame);
    }
    //关闭后需要重新定义
    /**
     * "r"：以只读模式打开文件。如果文件不存在，fopen 返回 NULL。
    "r+"：以读写模式打开文件。文件必须存在，否则 fopen 返回 NULL。不会清空文件内容，读写操作从文件开始处进行。
    "w"：以只写模式打开文件。如果文件不存在，则创建新文件。如果文件存在，则清空文件内容。
    "w+"：以读写模式打开文件。如果文件不存在，则创建新文件。如果文件存在，则清空文件内容。
    "a"：以追加模式打开文件。写操作从文件末尾开始。如果文件不存在，则创建新文件。
    "a+"：以读写追加模式打开文件。读操作从文件开始处进行，写操作从文件末尾进行。如果文件不存在，则创建新文件。
     */
    //确保在调用 fclose 之后，再次使用文件时重新打开文件，以避免对已经关闭的文件指针进行操作。
    //重新打开文件时，使用合适的模式。例如，"wb+" 模式会以读写模式打开文件，同时会清空文件内容。如果不想清空文件内容，可以选择其他适当的模式（如 "ab+" 追加模式）。
    fclose(fCache);
    fCache = fopen(tempYuv, "wb+");

}

