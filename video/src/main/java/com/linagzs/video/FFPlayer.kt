package com.linagzs.video

import android.view.Surface

class FFPlayer {
    external fun play(): Int

    external fun createPlayer(path: String, surface: Surface?): Int

    external fun getWidth(): Int

    external fun getHeight(): Int

    external fun getRotation(): Int

    external fun release()

    companion object {
        init {
            System.loadLibrary("ffplayer")
        }
    }
}