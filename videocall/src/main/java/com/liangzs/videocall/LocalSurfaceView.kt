package com.liangzs.videocall

import android.content.Context
import android.hardware.Camera
import android.hardware.Camera.CameraInfo
import android.view.SurfaceView

class LocalSurfaceView(context: Context?) : SurfaceView(context) {

    val camera: Camera;

    init {
        camera = Camera.open(CameraInfo.CAMERA_FACING_BACK);
    }

}