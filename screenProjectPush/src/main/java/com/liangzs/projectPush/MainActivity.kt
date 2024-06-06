package com.liangzs.media

import android.Manifest
import android.content.Context
import android.content.Intent
import android.content.pm.PackageManager
import android.media.projection.MediaProjectionManager
import android.net.Uri
import android.os.Build
import android.os.Bundle
import androidx.appcompat.app.AppCompatActivity
import com.liangzs.screenprojection.R

class MainActivity : AppCompatActivity() {
    lateinit var mediaProject: MediaProjectionManager
    val PROJECT_RESULT = 1;
    val socketTransper = SocketTransper()
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)
        if (checkPermission()) {
            starSocketServer()
        }
    }

    fun starSocketServer() {
        mediaProject = getSystemService(Context.MEDIA_PROJECTION_SERVICE) as MediaProjectionManager
        val mediaProjectIntent = mediaProject.createScreenCaptureIntent();
        startActivityForResult(mediaProjectIntent, PROJECT_RESULT)
    }

    override fun onActivityResult(requestCode: Int, resultCode: Int, data: Intent?) {
        super.onActivityResult(requestCode, resultCode, data)
        when (requestCode) {
            PROJECT_RESULT -> {
                //开启录屏
                socketTransper.start()
            }
        }
    }


    fun checkPermission(): Boolean {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M && checkSelfPermission(
                Manifest.permission.WRITE_EXTERNAL_STORAGE
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


    override fun onDestroy() {
        super.onDestroy()
        socketTransper.onDestroy()
    }

}