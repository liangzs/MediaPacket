package com.liangzs.media

import android.hardware.display.DisplayManager
import android.hardware.display.VirtualDisplay
import android.media.MediaCodec
import android.media.MediaCodec.BufferInfo
import android.media.MediaFormat
import android.media.projection.MediaProjection
import android.media.projection.MediaProjectionManager
import java.nio.ByteBuffer

/**
 * 用h265编码录屏文件，然后通过socket进行传送
 */
class RecordByMediaCodec(val mediaProject: MediaProjection, val transper: SocketTransper) :Thread() {
    lateinit var mediaCodecH265: MediaCodec;
    var width: Int = 720;
    var height: Int = 1280;
    val virtualizer: VirtualDisplay;
    lateinit var vps_sps_pps_buffer:ByteArray;

    var socketTransper:SocketTransper?=null;
    companion object{
        const val NAL_VPS=32;
        const val NAL_I=19;
    }

    init {
        val mediaFormat =
            MediaFormat.createVideoFormat(MediaFormat.MIMETYPE_VIDEO_AVC, width, height);
        mediaFormat.setInteger(MediaFormat.KEY_BIT_RATE, width * height)
        mediaFormat.setInteger(MediaFormat.KEY_FRAME_RATE, 30);
        mediaFormat.setInteger(MediaFormat.KEY_I_FRAME_INTERVAL, 1)
        mediaCodecH265 = MediaCodec.createDecoderByType("video/avc")
        mediaCodecH265.configure(mediaFormat, null, null, MediaCodec.CONFIGURE_FLAG_ENCODE)
        mediaCodecH265.start()

        virtualizer = mediaProject.createVirtualDisplay(
            "dispaly",
            width,
            height,
            1,
            DisplayManager.VIRTUAL_DISPLAY_FLAG_PUBLIC,
            mediaCodecH265.createInputSurface(),
            null,
            null
        );
        socketTransper=SocketTransper()
        socketTransper?.start()
    }

    fun startDecode(){
        start()
    }

    override fun run() {
        val outBufferInfo=MediaCodec.BufferInfo();
        var outBufferIndex=0;
        var byteBuffer:ByteBuffer?;
        while (true){
            try {
                //解码
                outBufferIndex=mediaCodecH265.dequeueOutputBuffer(outBufferInfo,1000);
                if(outBufferIndex>=0){
                    byteBuffer=mediaCodecH265.getOutputBuffer(outBufferIndex);
                    byteBuffer?.let {
                        dealFrame(it,outBufferInfo)
                    }
                }
                mediaCodecH265.releaseOutputBuffer(outBufferIndex, false);

            }catch (e:Exception){
                e.printStackTrace()
                break
            }
        }
    }

    /**
     * h265 宏类型 用两个字节中的中间六位数值来定义的，所以要&0x7D
     */
    fun  dealFrame(byteBuffer: ByteBuffer,bufferinfo:BufferInfo){
        //00 00 00 01 ,00 00 01 两种分隔符
        var offset=4;
        if(byteBuffer.get(2).toInt() == 0x01){
            offset=3;
        }
        val type=byteBuffer.get(offset).toInt().and(0x7d).shr(1)
        //如果发送I帧的时候， 要拼接pps+vps+sps，直播也是采用这种模式
        if(type== NAL_VPS){//存储这个信息
            vps_sps_pps_buffer=ByteArray(bufferinfo.size);
            byteBuffer.get(vps_sps_pps_buffer);//
        }else if(type== NAL_I){
            val byte=ByteArray(bufferinfo.size);
            byteBuffer.get(byte);
            //拼接
            val newByteArray=ByteArray(vps_sps_pps_buffer.size+byte.size)
            System.arraycopy(vps_sps_pps_buffer,0,newByteArray,0,vps_sps_pps_buffer.size)
            System.arraycopy(byte,0,newByteArray,vps_sps_pps_buffer.size,byte.size);
            socketTransper?.sendData(newByteArray)
        }else{
            val byte=ByteArray(bufferinfo.size);
            byteBuffer.get(byte);
            socketTransper?.sendData(byte)
        }


    }
}