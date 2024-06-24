//
// Created by DELLQ on 24/6/2024.
//

#ifndef MEDIAPACKAGE_PLAYER_STATUS_H
#define MEDIAPACKAGE_PLAYER_STATUS_H

enum Status {
    IDLE = 0,
    PREPARED = 1,
    STARTED = 2,
    PAUSED = 3,
    STOPPED = 4
};

class PlayerStatus {
public:
    Status status;
};


#endif //MEDIAPACKAGE_PLAYER_STATUS_H
