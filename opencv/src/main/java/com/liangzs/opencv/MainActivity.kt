package com.liangzs.opencv

import android.Manifest
import android.content.Context
import android.content.pm.PackageManager
import android.graphics.SurfaceTexture
import android.hardware.camera2.*
import android.hardware.camera2.params.StreamConfigurationMap
import android.os.Bundle
import android.os.Handler
import android.os.HandlerThread
import android.util.Size
import android.view.Surface
import androidx.annotation.NonNull
import androidx.appcompat.app.AppCompatActivity
import androidx.core.app.ActivityCompat
import com.liangzs.opencv.databinding.ActivityMainBinding
import java.util.*

class MainActivity : AppCompatActivity() {
    val opencvJni = OpencvJni()
    private lateinit var binding: ActivityMainBinding

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        binding = ActivityMainBinding.inflate(layoutInflater)
        setContentView(binding.root)
        setSupportActionBar(binding.toolbar)
        mBackgroundThread = HandlerThread("CameraBackground");
        mBackgroundThread?.start();
        mBackgroundHandler = Handler(mBackgroundThread!!.getLooper());
        binding.bt.setOnClickListener {
            openCamera();
        }
        binding.fab.setOnClickListener {
            opencvJni.init()
        }
    }

    private var mCameraDevice: CameraDevice? = null
    private var mCaptureSession: CameraCaptureSession? = null
    private var mPreviewRequestBuilder: CaptureRequest.Builder? = null
    private var mPreviewRequest: CaptureRequest? = null
    private var mBackgroundHandler: Handler? = null
    private var mBackgroundThread: HandlerThread? = null

    fun openCamera() {
        val manager: CameraManager = getSystemService(Context.CAMERA_SERVICE) as CameraManager
        try {
            val cameraId: String = manager.getCameraIdList().get(0)
            val characteristics: CameraCharacteristics = manager.getCameraCharacteristics(cameraId)
            val map: StreamConfigurationMap =
                characteristics.get(CameraCharacteristics.SCALER_STREAM_CONFIGURATION_MAP)!!
            val sizes: Array<Size> = map.getOutputSizes(SurfaceTexture::class.java)
            var width = 640
            var height = 480
//            if (sizes != null && sizes.size > 0) {
//                width = sizes[0].getWidth()
//                height = sizes[0].getHeight()
//            }
            binding.textureView.getSurfaceTexture()?.setDefaultBufferSize(width, height)
            val surface = Surface(binding.textureView.getSurfaceTexture())
            mPreviewRequestBuilder =
                mCameraDevice?.createCaptureRequest(CameraDevice.TEMPLATE_PREVIEW)
            mPreviewRequestBuilder?.addTarget(surface)
            mCameraDevice?.createCaptureSession(
                Arrays.asList(surface),
                object : CameraCaptureSession.StateCallback() {
                    override fun onConfigured(@NonNull session: CameraCaptureSession) {
                        if (mCameraDevice == null) return
                        mCaptureSession = session
                        try {
                            mPreviewRequestBuilder?.set(
                                CaptureRequest.CONTROL_AF_MODE,
                                CaptureRequest.CONTROL_AF_MODE_CONTINUOUS_PICTURE
                            )
                            mPreviewRequest = mPreviewRequestBuilder?.build()
                            mCaptureSession?.setRepeatingRequest(
                                mPreviewRequest!!,
                                null,
                                mBackgroundHandler
                            )
                        } catch (e: CameraAccessException) {
                            e.printStackTrace()
                        }
                    }

                    override fun onConfigureFailed(session: CameraCaptureSession) {
                    }

                },
                mBackgroundHandler
            )
            if (ActivityCompat.checkSelfPermission(
                    this,
                    Manifest.permission.CAMERA
                ) != PackageManager.PERMISSION_GRANTED
            ) {
                // TODO: Consider calling
                //    ActivityCompat#requestPermissions
                // here to request the missing permissions, and then overriding
                //   public void onRequestPermissionsResult(int requestCode, String[] permissions,
                //                                          int[] grantResults)
                // to handle the case where the user grants the permission. See the documentation
                // for ActivityCompat#requestPermissions for more details.
                return
            }
            manager.openCamera(cameraId, object : CameraDevice.StateCallback() {
                override fun onOpened(@NonNull camera: CameraDevice) {
                    mCameraDevice = camera
                    startPreview()
                }

                override fun onDisconnected(@NonNull camera: CameraDevice) {
                    camera.close()
                }

                override fun onError(@NonNull camera: CameraDevice, error: Int) {
                    camera.close()
                    mCameraDevice = null
                }

            }, mBackgroundHandler)
        } catch (e: CameraAccessException) {
            e.printStackTrace()
        } catch (e: IllegalStateException) {
            e.printStackTrace()
        }
    }

    private fun startPreview() {
        if (mCameraDevice == null || mPreviewRequestBuilder == null) return
        val texture: SurfaceTexture = binding.textureView.getSurfaceTexture()!!
        texture.setDefaultBufferSize(
            binding.textureView.getWidth(),
            binding.textureView.getHeight()
        )
        val surface = Surface(texture)
        try {
            mPreviewRequestBuilder?.addTarget(surface)
            mCameraDevice?.createCaptureSession(
                Arrays.asList(surface),
                object : CameraCaptureSession.StateCallback() {
                    override fun onConfigured(@NonNull session: CameraCaptureSession) {
                        mCaptureSession = session
                        mPreviewRequestBuilder?.set(
                            CaptureRequest.CONTROL_AF_MODE,
                            CaptureRequest.CONTROL_AF_MODE_CONTINUOUS_PICTURE
                        )
                        mPreviewRequest = mPreviewRequestBuilder?.build()
                        mCaptureSession?.setRepeatingRequest(
                            mPreviewRequest!!,
                            null,
                            mBackgroundHandler
                        )
                    }

                    override fun onConfigureFailed(session: CameraCaptureSession) {
                    }

                },
                mBackgroundHandler
            )
        } catch (e: CameraAccessException) {
            e.printStackTrace()
        }
    }

    private fun closeCamera() {
        if (mCaptureSession != null) {
            mCaptureSession!!.close()
            mCaptureSession = null
        }
        if (mCameraDevice != null) {
            mCameraDevice!!.close()
            mCameraDevice = null
        }
        if (mBackgroundHandler != null) {
            mBackgroundHandler!!.removeCallbacksAndMessages(null)
            stopBackgroundThread()
        }
    }

    private fun stopBackgroundThread() {
        mBackgroundThread!!.quitSafely()
        try {
            mBackgroundThread!!.join()
            mBackgroundThread = null
            mBackgroundHandler = null
        } catch (e: InterruptedException) {
            e.printStackTrace()
        }
    }

    override fun onDestroy() {
        super.onDestroy()
        closeCamera()
        stopBackgroundThread()
    }
}