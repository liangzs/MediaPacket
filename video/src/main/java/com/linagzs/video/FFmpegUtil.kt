package com.linagzs.video

class FFmpegUtil {

    external fun trim(input: String, output: String, startTime: Long, endTime: Long)

    companion object {
        init {
            System.loadLibrary("ffplayer")
        }
    }
}