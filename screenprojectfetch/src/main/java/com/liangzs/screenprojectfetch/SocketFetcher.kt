package com.liangzs.screenprojectfetch

<<<<<<< Updated upstream
import android.media.MediaCodec
import android.media.MediaCodec.BufferInfo
import android.media.MediaFormat
import android.view.Surface
import org.java_websocket.client.WebSocketClient
import org.java_websocket.handshake.ServerHandshake
import java.lang.Exception
import java.net.URI
import java.nio.ByteBuffer

class SocketFetcher(surface: Surface) {
    var socket: WebSocketClient? = null;
    var width: Int = 720;
    var height: Int = 1280;
    var decoder: MediaCodec? = null;

    init {
        socket = MyWebSocketClient(URI("http:192.168.1.1:8080"));
        //初始化mediacodec
        val mediaFormat =
            MediaFormat.createVideoFormat(MediaFormat.MIMETYPE_VIDEO_AVC, width, height);
        mediaFormat.setInteger(MediaFormat.KEY_BIT_RATE, width * height)
        mediaFormat.setInteger(MediaFormat.KEY_FRAME_RATE, 30);
        mediaFormat.setInteger(MediaFormat.KEY_I_FRAME_INTERVAL, 1)
        decoder = MediaCodec.createDecoderByType("video/avc")
        decoder?.configure(mediaFormat, surface, null, 0);
    }

    fun startServer() {
        socket?.connect()
        decoder?.start();
    }

    fun decodeFrame(bytes: ByteArray) {
        val inputIndex = decoder!!.dequeueInputBuffer(10000)
        //推入解码
        if (inputIndex > 0) {
            //赋值数据
            val buffer = decoder?.getInputBuffer(inputIndex);
            buffer?.clear()
            buffer?.put(bytes);//写入成功
            //通知解码
            decoder?.queueInputBuffer(inputIndex, 0, bytes.size, System.currentTimeMillis(), 0)
        }

        //获取数据
        val bufferInfo: BufferInfo = BufferInfo()
        val outIndex = decoder!!.dequeueOutputBuffer(bufferInfo, 10000);
//        while (outIndex > 0) {
//            decoder!!.releaseOutputBuffer(outIndex, true)
//        }
    }

    inner class MyWebSocketClient(uri: URI) : WebSocketClient(uri) {
        override fun onOpen(handshakedata: ServerHandshake?) {
        }

        override fun onMessage(message: String?) {
        }

        override fun onMessage(buffer: ByteBuffer?) {
            val bytes = ByteArray(buffer!!.remaining());
            buffer.get(bytes)
            decodeFrame(bytes)
        }

        override fun onClose(code: Int, reason: String?, remote: Boolean) {
        }

        override fun onError(ex: Exception?) {
            ex?.printStackTrace()
        }

=======
import org.java_websocket.client.WebSocketClient

class SocketFetcher {
    var socketClient: WebSocketClient? = null;

    init {
        socketClient = WebSocketClient("")
    }

    fun start() {
        socketClient?.connect();
>>>>>>> Stashed changes
    }
}