#include <jni.h>
#include <string>
#include <android/log.h>
#include <android/native_window.h>
#include <android/native_window_jni.h>

#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, "nativelib", __VA_ARGS__)
extern "C" {
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "libavutil/avutil.h"
#include "libswscale/swscale.h"
#include "libavutil/imgutils.h"
}

AVFormatContext *avFormatContext;
AVCodecContext *avCodecContext;
AVCodec *avCodec;
SwsContext *swsContext;


extern "C"

JNIEXPORT jint JNICALL
Java_com_linagzs_video_FFPlayer_play(JNIEnv *env, jobject thiz, jstring path, jobject surface) {
    //把路径转成c支持的char*
    char *pathChar = const_cast<char *>(env->GetStringUTFChars(path, 0));
    avcodec_register_all();

    avFormatContext = avformat_alloc_context();

    if (avformat_open_input(&avFormatContext, pathChar, NULL, NULL) != 0) {
        LOGI("打开文件失败");
        return -1;
    }
    if (avformat_find_stream_info(avFormatContext, NULL) < 0) {
        LOGI("获取流信息失败");
        return -1;
    }

    int videoStreamIndex = -1;
    for (int i = 0; i < avFormatContext->nb_streams; i++) {
        if (avFormatContext->streams[i]->codecpar->codec_type == AVMediaType::AVMEDIA_TYPE_VIDEO) {
            videoStreamIndex = i;
            break;
        }
    }
    if (videoStreamIndex == -1) {
        LOGI("找不到视频流");
        return -1;
    }
    //先有解码器上下文，然后再找到解码器，一般都是通过上下文
    avCodecContext = avFormatContext->streams[videoStreamIndex]->codec;
    avCodec = avcodec_find_decoder(avCodecContext->codec_id);

    //用open2方案
    if (avcodec_open2(avCodecContext, avCodec, NULL) < 0) {
        LOGI("打开解码器失败");
        return -1;
    }

    //推送package，得到avframe是解码。推送avframe，得到package是编码
    //原始的挂钩的avPacket和avFrame由ffmpeg进行内存分配
    //av_packet_alloc()+av_packet_ref().
    AVPacket *avPacket = av_packet_alloc();
    av_packet_free(&avPacket);
    //av_frame_free
    AVFrame *avFrame = av_frame_alloc();
    av_frame_free(&avFrame);
    AVFrame *rgbFrame = av_frame_alloc();
    //中转的avframe作为最终展示
    int ret;

    //定义window 进行展示
    ANativeWindow *window = ANativeWindow_fromSurface(env, surface);


    //设置窗口的缓冲区
    ANativeWindow_setBuffersGeometry(window, avCodecContext->width, avCodecContext->height,
                                     ANativeWindow_LegacyFormat::WINDOW_FORMAT_RGBA_8888);
    ANativeWindow_Buffer windowBuffer;;
    int numbytes = av_image_get_buffer_size(AVPixelFormat::AV_PIX_FMT_RGBA, avCodecContext->width,
                                            avCodecContext->height, 1);
    //创建缓冲区来接受数据,rgbFrame就是接受缓冲区数据的
    uint8_t *outBuffer = static_cast<uint8_t *>(av_malloc(sizeof(uint8_t) * numbytes));
    //格式化
    av_image_fill_arrays(rgbFrame->data, rgbFrame->linesize, outBuffer,
                         AVPixelFormat::AV_PIX_FMT_RGBA, avCodecContext->width,
                         avCodecContext->height, 1);
    int destW = avCodecContext->width;
    int destH = avCodecContext->height;
    swsContext = sws_getContext(avCodecContext->width, avCodecContext->height,
                                avCodecContext->pix_fmt, destW, destH,
                                AVPixelFormat::AV_PIX_FMT_RGBA, SWS_BILINEAR, NULL, NULL, NULL);

    while (av_read_frame(avFormatContext, avPacket) == 0) {
        if (avPacket->stream_index == videoStreamIndex) {
            ret = avcodec_send_packet(avCodecContext, avPacket);
            if (ret < 0 && ret != AVERROR(EAGAIN) && ret != AVERROR_EOF) {
                LOGI("解包失败");
                return -1;
            }
            while (avcodec_receive_frame(avCodecContext, avFrame) == 0) {
                //进行转换
                sws_scale(swsContext, avFrame->data, avFrame->linesize, 0, avCodecContext->height,
                          rgbFrame->data, rgbFrame->linesize);
                //逐行复制
                ANativeWindow_lock(window, &windowBuffer, NULL);
                //window的stride需要乘4  windowBuffer.stride * 4
                uint8_t *dst = static_cast<uint8_t *>(windowBuffer.bits);
                for (int i = 0; i < avCodecContext->height; i++) {
                    memcpy(dst + i * windowBuffer.stride * 4,
                           rgbFrame->data[0] + i * rgbFrame->linesize[0], rgbFrame->linesize[0]);
                }
                ANativeWindow_unlockAndPost(window);

            }
        }


    }


    ANativeWindow_release(window);
    //两者需成双成对
    env->ReleaseStringUTFChars(path, pathChar);
    return 0;
}