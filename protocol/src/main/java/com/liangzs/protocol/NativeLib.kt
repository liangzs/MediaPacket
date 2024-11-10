package com.liangzs.protocol

class NativeLib {

    /**
     * A native method that is implemented by the 'protocol' native library,
     * which is packaged with this application.
     */
    external fun stringFromJNI(): String

    companion object {
        // Used to load the 'protocol' library on application startup.
        init {
            System.loadLibrary("protocol")
        }
    }
}