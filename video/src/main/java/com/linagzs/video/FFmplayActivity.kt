package com.linagzs.video

import android.content.pm.PackageManager
import android.os.Build
import androidx.appcompat.app.AppCompatActivity
import android.os.Bundle
import android.util.Log
import android.view.Surface
import android.view.SurfaceHolder
import android.view.SurfaceHolder.Callback
import android.view.ViewGroup
import com.linagzs.video.databinding.ActivityFfmplayBinding

/**
 * 采用ffmpeg进行软解播放
 */
class FFmplayActivity : AppCompatActivity() {
    var surface: Surface? = null;
    var palyer: FFPlayer? = null;
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        val binding = ActivityFfmplayBinding.inflate(layoutInflater)
        setContentView(binding.root)
        palyer = FFPlayer()
        binding.surface.holder.addCallback(object : Callback {
            override fun surfaceCreated(holder: SurfaceHolder) {
                surface = holder.surface
                palyer?.createPlayer("/sdcard/360随身WiFi宣传片最终版MP4.mp4", surface)
                val width = palyer!!.getWidth();
                val height = palyer!!.getHeight();
                Log.v(javaClass.simpleName, "width=$width, height=$height,binding.surface.layoutParams.width=${binding.surface.layoutParams.width}, binding.surface.layoutParams.height=${binding.surface.layoutParams.height}")

                //动态改变surfaview的宽高
                val screenW = ScreenUtils.getScreenWidth(this@FFmplayActivity)
                var ratio = 1f;
                if (width > screenW) {
                    ratio = width.toFloat() / screenW.toFloat()
                }
                val layoutParams = binding.flContainer.layoutParams
                layoutParams.width = (width / ratio).toInt();
                layoutParams.height = (height / ratio).toInt();
                binding.flContainer.layoutParams = layoutParams
            }

            override fun surfaceChanged(holder: SurfaceHolder, format: Int, width: Int, height: Int) {
                //holder.setFixedSize(1920, 1080);
            }

            override fun surfaceDestroyed(holder: SurfaceHolder) {
            }
        })

        binding.bt.setOnClickListener {
            FFPlayer().play()
        }
    }
    //fun checkPermission(): Boolean {
    //    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M && checkSelfPermission(
    //            Manifest.permission.DYNAMIC_RECEIVER_NOT_EXPORTED_PERMISSION
    //        ) != PackageManager.PERMISSION_GRANTED
    //    ) {
    //        requestPermissions(
    //            arrayOf(
    //                Manifest.permission.
    //            ), 1
    //        )
    //    }
    //    return false
    //}


    override fun onBackPressed() {
        super.onBackPressed()
        palyer?.release()
    }
}