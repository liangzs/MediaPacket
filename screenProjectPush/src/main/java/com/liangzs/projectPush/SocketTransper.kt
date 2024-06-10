package com.liangzs.media

import org.java_websocket.WebSocket
import org.java_websocket.handshake.ClientHandshake
import org.java_websocket.server.WebSocketServer

class SocketTransper {
    var webSocket: WebSocket? = null;
    val webSocketServer = object : WebSocketServer() {
        override fun onOpen(conn: WebSocket?, handshake: ClientHandshake?) {
            webSocket = conn
        }

        override fun onClose(conn: WebSocket?, code: Int, reason: String?, remote: Boolean) {
        }

        override fun onMessage(conn: WebSocket?, message: String?) {
        }

        override fun onError(conn: WebSocket?, ex: Exception?) {
        }

        override fun onStart() {
        }
    }


    fun start() {
        webSocketServer.start()
    }

    /**
     * 不停发数据
     */
    fun sendData(data: ByteArray) {
        webSocket?.send(data)
    }

    fun onDestroy() {
        webSocket?.close()
        webSocketServer.stop()
    }
}