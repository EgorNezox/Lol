#ifndef FIRMWARE_APP_MRD_CONTINIOUS_TIMER_H_
#define FIRMWARE_APP_MRD_CONTINIOUS_TIMER_H_

#include "qmtimer.h"

class ContTimer 
{
public:
    ContTimer( void* CallBack );
    //start(unsigned int interval);
    //stop();
    //reset();
    bool set_timer(unsigned int interval);
    //add_interval(unsigned int interval);
    unsigned int get_counter();
    unsigned int get_interval();
    bool get_timer_state();
    void start_timer(unsigned int interval);
    void stop_timer();
    void reset();
    unsigned int get_timer_counter();
    void add_interval(unsigned int interval);
private:
    bool RUN;
    QmTimer *HI_level_timer;
    unsigned long SYSTEM_TICK;
    unsigned long START_TICK;
    unsigned long LAST_INTERVAL;
    void (*callback)();
    void onTimerTimeOut();
};

#endif /* FIRMWARE_APP_MRD_CONTINIOUS_TIMER_H_ */
