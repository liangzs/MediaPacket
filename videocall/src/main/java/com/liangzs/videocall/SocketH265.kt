package com.liangzs.videocall

import android.media.MediaCodec
import android.media.MediaFormat
import org.java_websocket.client.WebSocketClient
import org.java_websocket.handshake.ServerHandshake
import java.lang.Exception
import java.net.URI
import java.nio.ByteBuffer

/**
 * 采用h265进行通讯
 */
class SocketH265(val width: Int, val height: Int) {
    val socketClient: MyWebSocketClient = MyWebSocketClient(URI("ws://237.84.2.178:8080"))
    var mediaCodecEncoder: MediaCodec? = null;
    fun start() {
        mediaCodecEncoder = MediaCodec.createEncoderByType("video/hevc")
        val mediaFormat = MediaFormat.createVideoFormat(MediaFormat.MIMETYPE_VIDEO_HEVC, width, height);
        mediaCodecEncoder?.configure(mediaFormat, null, null, MediaCodec.CONFIGURE_FLAG_ENCODE)
        socketClient.connect()
    }

    /**
     * 给对端发送数据
     */
    fun sendData(bytes: ByteArray?) {

        //原始画面yuv21转成 yuv420p
        //原始画面yuv420p转成 h264
        socketClient.send(bytes)
    }


    class MyWebSocketClient(uri: URI) : WebSocketClient(uri) {
        override fun onOpen(handshakedata: ServerHandshake?) {
        }

        override fun onMessage(message: String?) {
        }

        /**
         * 这里解析远程数据
         */
        override fun onMessage(bytes: ByteBuffer?) {
            super.onMessage(bytes)
        }

        override fun onClose(code: Int, reason: String?, remote: Boolean) {
        }

        override fun onError(ex: Exception?) {
        }

    }
}