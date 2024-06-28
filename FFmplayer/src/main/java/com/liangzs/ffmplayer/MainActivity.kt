package com.liangzs.ffmplayer

import android.Manifest
import android.content.pm.PackageManager
import android.os.Build
import android.os.Bundle
import android.util.Log
import androidx.activity.enableEdgeToEdge
import androidx.appcompat.app.AppCompatActivity
import com.liangzs.ffmplayer.databinding.ActivityMainBinding

class MainActivity : AppCompatActivity() {
    val mFFmplayer = FFmPlayer()
    lateinit var binding: ActivityMainBinding
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        enableEdgeToEdge()
        binding = ActivityMainBinding.inflate(layoutInflater)
        setContentView(binding.root)

        binding.btPlay.setOnClickListener {
            val path = "/sdcard/Music/不同流派歌曲【导入设备】/Casio Social Club - Count Your Lucky Stars (Remastered).mp3";
            //val path = "/sdcard/Music/中文歌曲/回忆那么伤 - 孙子涵.mp3";
            mFFmplayer.setDataSource(path)
            mFFmplayer.listener = object : FFmPlayer.OnPreparedListener {
                override fun onPrepared() {
                    Log.i(javaClass.simpleName, "mFFmplayer.start()")
                    mFFmplayer.start()
                }

                override fun onProgress(progress: Int, duration: Int) {
                    runOnUiThread {
                        binding.tvCurrent.text = progress.toString()
                        binding.tvDuration.text = duration.toString()
                    }

                }

            }
        }
        checkPermission()
    }

    fun checkPermission(): Boolean {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M && checkSelfPermission(
                Manifest.permission.READ_EXTERNAL_STORAGE,
            ) != PackageManager.PERMISSION_GRANTED
        ) {
            requestPermissions(
                arrayOf(
                    Manifest.permission.READ_EXTERNAL_STORAGE,
                    Manifest.permission.WRITE_EXTERNAL_STORAGE
                ), 1
            )
        }
        return false
    }
}