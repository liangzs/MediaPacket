package com.liangzs.ffmplayer

class FFmPlayer {
    var listener: OnPreparedListener? = null


    external fun setDataSource(path: String);

    external fun prepare();

    external fun start();

    external fun pause();

    external fun stop();

    external fun release();

    //jni回调java的方法

    /**
     * prepare结束了进行回调
     */
    fun onPrepared() {
        listener?.onPrepared()
    }


    interface OnPreparedListener {
        fun onPrepared()
    }

    companion object {
        // Used to load the 'ffmplayer' library on application startup.
        init {
            System.loadLibrary("ffmplayer")
        }
    }
}