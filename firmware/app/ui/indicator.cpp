/**
  ******************************************************************************
  * @file    indicator.cpp
  * @author  Egor Dudyak, PMR dept. software team, ONIIP, PJSC
  * @date    11 окт. 2013 г.
  * @brief   Реализация класса Indicator
  *
  ******************************************************************************
  */

#include "dialogs.h"
#include "all_sym_indicators.h"
#include "qmdebug.h"
#include "../headset/controller.h"

//----------DEFINES------------

#define ICON_SIZE	24
#define BATTERY_SIZE 16
#define GEOM_ICON(order) ((MoonsGeometry){order*ICON_SIZE,0,((order+1)*ICON_SIZE-1),ICON_SIZE-1})

//----------TYPES--------------

//----------GLOBAL_VARS--------

//----------PROTOTYPES---------

//----------CODE---------------

GUI_Indicator::GUI_Indicator(MoonsGeometry *area) : GUI_Obj(area)
{
	MoonsGeometry icon_geom;

    icon_geom = GEOM_ICON(0);
    ind_multiradio = new GUI_EL_Icon(&GUI_EL_TEMP_IconIndicator, &icon_geom, sym_blank, (GUI_Obj *)this);

    icon_geom = GEOM_ICON(1);
    ind_headset = new GUI_EL_Icon(&GUI_EL_TEMP_IconIndicator, &icon_geom, sym_blank, (GUI_Obj *)this);

    icon_geom = {ICON_SIZE*6,0,ICON_SIZE*6+BATTERY_SIZE-1, ICON_SIZE-1};	//геометрия батарейки
    ind_battery = new GUI_EL_Battery(&GUI_EL_TEMP_BatteryIndicator, 0, &icon_geom, (GUI_Obj *)this);
}

//-----------------------------

GUI_Indicator::~GUI_Indicator()
{
    delete ind_multiradio;
    delete ind_headset;
    delete ind_battery;

}

//-----------------------------

void GUI_Indicator::UpdateMultiradio(Multiradio::MainServiceInterface::Status status){
	switch(status){
		case Multiradio::MainServiceInterface::StatusIdle:
		case Multiradio::MainServiceInterface::StatusNotReady:
            ind_multiradio->icon = sym_blank;
			break;
		case Multiradio::MainServiceInterface::StatusVoiceRx:
            ind_multiradio->icon = sym_rx;
			break;
		case Multiradio::MainServiceInterface::StatusVoiceTx:
            ind_multiradio->icon = sym_tx;
			break;
		case Multiradio::MainServiceInterface::StatusTuningTx:
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
            ind_headset->icon = sym_headphones_smart;
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

void GUI_Indicator::Draw( Multiradio::MainServiceInterface::Status multiradioStatus,
                          Headset::Controller::Status              headsetStatus,
                          int                                      battaryStatus
                         ){
	gsetcolorb(GENERAL_BACK_COLOR);
	gsetvp(0,0,GDISPW-1, GDISPH-1);
	groundrect(ui_indicator_area.xs,ui_indicator_area.ys,ui_indicator_area.xe,ui_indicator_area.ye,0,GFILL);

    UpdateMultiradio(multiradioStatus /*service->pGetMultitradioService()->getStatus()*/);
    UpdateHeadset   (headsetStatus /*service->pGetHeadsetController()->getStatus()*/);
    UpdateBattery   (battaryStatus /*service->pGetPowerBattery()->getChargeLevel()*/);

    ind_battery->Draw();
    ind_headset->Draw();
    ind_multiradio->Draw();
}

void GUI_Indicator::Draw(){
    gsetcolorb(GENERAL_BACK_COLOR);
    gsetvp(0,0,GDISPW-1, GDISPH-1);
    groundrect(ui_indicator_area.xs,ui_indicator_area.ys,ui_indicator_area.xe,ui_indicator_area.ye,0,GFILL);

    ind_battery->Draw();
    ind_headset->Draw();
    ind_multiradio->Draw();
}
