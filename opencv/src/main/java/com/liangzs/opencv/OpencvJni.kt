package com.liangzs.opencv

class OpencvJni {
    companion object {
        init {
            System.loadLibrary("opencv")
        }
    }

    external fun init(path: String);
}