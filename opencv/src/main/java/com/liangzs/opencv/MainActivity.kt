package com.liangzs.opencv

import android.hardware.Camera
import android.hardware.camera2.*
import android.os.Bundle
import android.view.SurfaceHolder
import android.view.SurfaceView
import androidx.appcompat.app.AppCompatActivity
import com.liangzs.opencv.databinding.ActivityMainBinding
import java.util.*

class MainActivity : AppCompatActivity() {
    val opencvJni = OpencvJni()
    private lateinit var binding: ActivityMainBinding
    private var camera: Camera? = null
    private var surfaceHolder: SurfaceHolder? = null
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        binding = ActivityMainBinding.inflate(layoutInflater)
        setContentView(binding.root)
        setSupportActionBar(binding.toolbar)
        binding.bt.setOnClickListener {
            startPreview();
        }
        binding.fab.setOnClickListener {
            opencvJni.init("/sdcard/Documents/lbpcascade_frontalface.xml")
        }
        surfaceHolder = binding.surfaceview.getHolder();
        surfaceHolder?.addCallback(object : SurfaceHolder.Callback {
            override fun surfaceCreated(holder: SurfaceHolder) {

            }

            override fun surfaceChanged(
                holder: SurfaceHolder,
                format: Int,
                width: Int,
                height: Int
            ) {
            }

            override fun surfaceDestroyed(holder: SurfaceHolder) {
                camera?.stopPreview();
                camera?.release();
                camera = null;
            }
        })

    }


    private fun startPreview() {
        camera = Camera.open(Camera.CameraInfo.CAMERA_FACING_FRONT);
        try {
            camera!!.setPreviewDisplay(surfaceHolder);
            camera!!.setDisplayOrientation(90);
            camera?.setPreviewCallback(object : Camera.PreviewCallback {
                override fun onPreviewFrame(data: ByteArray?, camera: Camera?) {
                    opencvJni.postData(data,camera!!.parameters.previewSize.width,
                        camera.parameters.previewSize.height,0)
                }
            })
            camera!!.startPreview();
        } catch (e: Exception) {
            e.printStackTrace();
        }

    }

    private fun closeCamera() {
        camera?.stopPreview();
        camera?.release();
        camera = null;
    }


    override fun onDestroy() {
        super.onDestroy()
        closeCamera()
    }
}