/**
 ******************************************************************************
 * @file    PswfModes.h
 * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
 * @date    25.09.2017
 *
 ******************************************************************************
 */

#ifndef FIRMWARE_APP_DSP_PSWFMODES_H_
#define FIRMWARE_APP_DSP_PSWFMODES_H_

#include <stdio.h>
#include "dspcontroller.h"
#include <vector>

// класс для обработки сообщений и условных команд

namespace Multiradio
{

class PswfModes : public QmObject
{
public:
	PswfModes(DspController *control);
	~PswfModes();

	typedef struct trFrame
	{
		int address;
		uint8_t* data;
		int len;
	};

	trFrame frame;

	// запуск режима передачи УК
	void startPswfTx(bool ack, uint8_t r_adr, uint8_t cmd,int retr);
	// запуск режима приема УК
	void startPswfRx();

	// запуск режима передачи СМС
	void startSmsTx ();
	// запуск режима приема СМС
	void startSmsRx ();

private:
	// расшариваем методы контроллера для ппрч режимов
	friend class DspController;

	// вычисляем смещение частоты для УК
    uint32_t CalcShiftFreq	   		  (uint32_t RN_KEY, uint32_t DAY, uint32_t HRS, uint32_t MIN, uint32_t SEC);                  // функция рассчета частоты смещения для УК
    // вычисляем частоту для СМС
    uint32_t CalcSmsTransmitFreq	  (uint32_t RN_KEY, uint32_t DAY, uint32_t HRS, uint32_t MIN, uint32_t SEC);                  // функция рассчета частоты смещения для СМС
    // вычисление волновой зоны для СМС
    uint32_t wzn_change				  (std::vector<int> &vect);
    // вычисление параметра FSTN для СМС
    uint32_t calcFstn(int R_ADR, int S_ADR, int RN_KEY, int DAY, int HRS, int MIN, int SEC, int QNB);

    uint32_t check_rx_call(int* wzn);

    uint32_t calc_ack_code(uint8_t ack);

    // логика для приемав и передачи УК
    void LogicPswfTx();
    void LogicPswfRx();

    // логика для приемав и передачи СМС
    void LogicSmsTx();
    void LogicSmsRx();
    // обработка пакетов по 63 адресу для обоих режимов
    void DataHandler(uint8_t* data, uint8_t indicator, int data_len);

    // обработка приема
    void recSms (uint8_t *data);
    void recPswf(uint8_t data,uint8_t code, uint8_t indicator);

    // обработка пакета передачи
    trFrame sendPswf();
    trFrame sendSms ();

    DspController *control;
};

} /* namespace Multiradio */

#endif /* FIRMWARE_APP_DSP_PSWFMODES_H_ */
