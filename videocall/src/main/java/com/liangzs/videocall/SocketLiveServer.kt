package com.liangzs.videocall

import org.java_websocket.WebSocket
import org.java_websocket.handshake.ClientHandshake
import org.java_websocket.server.WebSocketServer
import java.net.InetSocketAddress
import java.nio.ByteBuffer

//音视频通话服务端
class SocketLiveServer(private val socketCallback: SocketCallback) {
    private val webSocket: WebSocket? = null
    fun start() {
        webSocketServer.start()
    }

    fun close() {
        try {
            webSocket!!.close()
            webSocketServer.stop()
        } catch (e: InterruptedException) {
            e.printStackTrace()
        }
    }

    private val webSocketServer: WebSocketServer = object : WebSocketServer(InetSocketAddress(3800)) {
        override fun onOpen(webSocket: WebSocket, clientHandshake: ClientHandshake) {
            var webSocket: WebSocket? = webSocket
            webSocket = webSocket
        }

        override fun onClose(webSocket: WebSocket, i: Int, s: String, b: Boolean) {}
        override fun onMessage(webSocket: WebSocket, s: String) {}
        override fun onMessage(conn: WebSocket, bytes: ByteBuffer) {
            val buf = ByteArray(bytes.remaining())
            bytes[buf]
            socketCallback.callBack(buf)
        }

        override fun onError(webSocket: WebSocket, e: Exception) {}
        override fun onStart() {}
    }

    fun sendData(bytes: ByteArray?) {
        if (webSocket != null && webSocket.isOpen) {
            webSocket.send(bytes)
        }
    }

    interface SocketCallback {
        fun callBack(data: ByteArray?)
    }
}
