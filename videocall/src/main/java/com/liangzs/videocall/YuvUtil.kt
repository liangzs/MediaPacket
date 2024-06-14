package com.liangzs.videocall

class YuvUtil {
    companion object {
        fun nv21toNV12(nv21: ByteArray): ByteArray? {
            val size = nv21.size
            val nv12 = ByteArray(size)
            val len = size * 2 / 3
            System.arraycopy(nv21, 0, nv12, 0, len)
            var i = len
            while (i < size - 1) {
                nv12[i] = nv21[i + 1]
                nv12[i + 1] = nv21[i]
                i += 2
            }
            return nv12
        }
    }
}