/**
 ******************************************************************************
 * @file    multiradio.h
 * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
 * @date    29.10.2015
 *
 ******************************************************************************
 */

#ifndef FIRMWARE_APP_MULTIRADIO_H_
#define FIRMWARE_APP_MULTIRADIO_H_

#include <list>

namespace Multiradio {

enum voice_channel_t {
	channelInvalid,
	channelOpen,
	channelClose
};

enum voice_channel_speed_t {
	voicespeedInvalid,
	voicespeed600,
	voicespeed1200,
	voicespeed2400,
	voicespeed4800,
};

struct voice_channel_entry_t {
	uint32_t frequency;
	voice_channel_t type;
	voice_channel_speed_t speed;
};

typedef std::list<voice_channel_entry_t> voice_channels_table_t;

} /* namespace Multiradio */

#endif /* FIRMWARE_APP_MULTIRADIO_H_ */
