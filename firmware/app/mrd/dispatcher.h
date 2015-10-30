/**
 ******************************************************************************
 * @file    dispatcher.h
 * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
 * @date    30.10.2015
 *
 ******************************************************************************
 */

#ifndef FIRMWARE_APP_MRD_DISPATCHER_H_
#define FIRMWARE_APP_MRD_DISPATCHER_H_

#include "multiradio.h"

namespace Headset {
    class Controller;
}

namespace Multiradio {

class MainServiceInterface;
class VoiceServiceInterface;

class Dispatcher {
public:
	Dispatcher(int dsp_uart_resource, int dspreset_iopin_resource, int atu_uart_resource,
			Headset::Controller *headset_controller);
	~Dispatcher();
	void startServicing(const Multiradio::voice_channels_table_t &voice_channels_table);
	MainServiceInterface* getMainServiceInterface();
	VoiceServiceInterface* getVoiceServiceInterface();
};

} /* namespace Multiradio */

#endif /* FIRMWARE_APP_MRD_DISPATCHER_H_ */
