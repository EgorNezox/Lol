#include "dialogs.h"
#include "all_sym_indicators.h"
#include "qmdebug.h"
#include "../headset/controller.h"

#define ICON_SIZE	24
#define BATTERY_SIZE 16
#define GEOM_ICON(order) ((MoonsGeometry){order*ICON_SIZE,4,((order+1)*ICON_SIZE-1),ICON_SIZE-1})

GUI_Indicator::GUI_Indicator(MoonsGeometry *area) : GUI_Obj(area)
{
    MoonsGeometry icon_geom;

    uint8_t x_offset = area->xs;

    icon_geom = {68, 3, 68 + ICON_SIZE - 1, 3 + ICON_SIZE};
    ind_multiradio = new GUI_EL_Icon(&GUI_EL_TEMP_IconIndicator, &icon_geom, sym_blank, (GUI_Obj *)this); // Rx/Tx

    icon_geom = {44, 3, 44 + ICON_SIZE - 1, 3 + ICON_SIZE};
    ind_headset = new GUI_EL_Icon(&GUI_EL_TEMP_IconIndicator, &icon_geom, sym_blank, (GUI_Obj *)this);  //HeadSet

    icon_geom = {0, 35, 100, 60};
    LabelParams p_label = GUI_EL_TEMP_LabelMode;
    p_label.element.align = {alignHCenter, alignVCenter};
    date_time = new GUI_EL_Label(&p_label, &icon_geom, (char*)"", (GUI_Obj *)this);                    // Time

    icon_geom = {5, 36, 122, 62};

    icon_geom = {20, 4, 20 + ICON_SIZE-1, 28};
    gpsLabel = new GUI_EL_Icon(&GUI_EL_TEMP_CommonIcon, &icon_geom, sym_gps, (GUI_Obj *)this);        //GPS

    //x_offset += ICON_SIZE;
    icon_geom = {4, 2, 4 + ICON_SIZE - 1, 29};
    ind_battery = new GUI_EL_Battery(&GUI_EL_TEMP_BatteryIndicator, 75, &icon_geom, (GUI_Obj *)this);  // Battery

    synchXoffset = 105;
    icon_geom = {synchXoffset, 35, synchXoffset + 15, 60};
    LabelParams synch_label_params = GUI_EL_TEMP_LabelMode;                                             // sync
	gps_time_lbl = new GUI_EL_Label(&synch_label_params, &icon_geom, (char*)"G\0", (GUI_Obj *)this);

    x_offset += ICON_SIZE;
    synchXoffset = x_offset;
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
            ind_multiradio->icon = sym_rx_gray;
            break;
        case Multiradio::VoiceServiceInterface::StatusVoiceTx:
            ind_multiradio->icon = sym_tx_gray;
            break;
        case Multiradio::VoiceServiceInterface::StatusTuningTx:
            ind_multiradio->icon = sym_tx_gray;
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

void GUI_Indicator::UpdateHeadset(Headset::Controller::Status status)
{
    switch(status)
    {
        case Headset::Controller::StatusNone:
            ind_headset->icon = sym_headphones_none_gray;
            break;
        case Headset::Controller::StatusAnalog:
            ind_headset->icon = sym_headphones_analog_gray;
            break;
        case Headset::Controller::StatusSmartOk:
            ind_headset->icon = sym_headphones_smart_gray;
            break;
        case Headset::Controller::StatusSmartMalfunction:
            ind_headset->icon = sym_headphones_broken_gray;
            break;
        default:
            QM_ASSERT(0);
            break;
    }
}

void GUI_Indicator::UpdateSynchStatus(bool isSynch)
{
	this->isSynch = isSynch;
}

void GUI_Indicator::drawSynchSatus()
{
    synchXoffset = 105;
	if (isSynch)
	{
        gsetvp(0,0,127,127);
        MoonsGeometry icon_geom = {synchXoffset + 2, 40 + 1, synchXoffset + ICON_SIZE-1 + 2, 55 + 1};
		groundrect(icon_geom.xs - 5, icon_geom.ys, icon_geom.xe, icon_geom.ye, 0, GFILL);

        uint8_t radius = 9;
        uint8_t radius2 = 4;
        gcircle(icon_geom.xs + 4, icon_geom.ys + 8, radius, 0);

		gmoveto(icon_geom.xs + radius2, icon_geom.ys + radius);
		glineto(icon_geom.xs + radius2, icon_geom.ys);
		gmoveto(icon_geom.xs + radius2, icon_geom.ys + radius);
		glineto(icon_geom.xs + radius, icon_geom.ys + radius);

		if (isSynchFocus)
		{
			gsetvp(0,0,GDISPW-1,GDISPH-1);
			gsetcolorf(GENERAL_FORE_COLOR);
	   	    groundrect(icon_geom.xs - 10,icon_geom.ys - 4,icon_geom.xe - 5, icon_geom.ye + 5,0,GLINE);
		}
	}
	else
	{
        gsetvp(0,0,127,127);
        MoonsGeometry icon_geom = {synchXoffset, 40, synchXoffset + 15, 55};
		groundrect(icon_geom.xs - 8, icon_geom.ys - 4, icon_geom.xe - 3, icon_geom.ye + 2, 0, GFILL);
        gps_time_lbl->Draw();

		if (isSynchFocus)
		{
			gsetvp(0,0,GDISPW-1,GDISPH-1);
			gsetcolorf(GENERAL_FORE_COLOR);
	   	    groundrect(icon_geom.xs - 7,icon_geom.ys - 3,icon_geom.xe + 4,icon_geom.ye + 6,0,GLINE);
		}
	}


}

void GUI_Indicator::Draw()
{
    gsetcolorb(GENERAL_BACK_COLOR);
    gsetvp(0,0,GDISPW-1, GDISPH-1);
    groundrect(ui_indicator_area.xs,ui_indicator_area.ys,ui_indicator_area.xe,ui_indicator_area.ye,0,GFILL);

#if EMUL
    UpdateGpsStatus(2);
    UpdateBattery(50);
    UpdateHeadset(Headset::Controller::Status::StatusSmartOk);
    UpdateMultiradio(Multiradio::VoiceServiceInterface::Status::StatusVoiceTx);
    UpdateSynchStatus(false);
#endif

    ind_battery->Draw();
    ind_headset->Draw();
    ind_multiradio->Draw();
    date_time->Draw();
    gpsLabel->Draw();
    drawSynchSatus();
}

void GUI_Indicator::DrawTime()
{
	date_time->Draw();
}
