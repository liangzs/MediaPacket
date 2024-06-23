package com.liangzs.ffmplayer

class NativeLib {

    /**
     * A native method that is implemented by the 'ffmplayer' native library,
     * which is packaged with this application.
     */
    external fun stringFromJNI(): String

    companion object {
        // Used to load the 'ffmplayer' library on application startup.
        init {
            System.loadLibrary("ffmplayer")
        }
    }
}