#include "dialogs.h"
#include "all_sym_indicators.h"
#include "qmdebug.h"
#include "../headset/controller.h"

//----------DEFINES------------

#define ICON_SIZE	24
#define BATTERY_SIZE 16
#define GEOM_ICON(order) ((MoonsGeometry){order*ICON_SIZE,4,((order+1)*ICON_SIZE-1),ICON_SIZE-1})

#define PARAMS_DRAW 1

//----------TYPES--------------

//----------GLOBAL_VARS--------

//----------PROTOTYPES---------

//----------CODE---------------

GUI_Indicator::GUI_Indicator(MoonsGeometry *area) : GUI_Obj(area)
{
    MoonsGeometry icon_geom;

#if PARAMS_DRAW
    uint8_t x_offset = area->xs;
#else
    uint8_t x_offset = area->xs + 12 + 2 * ICON_SIZE;
#endif

    icon_geom = {x_offset, 6, x_offset + ICON_SIZE - 1, 23};
    ind_multiradio = new GUI_EL_Icon(&GUI_EL_TEMP_IconIndicator, &icon_geom, sym_blank, (GUI_Obj *)this);

    x_offset += ICON_SIZE;
    icon_geom = {x_offset, 6, x_offset + ICON_SIZE - 1, 32};
    ind_headset = new GUI_EL_Icon(&GUI_EL_TEMP_IconIndicator, &icon_geom, sym_blank, (GUI_Obj *)this);

//#if PARAMS_DRAW
//    icon_geom = {0, 35, 100, 60};
//#else
    icon_geom = {5, 40, 154, 66};
//#endif

    LabelParams p_label = GUI_EL_TEMP_LabelMode;
    p_label.element.align = {alignHCenter, alignVCenter};
    date_time = new GUI_EL_Label(&p_label, &icon_geom, (char*)"", (GUI_Obj *)this);

    x_offset += ICON_SIZE;
    icon_geom = {x_offset, 6, x_offset + ICON_SIZE-1, 32};
    gpsLabel = new GUI_EL_Icon(&GUI_EL_TEMP_CommonIcon, &icon_geom, sym_gps, (GUI_Obj *)this);

    x_offset += ICON_SIZE;
    icon_geom = {x_offset, 6, x_offset + ICON_SIZE-1, 29};
    ind_battery = new GUI_EL_Battery(&GUI_EL_TEMP_BatteryIndicator, 75, &icon_geom, (GUI_Obj *)this);
}

//-----------------------------

GUI_Indicator::~GUI_Indicator()
{
    delete date_time;
    delete ind_multiradio;
    delete ind_headset;
    delete ind_battery;
    delete gpsLabel;
}

//-----------------------------

void GUI_Indicator::UpdateMultiradio(Multiradio::VoiceServiceInterface::Status status){
    switch(status){
        case Multiradio::VoiceServiceInterface::StatusIdle:
        case Multiradio::VoiceServiceInterface::StatusNotReady:
            ind_multiradio->icon = sym_blank;
            break;
        case Multiradio::VoiceServiceInterface::StatusVoiceRx:
            ind_multiradio->icon = sym_rx;
            break;
        case Multiradio::VoiceServiceInterface::StatusVoiceTx:
            ind_multiradio->icon = sym_tx;
            break;
        case Multiradio::VoiceServiceInterface::StatusTuningTx:
            ind_multiradio->icon = sym_tx_tune;
            break;
        default:
            QM_ASSERT(0);
            break;
    }
}

void GUI_Indicator::UpdateBattery(int new_val){
    QM_ASSERT(new_val >= 0);
    ind_battery->charge = new_val;
}

void GUI_Indicator::UpdateGpsStatus(uint8_t status){
    switch( status ){
        case 0: gpsLabel->icon = sym_blank; break;
        case 1: gpsLabel->icon = sym_gps_unlock; break;
        case 2: gpsLabel->icon = sym_gps; break;
    }
}

//-----------------------------

void GUI_Indicator::UpdateHeadset(Headset::Controller::Status status){
    switch(status)
    {
        case Headset::Controller::StatusNone:
            ind_headset->icon = sym_headphones_none;
            break;
        case Headset::Controller::StatusAnalog:
            ind_headset->icon = sym_headphones_analog;
            break;
        case Headset::Controller::StatusSmartOk:
            ind_headset->icon = sym_headphones_analog;
            break;
        case Headset::Controller::StatusSmartMalfunction:
            ind_headset->icon = sym_headphones_broken;
            break;
        default:
            QM_ASSERT(0);
            break;
    }
}


//-----------------------------

void GUI_Indicator::Draw( Multiradio::VoiceServiceInterface::Status multiradioStatus,
                          Headset::Controller::Status              headsetStatus,
                          int                                      battaryStatus,
                          uint8_t                                     gpsStatus
                         ){
    gsetcolorb(GENERAL_BACK_COLOR);
    gsetvp(0,0,GDISPW-1, GDISPH-1);
    groundrect(ui_indicator_area.xs,ui_indicator_area.ys,ui_indicator_area.xe - 20 ,ui_indicator_area.ye,0,GFILL);

    UpdateMultiradio(multiradioStatus /*service->pGetMultitradioService()->getStatus()*/);
    UpdateHeadset   (headsetStatus /*service->pGetHeadsetController()->getStatus()*/);
    UpdateBattery   (battaryStatus /*service->pGetPowerBattery()->getChargeLevel()*/);
    UpdateGpsStatus (gpsStatus);

    ind_battery->Draw();
    ind_headset->Draw();
    ind_multiradio->Draw();
    date_time->Draw();
    gpsLabel->Draw();
}

void GUI_Indicator::Draw(){
    gsetcolorb(GENERAL_BACK_COLOR);
    gsetvp(0,0,GDISPW-1, GDISPH-1);
    groundrect(ui_indicator_area.xs,ui_indicator_area.ys,ui_indicator_area.xe-20,ui_indicator_area.ye,0,GFILL);

    ind_battery->Draw();
    ind_headset->Draw();
    ind_multiradio->Draw();
    date_time->Draw();
    gpsLabel->Draw();
}
