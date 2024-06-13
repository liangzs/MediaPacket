package com.liangzs.videocall

import android.content.Context
import android.hardware.Camera
import android.hardware.Camera.CameraInfo
import android.hardware.Camera.PreviewCallback
import android.view.SurfaceHolder
import android.view.SurfaceView
import java.nio.ByteBuffer

/**
 * 摄像头采集的视频数据格式是N21和YV12，但是编码器MediaCodec处理的数据格式是Y420P和Y420SP的，
 * 所以这里需要做一次数据格式的转化，同样如果想采集摄像头的每一帧图片做处理的话，还需要把N21格式转化成RGB格式
 */
class LocalSurfaceView(context: Context?) : SurfaceView(context), SurfaceHolder.Callback {

    lateinit var camera: Camera;
    lateinit var cameraSize: Camera.Size;
    lateinit var byteArray: ByteArray;

    var socketH265: SocketH265;

    init {
        holder.addCallback(this)
        socketH265 = SocketH265()
    }

    override fun surfaceCreated(holder: SurfaceHolder) {

    }

    override fun surfaceChanged(holder: SurfaceHolder, format: Int, width: Int, height: Int) {
    }

    override fun surfaceDestroyed(holder: SurfaceHolder) {
    }

    fun startPreview() {
        camera = Camera.open(CameraInfo.CAMERA_FACING_BACK);
        val params = camera.parameters;
        cameraSize = params.previewSize
        camera.setPreviewDisplay(holder)

        //设置相机的回调
        camera.setPreviewCallback(object : PreviewCallback {
            override fun onPreviewFrame(data: ByteArray?, camera: Camera?) {
                //data是原始数据，一是用于surfaceview的显示，而是是remote进行h265进行传送
                socketH265.sendData(data)
                camera?.addCallbackBuffer(data)
            }
        })

        //设置帧的大小，因为是yuv，所以是3/2的w*h
        byteArray = ByteArray(cameraSize.width * cameraSize.height * 3 / 2)
        camera.addCallbackBuffer(byteArray)
        camera.startPreview()
    }

    fun startCall() {
        socketH265.start()
        //开始摄像
        startPreview()
    }

}