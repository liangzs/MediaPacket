package com.linagzs.video

import android.os.Bundle
import android.widget.TextView
import androidx.appcompat.app.AppCompatActivity

class MainActivity : AppCompatActivity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)
        val fFmpegUtil = FFmpegUtil();
        findViewById<TextView>(R.id.tv_murge).setOnClickListener {
            val input = "/sdcard/videos/Screenrecorder-2025-02-06-16-36-52-642.mp4"
            val output="/sdcard/Movies/output.mp4";
            fFmpegUtil.trim(input,output,2000,8000)
        }

//        checkPermission()
    }

//    fun checkPermission(): Boolean {
//        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M && checkSelfPermission(
//                Manifest.permission.DYNAMIC_RECEIVER_NOT_EXPORTED_PERMISSION
//            ) != PackageManager.PERMISSION_GRANTED
//        ) {
//            requestPermissions(
//                arrayOf(
//                    Manifest.permission
//                ), 1
//            )
//        }
//        return false
//    }
}