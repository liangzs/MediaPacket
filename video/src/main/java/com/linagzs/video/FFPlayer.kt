package com.linagzs.video

import android.view.Surface

class FFPlayer {
    external fun play(path: String, surface: Surface?): Int

    companion object {
        init {
            System.loadLibrary("ffplayer")
        }
    }
}