package com.liangzs.videocall

import android.media.MediaCodec
import android.media.MediaCodec.BufferInfo
import android.media.MediaCodecInfo
import android.media.MediaFormat
import android.view.Surface
import java.nio.ByteBuffer

class H265DecoderFetch(surface: Surface, val width: Int, val height: Int) {
    lateinit var mediaCodec: MediaCodec;

    init {
        mediaCodec = MediaCodec.createDecoderByType("video/hevc")
        val mediaFormat = MediaFormat.createVideoFormat(MediaFormat.MIMETYPE_VIDEO_HEVC, 720, 1280)
        mediaFormat.setInteger(MediaFormat.KEY_BIT_RATE, width * height)
        mediaFormat.setInteger(MediaFormat.KEY_FRAME_RATE, 25)
        mediaFormat.setInteger(MediaFormat.KEY_I_FRAME_INTERVAL, 5)
        mediaFormat.setInteger(MediaFormat.KEY_COLOR_FORMAT, MediaCodecInfo.CodecCapabilities.COLOR_FormatYUV420Flexible)
        mediaCodec.configure(mediaFormat, surface, null, 0)
    }

    fun decodeFrame(byteArray: ByteArray) {
        //数据丢进去解码
        val inputIndex = mediaCodec.dequeueInputBuffer(10000)
        if (inputIndex > 0) {
            val buffer = mediaCodec.getInputBuffer(inputIndex)
            buffer?.clear()
            buffer?.put(byteArray);
            //通知解码
            mediaCodec.queueInputBuffer(inputIndex, 0, byteArray.size, System.currentTimeMillis(), 0)
        }
        //获取解码渲染到surface上
        val bufferInfo: BufferInfo = BufferInfo();
        var outputIndex = mediaCodec.dequeueOutputBuffer(bufferInfo, 10000)
        while (outputIndex > 0) {
            mediaCodec.releaseOutputBuffer(outputIndex, false)
            outputIndex = mediaCodec.dequeueOutputBuffer(bufferInfo, 10000)
        }
    }
}