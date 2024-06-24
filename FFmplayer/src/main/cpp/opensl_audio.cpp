//
// Created by DELLQ on 24/6/2024.
//

#include "opensl_audio.h"


void OpenSlAudio::initOpenSl() {
    SLresult sLresult;
    sLresult= slCreateEngine(&engine_object,0,0,0,0,0);


}

void OpenSlAudio::release() {

}

/**
 * cpp中的c方法，然后通过*data把this传递进去引用cpp的本身方法
 * @param data
 * @return
 */
void *openslDecode(void *data) {
    OpenSlAudio *audioPlayer = static_cast<OpenSlAudio *>(data);

    //是时候opengles
    audioPlayer->initOpenSl();

    pthread_exit(&audioPlayer->pthread_docode);
}

/**
 * 创建子线程进行解码
 */
void OpenSlAudio::start() {
    pthread_create(&pthread_docode, NULL, openslDecode, this);
}


