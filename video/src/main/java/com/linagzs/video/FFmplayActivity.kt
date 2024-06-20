package com.linagzs.video

import androidx.appcompat.app.AppCompatActivity
import android.os.Bundle
import android.view.Surface
import android.view.SurfaceHolder
import android.view.SurfaceHolder.Callback
import com.linagzs.video.databinding.ActivityFfmplayBinding

/**
 * 采用ffmpeg进行软解播放
 */
class FFmplayActivity : AppCompatActivity() {
    var surface: Surface? = null;
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        val binding = ActivityFfmplayBinding.inflate(layoutInflater)
        setContentView(binding.root)

        binding.surface.holder.addCallback(object : Callback {
            override fun surfaceCreated(holder: SurfaceHolder) {
                surface = holder.surface

            }

            override fun surfaceChanged(holder: SurfaceHolder, format: Int, width: Int, height: Int) {
            }

            override fun surfaceDestroyed(holder: SurfaceHolder) {
            }
        })

        binding.bt.setOnClickListener {
            FFPlayer().play("", surface)
        }
    }
}