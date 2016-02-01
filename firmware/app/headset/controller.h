/**
 ******************************************************************************
 * @file    Controller.h
 * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
 * @date    29.10.2015
 *
 ******************************************************************************
 */

#ifndef FIRMWARE_APP_HEADSET_CONTROLLER_H_
#define FIRMWARE_APP_HEADSET_CONTROLLER_H_

#include "sigc++/signal.h"
#include "multiradio.h"
#include "qmpushbuttonkey.h"

namespace Headset {

class Controller {
public:
	enum Status {
		StatusNone,
		StatusSmartOk,
		StatusSmartMalfunction,
		StatusAnalog
	};
	struct SmartStatusDescription {
		bool channels_mismatch;
	};

	Controller(int rs232_uart_resource, int ptt_iopin_resource);
	~Controller();
	void startServicing(const Multiradio::voice_channels_table_t &local_channels_table);
	Status getStatus();
	bool getSmartStatus(SmartStatusDescription &description);
	bool getAnalogStatus(bool &open_channels_missing);
	bool getPTTState(bool &state);
	bool getSmartCurrentChannel(int &number, Multiradio::voice_channel_t &type);

	sigc::signal<void, Status/*new_status*/> statusChanged;
	sigc::signal<bool/*accepted*/, bool/*new_state*/> pttStateChanged; // single connection (returns value)
	sigc::signal<void, int/*new_channel_number*/, Multiradio::voice_channel_t/*new_channel_type*/> smartCurrentChannelChanged;
private:
	void pttStateChangedSlot();

	QmPushButtonKey* ptt_key;
	bool ptt_state;
};

} /* namespace Headset */

#endif /* FIRMWARE_APP_HEADSET_CONTROLLER_H_ */
