package com.liangzs.videocall

import android.media.MediaCodec
import android.media.MediaCodec.BufferInfo
import android.media.MediaCodecInfo
import android.media.MediaFormat
import org.java_websocket.client.WebSocketClient
import org.java_websocket.handshake.ServerHandshake
import java.net.URI
import java.nio.ByteBuffer
import kotlin.experimental.and

/**
 * 采用h265进行通讯
 */
class SocketH265EncoderPush(val width: Int, val height: Int, val remoteCallback: LocalSurfaceView.RemoteCallback?) {
    var frameIndex = 0;

//    var socketLive: SocketLiveClient? = null;
    var socketLive: SocketLiveServer? = null;
    var mediaCodecEncoder: MediaCodec? = null;
    var outputIndex: Int = 0;
    lateinit var outBufferInfo: BufferInfo;
    var outByteBuffer: ByteBuffer? = null;
    fun start() {
        frameIndex = 0;
        mediaCodecEncoder = MediaCodec.createEncoderByType("video/avc")
        val mediaFormat = MediaFormat.createVideoFormat(MediaFormat.MIMETYPE_VIDEO_AVC, 720, 1280);
        mediaFormat.setInteger(MediaFormat.KEY_FRAME_RATE, 25)
        mediaFormat.setInteger(MediaFormat.KEY_BIT_RATE, width * height)
        mediaFormat.setInteger(MediaFormat.KEY_I_FRAME_INTERVAL, 5)
//        mediaFormat.setInteger(MediaFormat.KEY_COLOR_FORMAT, MediaCodecInfo.CodecCapabilities.COLOR_FormatYUV420Flexible)
        mediaCodecEncoder?.configure(mediaFormat, null, null, MediaCodec.CONFIGURE_FLAG_ENCODE)
//        socketLive = SocketLiveClient(object : SocketLiveClient.SocketCallback {
//            override fun callBack(data: ByteArray?) {
//                remoteCallback?.onData(data!!)
//            }
//
//        })
        socketLive = SocketLiveServer(object : SocketLiveServer.SocketCallback {
            override fun callBack(data: ByteArray?) {
                remoteCallback?.onData(data!!)
            }

        })

        socketLive?.start()
    }

    /**
     * 给对端发送数据
     */
    fun sendData(bytes: ByteArray?) {
        //原始画面yuv21转成 yuv420p
        val n12 = YuvUtil.nv21toNV12(bytes!!)

        //可能出现的旋转处理,后续观察处理
        // 获取排队号
        try {

            val inputIndex = mediaCodecEncoder!!.dequeueInputBuffer(10000)
            if (inputIndex >=0) {
                val byteBuffer = mediaCodecEncoder!!.getInputBuffer(inputIndex);
                byteBuffer?.clear()
                byteBuffer?.put(n12)
                //计算时间戳
                val timeUs = calcPresentTimeUs()
                //排队解码
                mediaCodecEncoder?.queueInputBuffer(inputIndex, 0, n12!!.size, timeUs, 0)

            }

            outBufferInfo = MediaCodec.BufferInfo();
            outputIndex = mediaCodecEncoder!!.dequeueOutputBuffer(outBufferInfo, 10000);
            //不停取，取光它
            while (outputIndex >= 0) {
                outByteBuffer = mediaCodecEncoder?.getOutputBuffer(outputIndex)

                //发送数据，识别到I帧，然后在I帧的前面插入pps，sps媒体数据
                dealFrameH265(outByteBuffer!!, outBufferInfo)
                mediaCodecEncoder?.releaseOutputBuffer(outputIndex, false)
                outputIndex = mediaCodecEncoder!!.dequeueOutputBuffer(outBufferInfo, 10000);
            }
            //原始画面yuv420p转成 h265
            socketLive?.sendData(bytes)
        } catch (e: Exception) {
            e.printStackTrace()
        }
    }

    var offset = 4;
    val NAL_I = 19
    val NAL_VPS = 32
    var vps_pps_sps: ByteArray? = null

    /**
     * 处理帧，在I帧中插入pps，sps
     */
    fun dealFrameH265(byteBuffer: ByteBuffer, bufferInfo: BufferInfo) {
        offset = 4;//00 00 00 01 pps

        //检测是否00 00 01 pps
        if (byteBuffer.get(2) == 0x01.toByte()) {
            offset = 3;
        }
        //h265 是取8位中的中间6位
//        val type = byteBuffer.get(offset) and (0x7E).shr(1)
//        if (type.toInt() == NAL_VPS) {
//            vps_pps_sps = ByteArray(bufferInfo.size)
//            byteBuffer.get(vps_pps_sps)
//        } else if (type.toInt() == NAL_I) {
//            //这里和vps_sps_pps进行拼接
//            val newByteArray = ByteArray(bufferInfo.size + (vps_pps_sps?.size ?: 0))
//            val bytes = ByteArray(bufferInfo.size);
//            byteBuffer.get(bytes)
//            System.arraycopy(vps_pps_sps, 0, newByteArray, 0, (vps_pps_sps?.size ?: 0))
//            System.arraycopy(bytes, 0, newByteArray, (vps_pps_sps?.size ?: 0), bytes.size)
//            socketLive?.sendData(newByteArray)
//        } else {
            val newByteArray = ByteArray(bufferInfo.size + (vps_pps_sps?.size ?: 0))
            byteBuffer.get(newByteArray)
            socketLive?.sendData(newByteArray)
//        }
    }

    /**
     * 按25帧,40ms进行计算
     */
    fun calcPresentTimeUs(): Long {
        return 40000000L * frameIndex
    }

}