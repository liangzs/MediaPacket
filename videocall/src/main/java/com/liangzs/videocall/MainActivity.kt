package com.liangzs.videocall

import android.app.Activity
import androidx.appcompat.app.AppCompatActivity
import android.os.Bundle
import android.widget.Button

class MainActivity : Activity() {
    var localSurfaceView: LocalSurfaceView? = null
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)
        localSurfaceView = findViewById<LocalSurfaceView>(R.id.surface_local)
    }

    fun onClick() {
        localSurfaceView?.startCall()
    }
}