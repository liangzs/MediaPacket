package com.liangzs.videocall

import org.java_websocket.client.WebSocketClient
import org.java_websocket.handshake.ServerHandshake
import java.lang.Exception
import java.net.URI
import java.nio.ByteBuffer

/**
 * 采用h265进行通讯
 */
class SocketH265 {
    val socketClient: MyWebSocketClient = MyWebSocketClient(URI("ws://237.84.2.178:8080"))

    fun start() {
        socketClient.connect()
    }

    /**
     * 给对端发送数据
     */
    fun sendData(bytes: ByteBuffer?) {
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