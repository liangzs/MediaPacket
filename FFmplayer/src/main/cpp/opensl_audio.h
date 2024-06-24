//
// Created by DELLQ on 24/6/2024.
//

#ifndef MEDIAPACKAGE_OPENSL_AUDIO_H
#define MEDIAPACKAGE_OPENSL_AUDIO_H

/**
 * 这里创建opensl的播放，先用ffmpeg进行解包，然后把数据丢到opensl中
 */

class OpenSlAudio {
public:
    int audio_strem_index = -1;

public:

    void initOpenSl();

    void release();

};


#endif //MEDIAPACKAGE_OPENSL_AUDIO_H
