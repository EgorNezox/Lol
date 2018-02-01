
#include "qm.h"
#define QMDEBUGDOMAIN	service_schedule
#include "qmdebug.h"
#include "dialogs.h"
#include "service.h"
#include "texts.h"
#include "../navigation/navigator.h"
#include "../../../system/reset.h"
#include "gui_tree.h"

namespace Ui {

void Service::startSchedulePromptTimer()
{
	isShowSchedulePrompt = true;
	schedulePromptRedrawTimer.setSingleShot(true);
	schedulePromptRedrawTimer.start(6000);
}

void Service::stopSchedulePromptTimer()
{
	isShowSchedulePrompt = false;
	draw();
}

void Service::showSchedulePrompt(DataStorage::FS::FileType fileType, uint16_t minutes)
{
    char min[5];
    sprintf((char*)&min, "%d", minutes);
    std::string text =
        std::string(min) +
        std::string(schedulePromptStr) +
        std::string(tmpParsing[fileType]);

    showMessage("",text.c_str(), promptArea);

    schedulePromptText = text;
    playSchedulePromptSignal();
    startSchedulePromptTimer();
}

// create list of sessions
// call on schedule changes
void Service::updateSessionTimeSchedule()
{
    uint8_t sessionCount = sheldure.size();

    if (sessionCount){

        uint8_t sessionTimeHour = 0;
        uint8_t sessionTimeMinute = 0;

        sessionList.clear();

        ScheduleTimeSession timeSession;
        for (uint8_t session = 0; session < sessionCount; session++){

            sessionTimeHour   = atoi(sheldure[session].time.substr(0,2).c_str());
            sessionTimeMinute = atoi(sheldure[session].time.substr(3,2).c_str());

            timeSession.index = session;
            timeSession.type = (DataStorage::FS::FileType)sheldure[session].type;
            timeSession.time = sessionTimeHour * 60 + sessionTimeMinute;

            uint8_t insertIndex = 0;

            if (sessionList.size() == 0)
                sessionList.push_back(timeSession);
            else
            {
                for (uint8_t sessionTime = 0; sessionTime < sessionList.size(); sessionTime++, insertIndex++){
                    if (timeSession.time < sessionList.at(sessionTime).time)
                        break;
                }
            	sessionList.insert(sessionList.begin() + insertIndex, timeSession);
            }
        }

        calcNextSessionIndex();
    } else
        schedulePromptTimer.stop();
}

void Service::calcNextSessionIndex()
{
    uint8_t curTimeHour = 0;
    uint8_t curTimeMinute = 0;
    uint8_t curTimeSecond = 0;

    getCurrentTime(&curTimeHour, &curTimeMinute, &curTimeSecond);

    uint16_t curTimeInMinutes = curTimeHour * 60 + curTimeMinute;

    for (uint8_t sessionTime = 0; sessionTime < sessionList.size(); sessionTime++){
        if (curTimeInMinutes > sessionList.at(sessionTime).time)
           continue;
        else
        {
           nextSessionIndex = sessionTime;
           break;
        }
    }
    onScheduleSessionTimer();
}

void Service::onScheduleSessionTimer()
{
    uint8_t curTimeHour = 0;
    uint8_t curTimeMinute = 0;
    uint8_t curTimeSecond = 0;

    getCurrentTime(&curTimeHour, &curTimeMinute, &curTimeSecond);

    uint16_t curTimeInMinutes = curTimeHour * 60 + curTimeMinute;

    uint16_t deltaTime = 0;
    if (nextSessionIndex == 0 && curTimeInMinutes > sessionList.at(0).time)
    	deltaTime = 24 * 60 - curTimeInMinutes + sessionList.at(nextSessionIndex).time; // timeTo0Hour + sessionTime
    else
    	deltaTime = sessionList.at(nextSessionIndex).time - curTimeInMinutes;

    if (deltaTime < 11){
        showSchedulePrompt(sessionList.at(nextSessionIndex).type, deltaTime);

        nextSessionIndex++;
        if (nextSessionIndex == sessionList.size())
           nextSessionIndex = 0; // cyclic
        if (sessionList.size() > 1){
            schedulePromptTimer.setInterval(2000);
            schedulePromptTimer.start();
        }
        return;
    }
    if (deltaTime <= 15){
        showSchedulePrompt(sessionList.at(nextSessionIndex).type, deltaTime);

        schedulePromptTimer.setInterval((deltaTime - 10)*60000);
        schedulePromptTimer.start();
        return;
    }
    schedulePromptTimer.setInterval((deltaTime - 15)*60000);
    schedulePromptTimer.start();
}

void Service::loadSheldure()
{
#ifndef _DEBUG_
   if (storageFs > 0){
       if (sheldureMass == 0)
          sheldureMass = new uint8_t[651];

       schedulePromptTimer.timeout.connect(sigc::mem_fun( this, &Service::onScheduleSessionTimer));
       schedulePromptRedrawTimer.timeout.connect(sigc::mem_fun( this, &Service::stopSchedulePromptTimer));

       if (storageFs->getSheldure(sheldureMass))
       {
         sheldureParsing(sheldureMass);
         if (isDspStarted)
            updateSessionTimeSchedule();
       }

       if (sheldureMass > 0)
       {
            delete []sheldureMass;
            sheldureMass = 0;
       }
   }
#else
    if (sheldureMass == 0)
       sheldureMass = new uint8_t[651];

    uint8_t massTemp[] =
    {0x32,'0', '1','0',':','3','2',':','0','0',0x00,0x44,0xec,0x88};

    for (uint8_t i = 0; i < 5; i++)
     memcpy(&sheldureMass[1+i*13], &massTemp[1], 13);
    sheldureMass[0] = 5;

    sheldureParsing(sheldureMass);

    if (sheldureMass > 0){
         delete []sheldureMass;
         sheldureMass = 0;
    }
#endif
    sheldureToStringList();
}

void Service::uploadSheldure()
{
#ifndef _DEBUG_
    if (storageFs > 0)
    {
        GUI_Painter::ClearViewPort(true);
        showMessage(waitingStr, flashProcessingStr, promptArea);

        if (sheldureMass == 0)
           sheldureMass = new uint8_t[1 + sheldure.size() * 13];

        sheldureUnparsing(sheldureMass);
        storageFs->setSheldure(sheldureMass, sheldure.size() * 13 + 1);

        draw();

        if (sheldureMass > 0){
             delete []sheldureMass;
             sheldureMass = 0;
        }
    }
    updateSessionTimeSchedule();
#endif
}

void Service::sheldureParsing(uint8_t* sMass)
{
    if (sMass[0] > 0 && sMass[0] <= 50)
    {
        uint8_t sheldureSize = sMass[0];

        for(uint8_t i = 0; i < sheldureSize; i++)
        {
            tempSheldureSession.clear();

            // --------- type ---------

            tempSheldureSession.type = (DataStorage::FS::FileType)(sMass[ 1 + (i * 13) ] - 48);

            // --------- time ---------

            for(uint8_t j = 0; j < 5; j++)
                tempSheldureSession.time.push_back(sMass[ 2 + (i*13) + j]);

            // --------- freq ---------

            uint32_t frec = 0;
            for(uint8_t k = 0; k < 4; k++)
              frec += (uint8_t)(sMass[ 10 + (i*13) + k]) << (3-k)*8;

            char ch[8];
            sprintf(ch,"%d",frec);
            for(uint8_t j = 0; j < 7; j++)
                tempSheldureSession.freq.push_back(ch[j]);

            sheldure.push_back(tempSheldureSession);
        }
    }
}

void Service::sheldureUnparsing(uint8_t* sMass)
{
    if (storageFs > 0){

        sMass[0] = sheldure.size();

        for (uint8_t session = 0; session < sMass[0]; session++)
        {
            // ---------- type ----------

            sMass[ 1 + (session * 13) ] = sheldure.at(session).type + 48;

            // ---------- time ----------

            std::string sec = ":00";
            memcpy(&sMass[ 1 + 1 + (session * 13)], &sheldure.at(session).time[0], 5);
            memcpy(&sMass[ 1 + 1 +(session * 13) + 5], &sec[0], 3);

            // ---------- freq ----------

            uint32_t freq = atoi(sheldure.at(session).freq.c_str());
            for(int i = 3; i >= 0; i--)
              sMass[1 + session * 13 + 9 + (3 - i)] = freq >> 8 * i;
        }
    }
}

void Service::sheldureToStringList()
{
    sheldure_data.clear();

    uint8_t sheldureSize = sheldure.size();

     if (sheldureSize > 0 && sheldureSize <= 50)
     {
         for (uint8_t session = 0; session < sheldureSize; session++)
         {
             std::string s;

             // --------- type -----------

             uint8_t typeMsg = (uint8_t)sheldure[session].type;
             s.append(tmpParsing[typeMsg]);
             (sheldure[session].type % 2 == 0) ? s.append("  ") : s.append("   ");

              // --------- time -----------

             s.append(sheldure[session].time).append("\n ");

             // --------- freq -----------

             s.append(sheldure[session].freq).append(freq_hz);

             sheldure_data.push_back(s);
         }
     }
     if(sheldureSize < 50)
        sheldure_data.push_back(addSheldure);
}

void Service::playSchedulePromptSignal()
{
	voice_service->playSoundSignal(4, 100, 100, 2, 200, 100);
}

}/* namespace Ui */

#include "qmdebug_domains_start.h"
QMDEBUG_DEFINE_DOMAIN(service_schedule, LevelDefault)
#include "qmdebug_domains_end.h"
