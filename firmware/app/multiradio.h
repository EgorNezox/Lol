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

#include <vector>

namespace Multiradio {

enum voice_emission_t {
	voiceemissionInvalid,
	voiceemissionUSB,
	voiceemissionFM
};

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

typedef std::vector<voice_channel_entry_t> voice_channels_table_t;

typedef std::vector<uint8_t> voice_message_t;

typedef std::vector<uint32_t> ale_call_freqs_t;
typedef std::vector<uint32_t> ale_work_freqs_t;

}

#endif /* FIRMWARE_APP_MULTIRADIO_H_ */
