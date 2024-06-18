package com.linagzs.video

import android.annotation.SuppressLint
import android.media.MediaCodec
import android.media.MediaCodec.BufferInfo
import android.media.MediaExtractor
import android.media.MediaFormat
import android.media.MediaMuxer
import java.io.File
import java.io.FileInputStream
import java.io.FileOutputStream
import java.nio.ByteBuffer
import kotlin.experimental.and
import kotlin.experimental.or

/**
 * 视频和音频混合
 * 1、把视频的音频数据和音乐音频数据都解码成pcm进行混音，然后再通过muxer进行写入视频数据和混合后的音频数据
 */
class VideoMixMusic {

    fun mix(videoPath: String, musicPath: String, outPath: String, videoVolume: Float, musicVolume: Float) {
        val videoPcm = File(videoPath + ".pcm").absolutePath
        val musicPcm = File(musicPath + ".pcm").absolutePath
        decode2Pcm(videoPath, videoPcm)
        decode2Pcm(musicPath, musicPcm)
        mixPcm(videoPcm, musicPcm, outPath, videoVolume, musicVolume)

        //音视频合并

    }

    /**
     * video 数据直接取extractor数据，然后直接给muxer，都是未压缩数据
     * audio则是通过合成后的pcm，然后需要重新编码成对应视频的audioformat数据，
     * 所以audio需要先解码，然后再编码输出
     */
    @SuppressLint("WrongConstant")
    fun muxerVideoMusic(videoPath: String, musicPath: String, outPath: String) {
        val videoExtractor = MediaExtractor()
        videoExtractor.setDataSource(videoPath)
        val videoTrackIndex = findVideoTrack(videoExtractor)
        videoExtractor.selectTrack(videoTrackIndex)
        val videoFormat = videoExtractor.getTrackFormat(videoTrackIndex);

        val mediaMuxer = MediaMuxer(outPath, MediaMuxer.OutputFormat.MUXER_OUTPUT_MPEG_4)
        //添加信道时，会返回该信道的索引，后续写入该数据时需要该索引,也就是说直接用mediaextractor和mexer直接合成的时候需要注意的该点事项
        val muxerVideoTrackIndex = mediaMuxer.addTrack(videoFormat)
        val maxSize = videoFormat.getInteger(MediaFormat.KEY_MAX_INPUT_SIZE)
        var byteBuffer = ByteBuffer.allocate(maxSize)
        val bufferInfo = BufferInfo()
        while (videoExtractor.sampleTime != -1L) {
            bufferInfo.presentationTimeUs = videoExtractor.sampleTime
            bufferInfo.flags = videoExtractor.sampleFlags
            //这里获取大小
            bufferInfo.size = videoExtractor.readSampleData(byteBuffer, 0)
            if (bufferInfo.size < 0) {
                break
            }
            mediaMuxer.writeSampleData(muxerVideoTrackIndex, byteBuffer, bufferInfo)
            videoExtractor.advance()
        }

        //读取音频
        //这里的音频编码需要去原视频的音频编码格式
        val audioOutputIndex = findVideoTrack(videoExtractor)
        val audioOutputFormat = videoExtractor.getTrackFormat(videoTrackIndex);


        val audioExtractor = MediaExtractor()
        audioExtractor.setDataSource(musicPath)
        val audioTrackIndex = findAudioTrack(audioExtractor)
        audioExtractor.selectTrack(audioTrackIndex)

        //音频要进行编码
        mediaMuxer.addTrack(audioOutputFormat)
        val musicMediaCodec = MediaCodec.createEncoderByType(audioOutputFormat.getString(MediaFormat.KEY_MIME)!!)
        musicMediaCodec.configure(audioOutputFormat, null, null, MediaCodec.CONFIGURE_FLAG_ENCODE)
        musicMediaCodec.start()
        var inputIndex = -1;
        var outputIndex: Int;

        var encodeDone = false;
        while (!encodeDone) {

            bufferInfo.presentationTimeUs = audioExtractor.sampleTime
            bufferInfo.flags = audioExtractor.sampleFlags
            bufferInfo.size = audioExtractor.readSampleData(byteBuffer, 0)
            if (bufferInfo.size < 0) {
                break
            }
            audioExtractor.advance()
            //编码
            inputIndex = musicMediaCodec.dequeueInputBuffer(10000);

            if (audioExtractor.sampleTime < 0) {
                musicMediaCodec.queueInputBuffer(inputIndex, 0, 0, 0, MediaCodec.BUFFER_FLAG_END_OF_STREAM)
                break
            }
            if (inputIndex >= 0) {
                val inputBuffer = musicMediaCodec.getInputBuffer(inputIndex);
                inputBuffer!!.clear()
                inputBuffer.put(byteBuffer)
                musicMediaCodec.queueInputBuffer(inputIndex, 0, bufferInfo.size, bufferInfo.presentationTimeUs, bufferInfo.flags)
            }
            outputIndex = musicMediaCodec.dequeueOutputBuffer(bufferInfo, 10000)
            while (outputIndex >= 0) {
                if (bufferInfo.flags == MediaCodec.BUFFER_FLAG_END_OF_STREAM) {
                    encodeDone = true
                    break
                }
                mediaMuxer.writeSampleData(audioOutputIndex, musicMediaCodec.getOutputBuffer(outputIndex)!!, bufferInfo)
                musicMediaCodec.releaseOutputBuffer(outputIndex, false);
                outputIndex = musicMediaCodec.dequeueOutputBuffer(bufferInfo, 10000)
            }
        }
        mediaMuxer.release()
        videoExtractor.release()
        audioExtractor.release()

    }

    /**
     * 两个pcm合成
     *
     *
     */
    private fun mixPcm(videoPcm: String, musicPcm: String, outPath: String, videoVolume: Float, musicVolume: Float) {
        val input1 = FileInputStream(videoPcm)
        val input2 = FileInputStream(musicPcm)
        val outputChannel = FileOutputStream(outPath)
        val byteArray1 = ByteArray(2048)
        val byteArray2 = ByteArray(2048)
        val byteResult = ByteArray(2048)
        var end1 = false;
        var end2 = false;


        var temp1: Short;
        var temp2: Short
        var tempResult: Int
        var hight: Byte;
        var low: Byte;
        while (!end1 || !end2) {

            end1 = input1.read(byteArray1) == -1
            end2 = input2.read(byteArray2) == -1

            if (!end1 && end2) {
                System.arraycopy(byteArray1, 0, byteResult, 0, byteArray1.size)
            }
            if (!end2 && end1) {
                System.arraycopy(byteArray2, 0, byteResult, 0, byteArray2.size)
            }
            if (!end1 && !end2) {//先低位，再高位的存储方式
                var i = 0;
                while (i < byteArray1.size) {
                    low = byteArray1[i].and(0xff.toByte())
                    hight = byteArray1.get(i + 1).toInt().shl(8).and(0xFF).toByte()
                    temp1 = hight.or(low).toShort()

                    low = byteArray2.get(i).and(0xff.toByte())
                    hight = byteArray2.get(i + 1).toInt().shl(8).and(0xFF).toByte()
                    temp2 = hight.or(low).toShort()
                    tempResult = (temp1 * videoVolume + temp2 * musicVolume).toInt();
                    if (tempResult > 32767) {
                        tempResult = 32767
                    } else if (tempResult < -32768) {
                        tempResult = -32768
                    }
                    byteResult[i] = tempResult.and(0xff).toByte()
                    byteResult.set(i + 1, tempResult.ushr(8).and(0xff).toByte())
                    outputChannel.write(byteResult)
                    i += 2;
                }
            }
        }

        input1.close()
        input2.close()
        outputChannel.close()
    }

    fun decode2Pcm(inputPath: String, pcmPath: String) {

        val mediaExtractor = MediaExtractor()
        mediaExtractor.setDataSource(inputPath);
        val audioTrackIndex = findAudioTrack(mediaExtractor)
        mediaExtractor.selectTrack(audioTrackIndex)
        val audioFormat = mediaExtractor.getTrackFormat(audioTrackIndex)

        //临时pcm文件
        val videoPcm = FileInputStream("video.pcm").channel

        val mediaCodecDecoder: MediaCodec = MediaCodec.createDecoderByType(audioFormat.getString(MediaFormat.KEY_MIME)!!)
        mediaCodecDecoder.configure(audioFormat, null, null, 0)
        mediaCodecDecoder.start()
        //开始解码
        while (true) {
            //结束
            if (mediaExtractor.sampleTime == -1L) {
                break
            }
            val inputIndex = mediaCodecDecoder.dequeueInputBuffer(10000);
            if (inputIndex >= 0) {
                val inputBuffer = mediaCodecDecoder.getInputBuffer(inputIndex);
                val sampleSize = mediaExtractor.readSampleData(inputBuffer!!, 0)
                if (sampleSize < 0) {
                    mediaCodecDecoder.queueInputBuffer(inputIndex, 0, 0, 0, MediaCodec.BUFFER_FLAG_END_OF_STREAM)
                } else {
                    mediaCodecDecoder.queueInputBuffer(inputIndex, 0, sampleSize, mediaExtractor.sampleTime, 0)
                    mediaExtractor.advance()
                }
            }

            //取出解码数据
            val bufferInfo = BufferInfo()
            var outputIndex = mediaCodecDecoder.dequeueOutputBuffer(bufferInfo, 10000)
            var outBuffer: ByteBuffer?
            while (outputIndex >= 0) {
                outBuffer = mediaCodecDecoder.getOutputBuffer(outputIndex)
                //写出文件
                videoPcm.write(outBuffer)
                mediaCodecDecoder.releaseOutputBuffer(inputIndex, false)
                outputIndex = mediaCodecDecoder.dequeueOutputBuffer(bufferInfo, 10000)
            }
        }
    }

    fun findAudioTrack(mediaExtractor: MediaExtractor): Int {
        for (i in 0 until mediaExtractor.trackCount) {
            val format = mediaExtractor.getTrackFormat(i)
            val mime = format.getString(MediaFormat.KEY_MIME)
            if (mime!!.startsWith("audio/")) {
                return i
            }
        }
        return -1
    }

    fun findVideoTrack(mediaExtractor: MediaExtractor): Int {
        for (i in 0 until mediaExtractor.trackCount) {
            val format = mediaExtractor.getTrackFormat(i)
            val mime = format.getString(MediaFormat.KEY_MIME)
            if (mime!!.startsWith("video/")) {
                return i
            }
        }
        return -1
    }
}