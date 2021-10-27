#ifndef SHELDURER_H
#define SHELDURER_H

#include "../datastorage/fs.h"

struct ScheduleTimeSession{
    uint8_t index; // in schedule
    DataStorage::FS::FileType type;
    uint16_t time; // in minutes hour*60+min
};

struct SheldureSession{
    std::string time = "00:00";
    DataStorage::FS::FileType type = DataStorage::FS::FT_SP;
    std::string freq = "1500000";

    void clear(){
        time = "";
        type = DataStorage::FS::FT_SP;
        freq = "";
    }

    void reInit(){
        time = "00:00";
        type = DataStorage::FS::FT_SP;
        freq = "10000";
    }

    void copyFrom(SheldureSession* source){
        time = source->time;
        type = source->type;
        freq = source->freq;
    }
};

typedef std::vector<SheldureSession> Sheldure;

#endif // SHELDURER_H
