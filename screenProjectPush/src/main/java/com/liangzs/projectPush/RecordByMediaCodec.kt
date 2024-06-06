package com.liangzs.media

import android.hardware.display.DisplayManager
import android.hardware.display.VirtualDisplay
import android.media.MediaCodec
import android.media.MediaFormat
import android.media.projection.MediaProjection
import android.media.projection.MediaProjectionManager

/**
 * 用h265编码录屏文件，然后通过socket进行传送
 */
class RecordByMediaCodec(val mediaProject: MediaProjection, val transper: SocketTransper) {
    lateinit var mediaCodecH265: MediaCodec;
    var width: Int = 720;
    var height: Int = 1280;
    val virtualizer: VirtualDisplay;

    init {
        val mediaFormat =
            MediaFormat.createVideoFormat(MediaFormat.MIMETYPE_VIDEO_HEVC, width, height);
        mediaFormat.setInteger(MediaFormat.KEY_BIT_RATE, width * height)
        mediaFormat.setInteger(MediaFormat.KEY_FRAME_RATE, 30);
        mediaFormat.setInteger(MediaFormat.KEY_I_FRAME_INTERVAL, 1)
        mediaCodecH265 = MediaCodec.createDecoderByType("video/hevc")
        mediaCodecH265.configure(mediaFormat, null, null, MediaCodec.CONFIGURE_FLAG_ENCODE)
        mediaCodecH265.start()

        virtualizer = mediaProject.createVirtualDisplay(
            "dispaly",
            width,
            height,
            1,
            DisplayManager.VIRTUAL_DISPLAY_FLAG_PUBLIC,
            mediaCodecH265.createInputSurface(),
            null,
            null
        );
    }
}