/**
 ******************************************************************************
 * @file    PswfModes.cpp
 * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
 * @date    25.09.2017
 *
 ******************************************************************************
 */

#include "qmendian.h"
#include <stdio.h>
#include "dspcontroller.h"
#include <vector>
#include "qmthread.h"
#include "qmdebug.h"
#include "dsptransport.h"
#include "qmtimer.h"

namespace Multiradio
{

void DspController::startGucTransmitting(int r_adr, int speed_tx, std::vector<int> command, bool isGps)
{
//    qmDebugMessage(QmDebug::Dump, "startGucTransmitting(%i, %i)", r_adr, speed_tx);
    QM_ASSERT(is_ready);

    ContentGuc.indicator = 20;
    ContentGuc.type = 1;
	ContentGuc.chip_time = 3; // super versia new, last value = 2
	ContentGuc.WIDTH_SIGNAL = 0; // last value  = 1, thi is freq mode 0 - 3k1, 1 - 20k maybe it works:)
    //data_storage_fs->getAleStationAddress(ContentGuc.S_ADR);
	ContentGuc.S_ADR = stationAddress;

    ContentGuc.R_ADR = r_adr;
    if (r_adr == 0)
    	isGucWaitReceipt = false;
    else
    	isGucWaitReceipt = true;

    uint8_t num_cmd = command.size();
    ContentGuc.NUM_com = num_cmd;

    isGpsGuc = isGps;

    for(int i = 0;i<num_cmd; i++)
    ContentGuc.command[i] = command[i];

    ContentGuc.ckk = 0;
    ContentGuc.ckk |= (1 & 0x01);
    ContentGuc.ckk |= (ContentGuc.WIDTH_SIGNAL & 0x01) << 1;
    ContentGuc.ckk |= (1 & 0x03) << 2;
    ContentGuc.ckk |= (ContentGuc.chip_time & 0x03) << 4;

    ContentGuc.Coord = 0;

    ContentGuc.stage =  GucTx;

    //initResetState();
    ParameterValue comandValue;
    comandValue.radio_mode = RadioModeOff;
    sendCommandEasy(RxRadiopath, RxRadioMode, comandValue);
    comandValue.guc_mode = RadioModeSazhenData;
    sendCommandEasy(TxRadiopath, TxRadioMode, comandValue);
    if (freqGucValue != 0)
    comandValue.frequency =  freqGucValue;
    sendCommandEasy(RxRadiopath, RxFrequency, comandValue);
    QmThread::msleep(100);
    sendCommandEasy(TxRadiopath, TxFrequency, comandValue);
    radio_state = radiostateGucTxPrepare;
    command.clear();

    sendGuc();
}

void DspController::startGucTransmitting()
{
 //   qmDebugMessage(QmDebug::Dump, "startGucTransmitting");
    QM_ASSERT(is_ready);

    ParameterValue comandValue;
    comandValue.radio_mode = RadioModeOff;
    sendCommandEasy(RxRadiopath, RxRadioMode, comandValue);

    comandValue.guc_mode = RadioModeSazhenData; // mode 11
    sendCommandEasy(TxRadiopath, TxRadioMode, comandValue);

    comandValue.frequency = freqGucValue;
    sendCommandEasy(RxRadiopath, RxFrequency, comandValue); //  � ·� °С‡� µ� ј � ·� ґ� µСЃСЊ � І� їСЂ� ё� ЅС� � ё� ї� µ ..
    sendCommandEasy(TxRadiopath, TxFrequency, comandValue);
    radio_state = radiostateGucTxPrepare;
}

void DspController::sendGuc()
{
    //qmDebugMessage(QmDebug::Dump, "sendGuc()");
    uint8_t tx_data[DspTransport::MAX_FRAME_DATA_SIZE];
    std::vector<bool> data_guc;
    uint8_t tx_address = 0x7A;
    int tx_data_len    = 0;
    ContentGuc.Coord   = isGpsGuc;
    uint8_t pack[5]    = {0, 0, 0, 0, 0};

    qmToBigEndian((uint8_t)ContentGuc.indicator, tx_data + tx_data_len); ++tx_data_len;
    qmToBigEndian((uint8_t)ContentGuc.type, tx_data + tx_data_len);      ++tx_data_len;

    pack[4]  = (ContentGuc.S_ADR   & 0x1F) << 3;
    pack[4] |= (ContentGuc.R_ADR   & 0x1F) >> 2;

    pack[3]  = (ContentGuc.R_ADR   & 0x03) << 6;
    pack[3] |= (ContentGuc.NUM_com & 0x7F) >> 1;

    pack[2]  = (ContentGuc.NUM_com & 0x01) << 7;
    pack[2] |= (ContentGuc.ckk     & 0x3F) << 1;
    pack[2] |=  ContentGuc.uin             >> 7;

    pack[1]  = (ContentGuc.uin     & 0x7F) << 1;
    pack[1] |=  ContentGuc.Coord   & 0x01;

    for(int i = 4; i >= 0; --i)
    {
    	qmToBigEndian((uint8_t)pack[i], tx_data + tx_data_len);
    	++tx_data_len;
    }

    int crc32_len = ContentGuc.NUM_com; // реальное количество команд
    int real_len  = crc32_len;

    // Выбор количества передаваемых байтов с координатами или без
    if (isGpsGuc)
    {
        if (ContentGuc.NUM_com  <= 6) 									ContentGuc.NUM_com = 6;
        if ((ContentGuc.NUM_com > 6 )  && (ContentGuc.NUM_com <= 10) )  ContentGuc.NUM_com = 10;
        if ((ContentGuc.NUM_com > 10)  && (ContentGuc.NUM_com <= 26) )  ContentGuc.NUM_com = 26;
        if ((ContentGuc.NUM_com > 26)  && (ContentGuc.NUM_com <= 100))  ContentGuc.NUM_com = 100;
    }
    else
    {
        if (ContentGuc.NUM_com <= 5) 								   ContentGuc.NUM_com = 5;
        if ((ContentGuc.NUM_com > 5)  && (ContentGuc.NUM_com <= 11) )  ContentGuc.NUM_com = 11;
        if ((ContentGuc.NUM_com > 11) && (ContentGuc.NUM_com <= 25) )  ContentGuc.NUM_com = 25;
        if ((ContentGuc.NUM_com > 25) && (ContentGuc.NUM_com <= 100))  ContentGuc.NUM_com = 100;
    }

    for(int i = 0; i < ContentGuc.NUM_com; i++)
    {
        qmToBigEndian((uint8_t)ContentGuc.command[i], tx_data + tx_data_len);
        ++tx_data_len;
    }
    // обработка и получение координат, добавление в исходный массив для защиты  crc32 сумой (ДАННЫЕ + КООДИНАТЫ)
    if (isGpsGuc)
    {
       uint8_t coord[9] = {0,0,0,0,0,0,0,0,0};
       getGpsGucCoordinat(coord);
       for(int i = 0; i < 9; i++)
       {
           ContentGuc.command[ContentGuc.NUM_com+i] = coord[i];
           qmToBigEndian((uint8_t)coord[i],tx_data + tx_data_len);
           ++tx_data_len;
       }
    }
    // выбор длинны кодируемого массива
     crc32_len = (isGpsGuc) ? (ContentGuc.NUM_com + 9) : (ContentGuc.NUM_com);

     if (isGpsGuc)
     {
    	 uint8_t mas[9]; int index = 0;
    	 for(int i = crc32_len - 9; i< crc32_len;i++)
    	 {
    		 mas[index] = ContentGuc.command[i];
    		 ++index;
    		 ContentGuc.command[i] = 0;
    	 }

    	 uint8_t value[120];
    	 for(int i = 0; i< ContentGuc.NUM_com;i++) value[i] = ContentGuc.command[i];
    	 for(int i = 9;i < 9+ContentGuc.NUM_com;i++)
    	 {
    		  ContentGuc.command[i] = value[i- 9];
    	 }
         for(int i = 0; i<9;i++)
         {
             ContentGuc.command[i] = mas[i];
             if (i != 8) pack_manager->addBytetoBitsArray(ContentGuc.command[i],data_guc,8);
         }

         bool quadrant = ContentGuc.command[8] & 1;
         data_guc.push_back(quadrant);

         quadrant = ContentGuc.command[8] & (1 << 1);
         data_guc.push_back(quadrant);

         for(int i = 0; i<real_len;i++)
         {
             pack_manager->addBytetoBitsArray(ContentGuc.command[i+9],data_guc,7);
         }
     }
     // сдвиг массива для crc32-суммы
    if (isGpsGuc)
    {
        pack_manager->getArrayByteFromBit(data_guc,ContentGuc.command);
        crc32_len = data_guc.size() / 8;

//        for(int i = 0; i< crc32_len;i++){
//        	qmDebugMessage(QmDebug::Dump,"packet guc: %d", ContentGuc.command[i]);
//        }
    }
    else
    {
    	std::vector<bool> data;
    	for(int i = 0; i<ContentGuc.NUM_com;i++) pack_manager->addBytetoBitsArray(ContentGuc.command[i],data,7);
    	for(int i = 0; i<crc32_len;i++) pack_manager->getArrayByteFromBit(data,ContentGuc.command);
    }

    if (!isGpsGuc)
    {
    	crc32_len = ((real_len*7)/8); uint8_t ost =  (real_len*7)% 8;
    	if (ost !=0)
    	{
    		uint8_t mask = 0;
    		for(int i = 0; i<ost;i++) mask +=  1 << (7 - i);
    		crc32_len +=1;
    		ContentGuc.command[crc32_len-1] = ContentGuc.command[crc32_len-1] & mask;
    	}
    }
    // добавление crc32 к пакету данных
     uint32_t crc = pack_manager->CRC32(ContentGuc.command, crc32_len);
     qmToBigEndian((uint32_t)crc, tx_data + tx_data_len);
     tx_data_len += 4;

    transport->transmitFrame(tx_address, tx_data, tx_data_len);
}

void DspController::sendGucQuit()
{
//	qmDebugMessage(QmDebug::Dump, "sendGucQuit");

	uint8_t tx_address = 0x7A;
	uint8_t tx_data[DspTransport::MAX_FRAME_DATA_SIZE];
	int tx_data_len = 0;

	ContentGuc.indicator    = 20;
	ContentGuc.type         = 4;
	ContentGuc.chip_time    = 3; // super versia new, last value = 2
	ContentGuc.WIDTH_SIGNAL = 0; // last value  = 1, thi is freq mode 0 - 3k1, 1 - 20k maybe it works:)
	//data_storage_fs->getAleStationAddress(ContentGuc.S_ADR);

	ContentGuc.ckk = 0;
	ContentGuc.ckk |= (1 & 0x01);
	ContentGuc.ckk |= (ContentGuc.WIDTH_SIGNAL & 0x01) << 1;
	ContentGuc.ckk |= (1 & 0x03) << 2;
	ContentGuc.ckk |= (ContentGuc.chip_time & 0x03) << 4;

	//ContentGuc.uin = 0;

	qmToBigEndian((uint8_t)ContentGuc.indicator, tx_data + tx_data_len);
	++tx_data_len;
	qmToBigEndian((uint8_t)ContentGuc.type, tx_data + tx_data_len);
	++tx_data_len;


	uint8_t pack[3] = {0, 0, 0};
	pack[2] =  (ContentGuc.R_ADR & 0x1F) << 3; // 5 � � В±� � С‘� ЎвЂљ
	pack[2] |= (ContentGuc.S_ADR & 0x1F) >> 2; // 3 � � В±� � С‘� ЎвЂљ� � В°
	pack[1] |= (ContentGuc.S_ADR & 0x1F) << 6; // 2 � � В±� � С‘� ЎвЂљ� � В°
	pack[1] |= (ContentGuc.uin >> 2) & 0x3F;   // 6 � � В±� � С‘� ЎвЂљ
	pack[0] =  (ContentGuc.uin << 6) & 0xC0;   // 2 � � В±� � С‘� ЎвЂљ� � В°

    for(int i = 2; i >= 0; --i) {
    	qmToBigEndian((uint8_t)pack[i], tx_data + tx_data_len);
    	++tx_data_len;
    }

    transport->transmitFrame(tx_address, tx_data, tx_data_len);
}

uint8_t *DspController::getGpsGucCoordinat(uint8_t *coord)
{
	Navigation::Coord_Date date = navigator->getCoordDate();
	std::string lon((const char*)date.longitude);
	std::string lat((const char*)date.latitude);

	coord[0] = (uint8_t)atoi(lat.substr(0,2).c_str());
	coord[1] = (uint8_t)atoi(lat.substr(2,2).c_str());
	coord[2] = (uint8_t)atoi(lat.substr(5,2).c_str());
	coord[3] = (uint8_t)atoi(lat.substr(7,2).c_str());
	coord[4] = (uint8_t)atoi(lon.substr(0,3).c_str());
	coord[5] = (uint8_t)atoi(lon.substr(3,2).c_str());
	coord[6] = (uint8_t)atoi(lon.substr(6,2).c_str());
	coord[7] = (uint8_t)atoi(lon.substr(8,2).c_str());

	if ((strstr((const char*)date.latitude,"N") !=0) && strstr((const char*)date.longitude,"E") !=0)
		coord[8] = 0;
	if ((strstr((const char*)date.latitude,"S") !=0) && strstr((const char*)date.longitude,"E") !=0)
		coord[8] = 1;
	if ((strstr((const char*)date.latitude,"S") !=0) && strstr((const char*)date.longitude,"W") !=0)
		coord[8] = 2;
	if ((strstr((const char*)date.latitude,"N") !=0) && strstr((const char*)date.longitude,"W") !=0)
		coord[8] = 3;

//	for(int i = 0; i< 9;i++) coord[i] = i+1;
//	coord[8] = 0xc0;
    return coord;
}

uint8_t* DspController::getGucCoord()
{
     return guc_coord;
}

void DspController::startGucTimer()
{
	guc_rx_quit_timer_counter = 180;
	qwitCounterChanged(guc_rx_quit_timer_counter);
	guc_rx_quit_timer->start();
}

void DspController::stopGucTimer()
{
	guc_rx_quit_timer->stop();
	guc_rx_quit_timer_counter = 180;
	//qwitCounterChanged(guc_rx_quit_timer_counter);
}

void DspController::onGucWaitingQuitTimeout()
{
	if (guc_rx_quit_timer_counter)
	{
		guc_rx_quit_timer->start();
		guc_rx_quit_timer_counter--;
		qwitCounterChanged(guc_rx_quit_timer_counter);
	}
	else
	{
		recievedGucQuitForTransm(-1);
		completedStationMode(false);
	}
}

void DspController::startGucRecieving()
{
    QM_ASSERT(is_ready);

    initResetState();

    ParameterValue comandValue;
    comandValue.radio_mode = RadioModeOff;
    sendCommandEasy(TxRadiopath, TxRadioMode, comandValue);
    comandValue.guc_mode = 3;

    sendCommandEasy(RadioLineNotPswf, 0 ,comandValue);
    comandValue.guc_mode = stationAddress;
    sendCommandEasy(RadioLineNotPswf, 3 ,comandValue);

    comandValue.guc_mode = 3;
    sendCommandEasy(RadioLineNotPswf, 1, comandValue);
    QmThread::msleep(100);
    //-----------------------------------

    comandValue.guc_mode = RadioModeSazhenData; // 11 mode
    sendCommandEasy(RxRadiopath, RxRadioMode, comandValue);

    comandValue.frequency = freqGucValue;
    sendCommandEasy(RxRadiopath, RxFrequency, comandValue);

    ContentGuc.stage =  GucRx;
    guc_vector.clear();
}

uint8_t* DspController::get_guc_vector()
{
	int num = (guc_vector.at(0).at(3) & 0x3f) << 1;
	num +=    (guc_vector.at(0).at(4) & 0x80) >> 7;

	//� � С—� � С•� � В»� ЎС“� ЎвЂЎ� � Вµ� � � …� � С‘� � Вµ � � С”� � С•� � В»� � С‘� ЎвЂЎ� � Вµ� Ў� ѓ� ЎвЂљ� � � � � � В° � Ў� Њ� � В»� � Вµ� � С�� � Вµ� � � …� ЎвЂљ� � С•� � � �  � � � �  � � � � � � Вµ� � С”� ЎвЂљ� � С•� Ў� ‚� � Вµ
	guc_text[0] = num;

    uint8_t out[120];
    for(int i = 0; i<120;i++) out[i] = 0;
    int crc_coord_len = 0;

    // � � Вµ� Ў� ѓ� � В»� � С‘ � Ў� ѓ � � С”� � С•� � С•� Ў� ‚� � Т‘� � С‘� � � …� � В°� ЎвЂљ� � В°� � С�� � С‘, � ЎвЂљ� � С• � � � � � ЎвЂ№� � В±� � С•� Ў� ‚� � С”� � В° � � С—� � С• � � С•� � Т‘� � � …� � С•� � С�� ЎС“ � � В°� � В»� � С–� � С•� Ў� ‚� � С‘� ЎвЂљ� � С�� ЎС“, � � С‘� � � …� � В°� ЎвЂЎ� � Вµ � � С—� � С• � � Т‘� Ў� ‚� ЎС“� � С–� � С•� � С�� ЎС“
	int count = 0;
    if (isGpsGuc == 0)
    {
        if (num <= 5) count = 5;
        if ((num > 5) && (num <= 11))   count = 11;
        if ((num > 11) && (num <= 25))  count = 25;
        if ((num > 25) && (num <= 100)) count = 100;
    }
    else
    {
        if (num <= 6) count = 6;
        if ((num > 6) && (num <= 10))   count = 10;
        if ((num > 10) && (num <= 26))  count = 26;
        if ((num > 26) && (num <= 100)) count = 100;
    }

    if (isGpsGuc)
    {
        for (int i = 0; i< 9;i++)
        {
            guc_text[i+1] = guc_vector.at(0).at(7+i+count);
        }
        // -- � � вЂ”� � В°� � С—� � С‘� Ў� ѓ� � В°� � В»� � С‘ � � С”� � С•� � С•� Ў� ‚� � Т‘� � С‘� � � …� � В°� ЎвЂљ� ЎвЂ№, � � � …� � В°� ЎвЂЎ� � С‘� � � …� � В°� Ў� Џ � Ў� ѓ � � С—� � Вµ� Ў� ‚� � � � � � С•� � в„– � � С—� � С•� � В·� � С‘� ЎвЂ� � � С‘� � С‘ � � С�� � В°� Ў� ѓ� Ў� ѓ� � С‘� � � � � � В° guc_text
        for(int i = 0; i< count;i++){
        	if (i < num)
        		guc_text[9 + i+1] = guc_vector.at(0).at(7+i);
        	else
        		guc_text[9 + i+1] = 0;
        }
        // -- � � вЂ”� � В°� � С—� � С‘c� � В°� � В»� � С‘ � � Т‘� � В°� � � …� � � …� ЎвЂ№� � Вµ, � � � …� � В°� ЎвЂЎ� � С‘� � � …� � В°� Ў� Џ � Ў� ѓ 10-� � в„– � � С—� � С•� � В·� � С‘� ЎвЂ� � � С‘� � С‘ � � � …� � В°� Ўв‚¬� � Вµ� � С–� � С• � � С�� � В°� Ў� ѓ� Ў� ѓ� � С‘� � � � � � В°
        std::vector<bool> data;
        for(int i = 0; i< 8; i++) pack_manager->addBytetoBitsArray(guc_text[i+1],data,8);
        // � � Т‘� � С•� � В±� � В°� � � � � � С‘� � В»� � С‘ � � С”� � С•� � С•� Ў� ‚� � Т‘� � С‘� � � …� � В°� ЎвЂљ� ЎвЂ№ � � С” � � В±� � С‘� ЎвЂљ� � С•� � � � � � С•� � С�� ЎС“ � � � � � � Вµ� � С”� ЎвЂљ� � С•� Ў� ‚� ЎС“

        bool quadrant = guc_text[9] & (1 << 7);
        data.push_back(quadrant);
        quadrant = guc_text[9] & (1 >> 6);
        data.push_back(quadrant);
        // � � Т‘� � С•� � В±� � В°� � � � � � С‘� � В»� � С‘ � � С” � � В±� � С‘� ЎвЂљ� � С•� � � � � � С•� � С�� ЎС“ � � � � � � Вµ� � С”� ЎвЂљ� � С•� Ў� ‚� ЎС“ � � С”� � � � � � В°� � Т‘� Ў� ‚� � В°� � � …� ЎвЂљ
        for(int i = 0; i<num;i++) pack_manager->addBytetoBitsArray(guc_text[9 + i+1],data,7);
        // � � Т‘� � С•� � В±� � В°� � � � � � С‘� � В»� � С‘ � � С” � � В±� � С‘� ЎвЂљ� � С•� � � � � � С•� � С�� ЎС“ � � � � � � Вµ� � С”� ЎвЂљ� � С•� Ў� ‚� ЎС“ � � Т‘� � В°� � � …� � � …� ЎвЂ№� � Вµ � � С—� � С• 7 � � В±� � С‘� ЎвЂљ
        pack_manager->getArrayByteFromBit(data,out);
        // � � В·� � В°� � С—� � С‘� Ў� ѓ� � В°� � В»� � С‘ � � � �  � � � � � ЎвЂ№� ЎвЂ¦� � С•� � Т‘� � � …� � С•� � в„– � � С�� � В°� Ў� ѓ� Ў� ѓ� � С‘� � � �  � � С—� Ў� ‚� � Вµ� � С•� � В±� Ў� ‚� � В°� � В·� � С•� � � � � � В°� � � …� � � …� ЎвЂ№� � Вµ � � Т‘� � В°� � � …� � � …� ЎвЂ№� � Вµ � � С‘� � В· � � В±� � С‘� ЎвЂљ� � С•� � � � � � С•� � С–� � С• � � С�� � В°� Ў� ѓ� Ў� ѓ� � С‘� � � � � � В° � � С—� � С• � � В°� � � …� � С•� � В»� � С•� � С–� � С‘� � С‘ � Ў� ѓ � ЎвЂћ� � С•� Ў� ‚� � С�� � С‘� Ў� ‚� � С•� � � � � � В°� � � …� � С‘� � Вµ � � С—� � В°� � С”� � Вµ� ЎвЂљ� � В° � � Т‘� � В»� Ў� Џ CRC32 � � � …� � В° � � С—� � Вµ� Ў� ‚� � Вµ� � Т‘� � В°� ЎвЂЎ� � Вµ
        crc_coord_len = data.size() / 8;
        // � � С—� � С•� � В»� ЎС“� ЎвЂЎ� � С‘� � В»� � С‘ � � Т‘� � В»� � С‘� � � …� � � …� ЎС“ � � С—� � В°� � С”� � Вµ� ЎвЂљ� � В°

        uint8_t cord[9];

        for(int i = 1; i<9;i++) cord[i] = guc_text[i];
        for(int i = 0;i<num;i++) guc_text[i+1] = guc_text[9+i+1];
        for(int i = 1;i<9; i++)  guc_text[num+i] = cord[i];
    }
    else
    {
    	std::vector<bool> data;
        for(int i = 0; i<num;i++) pack_manager->addBytetoBitsArray(guc_vector.at(0).at(7+i),data,7);
        for(int i = 0; i<count;i++) pack_manager->getArrayByteFromBit(data,out);
        guc_text[0] = num;
        for(int i = 0; i<num;i++) guc_text[i+1] = guc_vector.at(0).at(7+i);
    }

	// � � Т‘� � С•� Ў� ѓ� ЎвЂљ� � В°� � Вµ� � С� crc32 � Ў� ѓ� ЎС“� � С�� � С�� ЎС“ � � С‘� � В· � � С”� � С•� � � …� ЎвЂ� � � В° � � С—� � В°� � С”� � Вµ� ЎвЂљ� � В°
	int m = 3;
	uint32_t crc_packet = 0;
	int l = 0;
	while(m >=0){
		uint8_t sum = guc_vector.at(0).at(guc_vector.at(0).size() - 1 - m);
		crc_packet += sum << (8*m);
		l++;
		m--;
	}

	// � Ў� ѓ� ЎвЂЎ� � С‘� ЎвЂљ� � В°� � Вµ� � С� crc32 � Ў� ѓ� ЎС“� � С�� � С�� ЎС“
    uint32_t crc = 0;
    int value  = (isGpsGuc) ? crc_coord_len : num;
    // � � � � � ЎвЂ№� � В±� Ў� ‚� � В°� � В»� � С‘ � � Т‘� � В»� � С‘� � � …� � � …� ЎС“, � � С‘� Ў� ѓ� ЎвЂ¦� � С•� � Т‘� Ў� Џ � � С‘� � В· � Ў� ‚� � Вµ� � В¶� � С‘� � С�� � В° � � С—� � Вµ� Ў� ‚� � Вµ� � Т‘� � В°� ЎвЂЎ� � С‘

    if (!isGpsGuc) {value = ((num*7)/8); uint8_t ost = (num*7)% 8;

    if (ost !=0)
        	{
        		uint8_t mask = 0;
        		for(int i = 0; i<ost;i++) mask +=  1 << (7 - i);
        		value +=1;
        		out[value-1] = out[value-1] & mask;
        	}
    }

    crc = pack_manager->CRC32(out,value);

    if (crc != crc_packet)
    {
        gucCrcFailed();
        guc_text[0] = '\0';
        failQuitGuc = true;
    }
	guc_vector.clear();

	return guc_text;
}

void DspController::setFreq(int value)
{
    freqGucValue  = value;
}

bool DspController::getIsGucCoord()
{
    return isGpsGuc;
}

void DspController::recGucLog(uint8_t address, uint8_t* data, int data_len)
{
	uint8_t indicator  = qmFromBigEndian<uint8_t>(data+0);
	uint8_t code       = qmFromBigEndian<uint8_t>(data+1);

	uint8_t *value_ptr = data + 2;
	int value_len      = data_len - 2;


	if (address == 0x7B)
	{
		if (indicator == 22)
		{
			if (ContentGuc.stage == GucRx)
			{
				//qmDebugMessage(QmDebug::Dump, "---- 0x7B indicator:22 GucRx");
				exitVoceMode();
				magic();
			}
			else if (ContentGuc.stage == GucTx) // wait recieving ack
			{
				//qmDebugMessage(QmDebug::Dump, "---- 0x7B indicator:22 GucTx");
				if (isGucWaitReceipt)
				{
					//qmDebugMessage(QmDebug::Dump, "---- isGucWaitReceipt");
					startGucRecieving();
					ContentGuc.stage = GucTx;
					startRxQuit();
					startGucTimer();
				}
				else
				{
					//qmDebugMessage(QmDebug::Dump, "---- NOT isGucWaitReceipt");
					startRxQuit();
					completedStationMode(true);
				}
			}
		}
	}

	if (address == 0x6B)
	{
		if (ContentGuc.stage != GucNone)
		{
			if (indicator == 32)
			{
				//qmDebugMessage(QmDebug::Dump, "0x6B recieved frame: indicator %d", indicator);
			}
			if (indicator == 30)
			{
				ContentGuc.R_ADR = ((data[2] & 0xF8) >> 3);
				ContentGuc.uin   = ((data[4] & 0x1) << 7) + ((data[5] & 0xFE) >> 1);
				isGpsGuc = data[5] & 0x1;
				ContentGuc.S_ADR = ((data[2] & 0x7) << 2) + ((data[3] & 0xC0) >> 6);
				if (ContentGuc.stage == GucTx)
				{
					//ContentGuc.S_ADR = ((data[2] & 0x7) << 2) + ((data[3] & 0xC0) >> 6);
					//qmDebugMessage(QmDebug::Dump, "---- 0x6B indicator:30 stage: GucTx R_ADR: %d S_ADR: %d", ContentGuc.R_ADR, ContentGuc.S_ADR);
					recievedGucQuitForTransm(ContentGuc.S_ADR);
					completedStationMode(false);
					stopGucTimer();
				}
				else
				{
					//qmDebugMessage(QmDebug::Dump, "---- 0x6B indicator:30 stage: GucRx R_ADR: %d S_ADR: %d", ContentGuc.R_ADR, ContentGuc.S_ADR);
					std::vector<uint8_t> guc;
					for(int i = 0;i<data_len;i++)
					{
						//qmDebugMessage(QmDebug::Dump, "0x6B recieved frame: %d , num %d", data[i],i);
						guc.push_back(data[i]);
					}
					guc_vector.push_back(guc);
					//guc_timer->start();
					recievedGucResp(ContentGuc.R_ADR, ContentGuc.S_ADR != 0);
					if (ContentGuc.S_ADR != 0)
					{
						startGucTransmitting();
						sendGucQuit();
					}

				}
			}
		}
	}
}



} /* namespace Multiradio */
