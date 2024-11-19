package com.liangzs.opencv

class OpencvJni {
    companion object {
        init {
            System.loadLibrary("opencv")
        }
    }

    external fun init(path: String);

    external fun postData(byteArray: ByteArray?, w: Int, h: Int, cameraId: Int);
}