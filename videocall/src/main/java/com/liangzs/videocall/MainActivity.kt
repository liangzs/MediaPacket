package com.liangzs.videocall

import android.Manifest
import android.app.Activity
import android.content.pm.PackageManager
import android.os.Build
import androidx.appcompat.app.AppCompatActivity
import android.os.Bundle
import android.view.SurfaceHolder
import android.view.SurfaceHolder.Callback
import android.view.SurfaceView
import android.widget.Button

class MainActivity : Activity() {
    var localSurfaceView: LocalSurfaceView? = null
    var remoteSurfaceView: SurfaceView? = null;
    var h265DecoderFetch: H265DecoderFetch? = null;
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)
        localSurfaceView = findViewById<LocalSurfaceView>(R.id.surface_local)
        remoteSurfaceView = findViewById(R.id.surface_remote);
        remoteSurfaceView?.holder?.addCallback(object : Callback {
            override fun surfaceCreated(holder: SurfaceHolder) {
                h265DecoderFetch = H265DecoderFetch(holder.surface, 720, 1280)
            }

            override fun surfaceChanged(holder: SurfaceHolder, format: Int, width: Int, height: Int) {
            }

            override fun surfaceDestroyed(holder: SurfaceHolder) {
            }
        })
        checkPermission()

        findViewById<Button>(R.id.button).setOnClickListener {
            localSurfaceView?.startCall(remoteCallback)
        }
    }

    val remoteCallback = object : LocalSurfaceView.RemoteCallback {
        override fun onData(byteArray: ByteArray) {
            h265DecoderFetch?.decodeFrame(byteArray)
        }

    }


    fun checkPermission(): Boolean {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M && checkSelfPermission(
                Manifest.permission.CAMERA
            ) != PackageManager.PERMISSION_GRANTED
        ) {
            requestPermissions(
                arrayOf(
                    Manifest.permission.READ_EXTERNAL_STORAGE,
                    Manifest.permission.CAMERA
                ), 1
            )
        }
        return false
    }

}