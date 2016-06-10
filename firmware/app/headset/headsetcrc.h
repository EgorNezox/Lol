/**
 ******************************************************************************
 * @file    headsetcrc.h
 * @author  Petr Dmitriev
 * @date    09.06.2016
 *
 ******************************************************************************
 */

#ifndef FIRMWARE_APP_HEADSET_HEADSETCRC_H_
#define FIRMWARE_APP_HEADSET_HEADSETCRC_H_

#include "qmcrc.h"

typedef QmCrc<uint16_t, 16, 0x1021, 0xFFFF, true, 0x0000> HeadsetCRC;

#endif /* FIRMWARE_APP_HEADSET_HEADSETCRC_H_ */
