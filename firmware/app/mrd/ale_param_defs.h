/**
  ******************************************************************************
  * @file    ale_param_defs.h
  * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
  * @date    27.05.2016
  *
  ******************************************************************************
 */

#ifndef ALE_PARAM_DEFS_H_
#define ALE_PARAM_DEFS_H_

#include "qm.h"

#define ALE_OPTION_DISABLE_ADAPTATION
#define ALE_OPTION_IGNORE_SNR

/* Конфигурируемые параметры алгоритма */
#define ALE_VM_INITIAL_SFORM	6
#define ALE_CALL_SNR_HIGH		-8
#define ALE_CALL_SNR_LOW		-13

/* Из таблицы общих констант */
#define ALE_TIME_TTuneTx			100
#define ALE_TIME_TOpenTx			10
#define ALE_TIME_TTuneRx			10
#define ALE_TIME_TEthTx				50
#define ALE_TIME_TEthRx				50
#define ALE_TIME_TRChan				16
#define ALE_TIME_DTMistiming		((ALE_TIME_TEthTx + ALE_TIME_TRChan + ALE_TIME_TEthRx)/2)
#define ALE_TIME_THshakeTransMode	3024
#define ALE_TIME_TRespCallQual		2128
#define ALE_TIME_THshakeReceiv		448
#define ALE_TIME_THshakeTrans		448
#define ALE_TIME_TmsgHeadL			2576
#define ALE_TIME_TpackHeadL			2576
#define ALE_TIME_TRespPackQualL		1792
#define ALE_TIME_TLinkReleaseL		784
#define ALE_TIME_dTInit				100

/* Из перечня вспомогательных констант */
#define ALE_TIME_TMaxEthX		qmMax(ALE_TIME_TEthTx, ALE_TIME_TEthRx)
#define ALE_TIME_TMaxOpenTuneX	qmMax(ALE_TIME_TOpenTx, ALE_TIME_TTuneRx)

/* Из таблицы констант, зависящих от режима синхронизации времени */
#define ALE_TIME_dTSyn			250
#define ALE_TIME_TCall			2128

/* Из перечня констант для цикла передачи данных сеанса голосовой почты */
#define ALE_TIME_dTSynPacket(sform)	(((sform) == -1)?(100):(0))
#define ALE_TIME_THeadL(sform)	(((sform) == -1)?(ALE_TIME_TmsgHeadL):(ALE_TIME_TpackHeadL))
//																0		1		2		3		4		5		6		7
#define ALE_TIME_TDataL(sform)	(((sform) == -1)?(0):((int[]){	2640,	3960,	7920,	7920,	7392,	14784,	29568,	59136	}[(sform)]))

//								   0  1  2  3   4   5    6    7
#define ALE_VM_SNR_TABLE_VALUES	{ 10, 8, 5, 5, -5, -8, -11, -14 }

#endif /* ALE_PARAM_DEFS_H_ */
