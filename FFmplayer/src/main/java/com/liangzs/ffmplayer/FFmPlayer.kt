package com.liangzs.ffmplayer

import android.util.Log

class FFmPlayer {
    var listener: OnPreparedListener? = null


    external fun setDataSource(path: String);

    external fun prepare();

    external fun start();

    external fun pause();

    external fun stop();

    external fun release();

    external fun resume();

    external fun setMute(track: Int);

    //jni回调java的方法

    /**
     * prepare结束了进行回调
     */
    fun onCallJavaPrepared() {
        Log.i(javaClass.simpleName, "onCallJavaPrepared:sucessful")
        listener?.onPrepared()
    }

    fun onCallJavaProgress(progress: Int, duration: Int) {
        listener?.onProgress(progress, duration)
    }


    interface OnPreparedListener {
        fun onPrepared()
        fun onProgress(progress: Int, duration: Int)
    }

    companion object {
        // Used to load the 'ffmplayer' library on application startup.
        init {
            System.loadLibrary("ffmplayer")
        }
    }
}