//
// Created by DELLQ on 24/6/2024.
//

#ifndef MEDIAPACKAGE_PLAYER_STATUS_H
#define MEDIAPACKAGE_PLAYER_STATUS_H

enum Status {
    IDLE = 0,
    PREPARED = 1,
    STARTED = 2,
    SEEKING = 3,
    PAUSED = 4,
    STOPPED = 5
};

#endif //MEDIAPACKAGE_PLAYER_STATUS_H
