package com.liangzs.trim

import android.media.MediaCodec
import android.media.MediaExtractor
import android.media.MediaFormat
import java.io.File
import java.io.FileInputStream
import java.io.FileOutputStream
import java.nio.ByteBuffer
import kotlin.experimental.and
import kotlin.experimental.or


/**
 * 两个音频合并方案：先把音频文件转化成pcm，然后根据音频的音量设置，把两个pcm进行相加，
 * 判断是否为8位还是十六位的情况，十六位的是低位，高位这样的顺序进行存储
 *
 * 这就要求再转换成pcm的时候，采用相同的采用率和bit等才行，也就是说pcm的输出参数要统一
 *
 * 如果要对pcm进行seek，则动态通过采用率,位数进行计算
 * int seekByteOffset = seekTimeInSeconds * sampleRate * channels * bytesPerSample;
 */
class MusicMurge {

    fun musicsMurge(inputPath: String, inputPath2: String) {
        val fileInputStream1 = FileInputStream(File(inputPath))
        val fileInputStream2 = FileInputStream(File(inputPath2))
        val byteArray1 = ByteArray(1024);
        val byteArray2 = ByteArray(1024);
        val byteResult = ByteArray(1024);
        //定义一个音量
        val volume1 = 1f;
        val volume2 = 1f;

        var end1 = false
        var end2 = false;
        var temp1: Short
        var temp2: Short
        var tempResult: Int
        while (!end1 || !end2) {
            end1 = fileInputStream1.read(byteArray1) == -1
            end2 = fileInputStream2.read(byteArray2) == -1;
            if (end1 && !end2) {
                System.arraycopy(byteResult, 0, byteArray2, 0, byteArray2.size)
            }
            if (end2 && !end1) {
                System.arraycopy(byteResult, 0, byteArray1, 0, byteArray1.size)
            }
            if (!end1 && !end2) {
                //两个相加
                //前八位是低位,后八位是高位
                var i = 0;
                while (i < byteArray1.size && i < byteArray2.size) {//拼成16位，1byte是8bit，要两个byte
                    var hightBit = byteArray1.get(i + 1).and(0xff.toByte()).toInt().shl(8).toByte()
                    temp1 = byteArray1.get(i).and(0xff.toByte()).or(hightBit).toShort()
                    hightBit = byteArray2.get(i + 1).and(0xff.toByte()).toInt().shl(8).toByte()
                    temp2 = byteArray2.get(i).and(0xFF.toByte()).or(hightBit).toShort()

                    tempResult = (temp1 * volume1 + temp2 * volume2).toInt();
                    //检测是否超过阈值65535->32767   -32768
                    if (tempResult > 32767) tempResult = 32767
                    if (tempResult < -32768) tempResult = -32768;
                    //低位
                    byteResult.set(i, tempResult.toByte().and(0xFF.toByte()))
                    //高位
                    byteResult.set(i + 1, tempResult.ushr(8).toByte().and(0xFF.toByte()))
                    i += 2;
                }
            }
        }

    }

    fun turnMusic2Pcm(inputPath: String, trimStart: Long, trimEnd: Long, outPath: String) {

        val mediaTractor = MediaExtractor()
        mediaTractor.setDataSource(inputPath)
        val audioTrackIndex = getAudioTrackIndex(mediaTractor)
        mediaTractor.selectTrack(audioTrackIndex)
        mediaTractor.seekTo(trimStart, MediaExtractor.SEEK_TO_CLOSEST_SYNC)

        val audioMediaFormat = mediaTractor.getTrackFormat(audioTrackIndex)
        //初始化编码
        val mediaDecoder =
            MediaCodec.createDecoderByType(audioMediaFormat.getString(MediaFormat.KEY_MIME)!!)
        mediaDecoder.configure(audioMediaFormat, null, null, 0);
        mediaDecoder.start()
        //extractor 的bytebuffer需要设置最大限制,需要判断是否存在
        var maxSize = 10000;
        if (audioMediaFormat.containsKey(MediaFormat.KEY_MAX_INPUT_SIZE)) {
            maxSize = audioMediaFormat.getInteger(MediaFormat.KEY_MAX_INPUT_SIZE)
        }

        val pcmFile = File(outPath)
        val writeChannel = FileOutputStream(pcmFile).channel

        var inputIndex = -1;
        var byteBuffer: ByteBuffer?;
        var tempByteArray: ByteArray;
        val exactorByteBuffer = ByteBuffer.allocate(maxSize)
        var bufferInfo = MediaCodec.BufferInfo();
        while (true) {
            if (mediaTractor.sampleTime == -1L || mediaTractor.sampleTime > trimEnd) {//已经完成
                break
            }
            //如果seek不成功，继续seek
            if (mediaTractor.sampleTime < trimStart) {
                mediaTractor.advance()
                continue
            }
            inputIndex = mediaDecoder.dequeueInputBuffer(10000);
            if (inputIndex >= 0) {

                //先从etractor读取数据
                bufferInfo.size = mediaTractor.readSampleData(exactorByteBuffer, 0)
                bufferInfo.presentationTimeUs = mediaTractor.sampleTime

                tempByteArray = ByteArray(exactorByteBuffer.remaining());
                exactorByteBuffer.get(tempByteArray);
                //获取index对应的存储映射对象，然后填充数据
                byteBuffer = mediaDecoder.getInputBuffer(inputIndex)
                byteBuffer?.clear()
                byteBuffer?.put(tempByteArray)
                //通知解码
                mediaDecoder.queueInputBuffer(
                    inputIndex,
                    bufferInfo.offset,
                    bufferInfo.size,
                    bufferInfo.presentationTimeUs,
                    mediaTractor.sampleFlags
                )

                mediaTractor.advance()
            }

            //获取原始数据写出
            var outIndex = mediaDecoder.dequeueOutputBuffer(bufferInfo, 1000);
            while (outIndex >= 0) {
                val outBuffer = mediaDecoder.getOutputBuffer(outIndex)
                //写出pcm
                writeChannel.write(outBuffer)
                mediaDecoder.releaseOutputBuffer(outIndex, false);
                outIndex = mediaDecoder.dequeueOutputBuffer(bufferInfo, 1000);
            }
        }

        //写出后的pcm再转乘目标文件
        writeChannel.close()
        mediaTractor.release()
        mediaDecoder.stop()
        mediaDecoder.release()


        //        转换MP3    pcm数据转换成mp3封装格式
        //val wavFile = File(Environment.getExternalStorageDirectory(), "output.mp3")
        //PcmToWavUtil(44100, AudioFormat.CHANNEL_IN_STEREO,
        //    2, AudioFormat.ENCODING_PCM_16BIT).pcmToWav(pcmFile.absolutePath,
        //    wavFile.absolutePath)

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
}