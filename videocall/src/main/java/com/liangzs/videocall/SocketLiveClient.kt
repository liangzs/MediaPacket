package com.liangzs.videocall

import android.util.Log
import org.java_websocket.client.WebSocketClient
import org.java_websocket.handshake.ServerHandshake
import java.net.URI
import java.nio.ByteBuffer

//音视频通话客户端
class SocketLiveClient(private val socketCallback: SocketCallback) {
    var myWebSocketClient: MyWebSocketClient? = null
    fun start() {
        try {
            val url = URI("ws://192.188.1.188:3800")
            myWebSocketClient = MyWebSocketClient(url)
            myWebSocketClient!!.connect()
        } catch (e: Exception) {
            e.printStackTrace()
        }
    }

    fun sendData(bytes: ByteArray?) {
        if (myWebSocketClient != null && myWebSocketClient!!.isOpen) {
            myWebSocketClient!!.send(bytes)
        }
    }

    inner class MyWebSocketClient(serverURI: URI?) : WebSocketClient(serverURI) {
        override fun onOpen(serverHandshake: ServerHandshake) {}
        override fun onMessage(s: String) {}
        override fun onMessage(bytes: ByteBuffer) {
            Log.i("SocketLiveClient", "消息长度  : " + bytes.remaining())
            val buf = ByteArray(bytes.remaining())
            bytes[buf]
            socketCallback.callBack(buf)
        }

        override fun onClose(i: Int, s: String, b: Boolean) {}
        override fun onError(e: Exception) {
            e.printStackTrace()
        }
    }

    interface SocketCallback {
        fun callBack(data: ByteArray?)
    }
}
