package com.liangzs.trim

import android.media.AudioFormat
import android.media.MediaCodec
import android.media.MediaCodec.BufferInfo
import android.media.MediaExtractor
import android.media.MediaFormat
import android.os.Environment
import java.io.File
import java.io.FileOutputStream
import java.nio.ByteBuffer

class MusicTrim {

    /**
     * 采用fileChannel 进行写出，filechannel可以直接输出byteBuffer数据
     *
     * 裁剪思路，编码数据->原始数据->裁剪->编码数据
     *
     * 读取数据用mediaextractor
     */
    fun trim(inputPath: String, trimStart: Long, trimEnd: Long, outPath: String, callback: Callback) {

        val mediaTractor = MediaExtractor()
        mediaTractor.setDataSource(inputPath)
        val audioTrackIndex = getAudioTrackIndex(mediaTractor)
        mediaTractor.selectTrack(audioTrackIndex)
        mediaTractor.seekTo(trimStart, MediaExtractor.SEEK_TO_CLOSEST_SYNC)

        val audioMediaFormat = mediaTractor.getTrackFormat(audioTrackIndex)
        //初始化编码
        val mediaDecoder = MediaCodec.createDecoderByType(audioMediaFormat.getString(MediaFormat.KEY_MIME)!!)
        mediaDecoder.configure(audioMediaFormat, null, null, 0);
        mediaDecoder.start()
        //extractor 的bytebuffer需要设置最大限制,需要判断是否存在
        var maxSize=10000;
        if(audioMediaFormat.containsKey(MediaFormat.KEY_MAX_INPUT_SIZE)){
            maxSize=audioMediaFormat.getInteger(MediaFormat.KEY_MAX_INPUT_SIZE)
        }

        val pcmFile = File(Environment.getExternalStorageDirectory(), "out.pcm")
        val writeChannel = FileOutputStream(pcmFile).channel

        var inputIndex=-1;
        var byteBuffer:ByteBuffer?;
        var tempByteArray:ByteArray;
        val exactorByteBuffer=ByteBuffer.allocate(maxSize)
        var bufferInfo=BufferInfo();
        while (true){
            if(mediaTractor.sampleTime==-1L||mediaTractor.sampleTime>trimEnd){//已经完成
                break
            }
            //如果seek不成功，继续seek
            if(mediaTractor.sampleTime<trimStart){
                mediaTractor.advance()
                continue
            }
            inputIndex=mediaDecoder.dequeueInputBuffer(10000);
            if(inputIndex>=0){

                //先从etractor读取数据
                bufferInfo.size=  mediaTractor.readSampleData(exactorByteBuffer,0)
                bufferInfo.presentationTimeUs=mediaTractor.sampleTime

                tempByteArray=ByteArray(exactorByteBuffer.remaining());
                exactorByteBuffer.get(tempByteArray);
                //获取index对应的存储映射对象，然后填充数据
                byteBuffer= mediaDecoder.getInputBuffer(inputIndex)
                byteBuffer?.clear()
                byteBuffer?.put(tempByteArray)
                //通知解码
                mediaDecoder.queueInputBuffer(inputIndex,bufferInfo.offset,bufferInfo.size,bufferInfo.presentationTimeUs,mediaTractor.sampleFlags)

                mediaTractor.advance()
            }

            //获取原始数据写出
            var outIndex=mediaDecoder.dequeueOutputBuffer(bufferInfo,1000);
            while (outIndex>=0){
               val outBuffer= mediaDecoder.getOutputBuffer(outIndex)
                //写出pcm
                writeChannel.write(outBuffer)
                mediaDecoder.releaseOutputBuffer(outIndex,false);
                outIndex=mediaDecoder.dequeueOutputBuffer(bufferInfo,1000);
            }
        }

        //写出后的pcm再转乘目标文件
        writeChannel.close()
        mediaTractor.release()
        mediaDecoder.stop()
        mediaDecoder.release()


        //        转换MP3    pcm数据转换成mp3封装格式
        val wavFile = File(Environment.getExternalStorageDirectory(), "output.mp3")
        PcmToWavUtil(44100, AudioFormat.CHANNEL_IN_STEREO,
            2, AudioFormat.ENCODING_PCM_16BIT).pcmToWav(pcmFile.absolutePath,
            wavFile.absolutePath)

    }

    fun getAudioTrackIndex(mediaExtractor: MediaExtractor): Int {
        for (i in 0 until mediaExtractor.trackCount) {
            mediaExtractor.getTrackFormat(i).getString(MediaFormat.KEY_MIME)
                ?.let {
                    if (it.startsWith("audio/")) {
                        return i;
                    }
                }
        }
        return -1;
    }


    interface Callback {
        fun start() {}
        fun progress() {}
        fun finish();
    }
}