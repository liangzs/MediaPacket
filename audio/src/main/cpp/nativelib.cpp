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
#include <libavutil/time.h>
#include <libswresample/swresample.h>

}

AVFormatContext *avFormatContext;
AVCodecContext *avCodecContext;
AVCodec *avCodec;
//SwsContext *swsContext;
SwrContext *swrContext;
int audioStreamIndex = -1;
ANativeWindow *window;
AVPacket *avPacket;
AVFrame *avFrame;
AVFrame *rgbFrame;
extern "C"

JNIEXPORT jint JNICALL
Java_com_linagzs_video_FFPlayer_play(JNIEnv *env, jobject thiz) {


//    //用open2方案
//    if (avcodec_open2(avCodecContext, avCodec, NULL) < 0) {
//        LOGI("打开解码器失败");
//        return -1;
//    }
//
//    //推送package，得到avframe是解码。推送avframe，得到package是编码
//    //原始的挂钩的avPacket和avFrame由ffmpeg进行内存分配
//    //av_packet_alloc()+av_packet_ref().
//    avPacket = av_packet_alloc();
//    //av_frame_free
//    avFrame = av_frame_alloc();
//
//    rgbFrame = av_frame_alloc();
//    //中转的avframe作为最终展示
//    int ret;
//
//
//    //设置窗口的缓冲区
//    ANativeWindow_setBuffersGeometry(window, avCodecContext->width, avCodecContext->height,
//                                     ANativeWindow_LegacyFormat::WINDOW_FORMAT_RGBA_8888);
//    ANativeWindow_Buffer windowBuffer;;
//    int numbytes = av_image_get_buffer_size(AVPixelFormat::AV_PIX_FMT_RGBA, avCodecContext->width,
//                                            avCodecContext->height, 1);
//    //创建缓冲区来接受数据,rgbFrame就是接受缓冲区数据的
//    uint8_t *outBuffer = static_cast<uint8_t *>(av_malloc(sizeof(uint8_t) * numbytes));
//    //格式化
//    av_image_fill_arrays(rgbFrame->data, rgbFrame->linesize, outBuffer,
//                         AVPixelFormat::AV_PIX_FMT_RGBA, avCodecContext->width,
//                         avCodecContext->height, 1);
//    int destW = avCodecContext->width;
//    int destH = avCodecContext->height;
//    swsContext = sws_getContext(avCodecContext->width, avCodecContext->height,
//                                avCodecContext->pix_fmt, destW, destH,
//                                AVPixelFormat::AV_PIX_FMT_RGBA, SWS_BILINEAR, NULL, NULL, NULL);
//
//    while (av_read_frame(avFormatContext, avPacket) == 0) {
//        if (avPacket->stream_index == videoStreamIndex) {
//            ret = avcodec_send_packet(avCodecContext, avPacket);
//            if (ret < 0 && ret != AVERROR(EAGAIN) && ret != AVERROR_EOF) {
//                LOGI("解包失败");
//                return -1;
//            }
//            while (avcodec_receive_frame(avCodecContext, avFrame) == 0) {
//                //进行转换
//                sws_scale(swsContext, avFrame->data, avFrame->linesize, 0, avCodecContext->height,
//                          rgbFrame->data, rgbFrame->linesize);
//                //逐行复制
//                ANativeWindow_lock(window, &windowBuffer, NULL);
//                //window的stride需要乘4  windowBuffer.stride * 4
//                uint8_t *dst = static_cast<uint8_t *>(windowBuffer.bits);
//                for (int i = 0; i < avCodecContext->height; i++) {
//                    memcpy(dst + i * windowBuffer.stride * 4,
//                           rgbFrame->data[0] + i * rgbFrame->linesize[0], rgbFrame->linesize[0]);
//                }
//                av_usleep(3300);
//                ANativeWindow_unlockAndPost(window);
//            }
//
//        }
//        av_packet_unref(avPacket);
//    }
//    av_packet_free(&avPacket);
//    av_frame_free(&avFrame);
//    av_frame_free(&rgbFrame);
//    avcodec_close(avCodecContext);
//    avformat_close_input(&avFormatContext);
//    ANativeWindow_release(window);

    return 0;
}

/**
 * 音频的播放也需要转化，视频需要sws_scale进行转化，而音频是swr_convert进行转化的
 */
extern "C"
JNIEXPORT jint JNICALL
Java_com_linagzs_video_FFPlayer_createPlayer(JNIEnv *env, jobject thiz, jstring path,
                                             jobject surface) {
    const char *char_path = env->GetStringUTFChars(path, 0);
    av_register_all();

    avFormatContext = avformat_alloc_context();
    if (avformat_open_input(&avFormatContext, char_path, NULL, NULL) != 0) {
        LOGI("open file error");
        return -1;
    }

    if (avformat_find_stream_info(avFormatContext, NULL) < 0) {
        LOGI("获取视频信息失败");
        return -1;
    }
    audioStreamIndex = -1;
    for (int i = 0; i < avFormatContext->nb_streams; i++) {
        if (avFormatContext->streams[i]->codecpar->codec_type == AVMediaType::AVMEDIA_TYPE_AUDIO) {
            audioStreamIndex = i;
            break;
        }
    }
    if (audioStreamIndex == -1) {
        LOGI(" 没有找到音频信道");
        return -1;
    }
    avCodecContext = avFormatContext->streams[audioStreamIndex]->codec;
    avCodec = avcodec_find_decoder(avCodecContext->codec_id);

    //打开文件
    if (avcodec_open2(avCodecContext, avCodec, NULL) != 0) {
        LOGI("打开编码失败");
        return -1;
    }

    avPacket = av_packet_alloc();
    avFrame = av_frame_alloc();


    swrContext = swr_alloc();
    //设置swr的参数,输出参数和输入参数
    //立体声，四体声，六体声
    int64_t out_ch_layout = AV_CH_LAYOUT_STEREO;
    //采样位数
    AVSampleFormat out_sample_fmt = AVSampleFormat::AV_SAMPLE_FMT_S16;
    //还有最重要的采样率,可以跟输入的一致
    int sample_rate = avCodecContext->sample_rate;

    swr_alloc_set_opts(swrContext, out_ch_layout, out_sample_fmt, sample_rate,
                       avCodecContext->channel_layout,
                       avCodecContext->sample_fmt, avCodecContext->sample_rate, 0, 0);
    //音频用swr方式,即重采样，几乎所有的音频输出都会经过重采样，因为音频的喇叭不一致
    //初始化
    swr_init(swrContext);
    //定义缓冲区，数据都是通过缓冲区进行交接 sample_rate*2bit(16)*2(STEREO)
    uint8_t *outBuffer = static_cast<uint8_t *>(av_malloc(44100 * 2 * 2));
    int ret;
    while (av_read_frame(avFormatContext, avPacket) == 0) {
        if (avPacket->stream_index == audioStreamIndex) {
            ret = avcodec_send_packet(avCodecContext, avPacket);
            if (ret < 0) {
                LOGI("avcodec_send_packet error");
                break;
            }
            ret = avcodec_receive_frame(avCodecContext, avFrame);
            if (ret < 0) {
                LOGI("avcodec_receive_frame error");
                break;
            }
            swr_convert(swrContext, &outBuffer,44100 * 2 * 2,
                    (const uint8_t **)(avFrame->data), avFrame->nb_samples);

            int size= av_samples_get_buffer_size(NULL,out_ch_layout,avFrame->nb_samples,out_sample_fmt,1);
        }
    }


    av_packet_unref(avPacket);
    av_frame_unref(avFrame);
    av_frame_free(&avFrame);
    av_packet_free(&avPacket);
    avformat_free_context(avFormatContext);
    avcodec_free_context(&avCodecContext);
    env->ReleaseStringUTFChars(path, char_path);
    return 0;
}
extern "C"
JNIEXPORT jint JNICALL
Java_com_linagzs_video_FFPlayer_getWidth(JNIEnv *env, jobject thiz) {
    return avCodecContext->width;
}
extern "C"
JNIEXPORT jint JNICALL
Java_com_linagzs_video_FFPlayer_getHeight(JNIEnv *env, jobject thiz) {
    return avCodecContext->height;
}
extern "C"
JNIEXPORT void JNICALL
Java_com_linagzs_video_FFPlayer_release(JNIEnv *env, jobject thiz) {
    av_packet_free(&avPacket);
    av_frame_free(&avFrame);
    av_frame_free(&rgbFrame);
    avcodec_close(avCodecContext);
    avformat_close_input(&avFormatContext);
    ANativeWindow_release(window);
}
extern "C"
JNIEXPORT jint JNICALL
Java_com_linagzs_video_FFPlayer_getRotation(JNIEnv *env, jobject thiz) {
}