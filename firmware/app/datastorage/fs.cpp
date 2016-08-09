#include "qm.h"
#include <qmfile.h>
#include "fs.h"

#include "../../../sazhenn.h"

namespace DataStorage {

FS::FS(int flash_spi_resource) {
	QM_UNUSED(flash_spi_resource);
	//...
}

FS::~FS() {
	//...
}

void FS::init() {
	//...
    RnKey = 0;
    voice_frequency = 10000000;
    voice_emission_type = Multiradio::voiceemissionUSB;
    voice_channel_speed = Multiradio::voicespeed1200;
}

bool FS::getVoiceChannelsTable(Multiradio::voice_channels_table_t& data) {
	data.clear();

	Multiradio::voice_channel_entry_t ch_entry;
	ch_entry.speed = Multiradio::voicespeed1200;

	//КВ
	ch_entry.type = Multiradio::channelOpen;
	ch_entry.frequency = 3288000;
	data.push_back(ch_entry);
	ch_entry.type = Multiradio::channelClose;
	ch_entry.frequency = 3288000;
	data.push_back(ch_entry);
	ch_entry.type = Multiradio::channelOpen;
	ch_entry.frequency = 3810000;
	data.push_back(ch_entry);
	ch_entry.type = Multiradio::channelClose;
	ch_entry.frequency = 3810000;
	data.push_back(ch_entry);
	ch_entry.type = Multiradio::channelOpen;
	ch_entry.frequency = 4517000;
	data.push_back(ch_entry);
	ch_entry.type = Multiradio::channelClose;
	ch_entry.frequency = 4517000;
	data.push_back(ch_entry);
	ch_entry.type = Multiradio::channelOpen;
	ch_entry.frequency = 5786000;
	data.push_back(ch_entry);
	ch_entry.type = Multiradio::channelClose;
	ch_entry.frequency = 5786000;
	data.push_back(ch_entry);
	ch_entry.type = Multiradio::channelOpen;
	ch_entry.frequency = 6779000;
	data.push_back(ch_entry);
	ch_entry.type = Multiradio::channelClose;
	ch_entry.frequency = 6779000;
	data.push_back(ch_entry);
	ch_entry.type = Multiradio::channelOpen;
	ch_entry.frequency = 7011000;
	data.push_back(ch_entry);
	ch_entry.type = Multiradio::channelClose;
	ch_entry.frequency = 7011000;
	data.push_back(ch_entry);
	ch_entry.type = Multiradio::channelOpen;
	ch_entry.frequency = 8667000;
	data.push_back(ch_entry);
	ch_entry.type = Multiradio::channelClose;
	ch_entry.frequency = 8667000;
	data.push_back(ch_entry);
	ch_entry.type = Multiradio::channelOpen;
	ch_entry.frequency = 10227000;
	data.push_back(ch_entry);
	ch_entry.type = Multiradio::channelClose;
	ch_entry.frequency = 10227000;
	data.push_back(ch_entry);
	ch_entry.type = Multiradio::channelOpen;
	ch_entry.frequency = 12143000;
	data.push_back(ch_entry);
	ch_entry.type = Multiradio::channelClose;
	ch_entry.frequency = 12143000;
	data.push_back(ch_entry);
	ch_entry.type = Multiradio::channelOpen;
	ch_entry.frequency = 12204000;
	data.push_back(ch_entry);
	ch_entry.type = Multiradio::channelClose;
	ch_entry.frequency = 12204000;
	data.push_back(ch_entry);
	ch_entry.type = Multiradio::channelOpen;
	ch_entry.frequency = 14557000;
	data.push_back(ch_entry);
	ch_entry.type = Multiradio::channelClose;
	ch_entry.frequency = 14557000;
	data.push_back(ch_entry);

	//УКВ
	ch_entry.type = Multiradio::channelOpen;
	ch_entry.frequency = 30050000;
	data.push_back(ch_entry);
	ch_entry.type = Multiradio::channelClose;
	ch_entry.frequency = 30050000;
	data.push_back(ch_entry);
	ch_entry.type = Multiradio::channelOpen;
	ch_entry.frequency = 33500000;
	data.push_back(ch_entry);
	ch_entry.type = Multiradio::channelClose;
	ch_entry.frequency = 33500000;
	data.push_back(ch_entry);
	ch_entry.type = Multiradio::channelOpen;
	ch_entry.frequency = 36150000;
	data.push_back(ch_entry);
	ch_entry.type = Multiradio::channelClose;
	ch_entry.frequency = 36150000;
	data.push_back(ch_entry);
	ch_entry.type = Multiradio::channelOpen;
	ch_entry.frequency = 38450000;
	data.push_back(ch_entry);
	ch_entry.type = Multiradio::channelClose;
	ch_entry.frequency = 38450000;
	data.push_back(ch_entry);
	ch_entry.type = Multiradio::channelOpen;
	ch_entry.frequency = 40450000;
	data.push_back(ch_entry);
	ch_entry.type = Multiradio::channelClose;
	ch_entry.frequency = 40450000;
	data.push_back(ch_entry);
	ch_entry.type = Multiradio::channelOpen;
	ch_entry.frequency = 42425000;
	data.push_back(ch_entry);
	ch_entry.type = Multiradio::channelClose;
	ch_entry.frequency = 42425000;
	data.push_back(ch_entry);
	ch_entry.type = Multiradio::channelOpen;
	ch_entry.frequency = 45600000;
	data.push_back(ch_entry);
	ch_entry.type = Multiradio::channelClose;
	ch_entry.frequency = 45600000;
	data.push_back(ch_entry);
	ch_entry.type = Multiradio::channelOpen;
	ch_entry.frequency = 49600000;
	data.push_back(ch_entry);
	ch_entry.type = Multiradio::channelClose;
	ch_entry.frequency = 49600000;
	data.push_back(ch_entry);

	ch_entry.frequency = 50000000;
	for (int i = data.size(); i < 98; ++i) {
		if (data.at(i - 1).type == Multiradio::channelOpen) {
			ch_entry.type = Multiradio::channelClose;
		} else {
			ch_entry.type = Multiradio::channelOpen;
		}
		data.push_back(ch_entry);
	}
	data[87].type = Multiradio::channelInvalid;

	return true;
}

bool FS::getAleDefaultCallFreqs(Multiradio::ale_call_freqs_t &data) {
	data.clear();
	data.push_back(3288000);
	data.push_back(3637000);
	data.push_back(3844000);
	data.push_back(4517000);
	data.push_back(4609000);
	data.push_back(4889000);
	data.push_back(5786000);
	data.push_back(5851000);
	data.push_back(6323000);
	data.push_back(6779000);
	data.push_back(7709000);
	data.push_back(8651000);
	return true;
}

bool FS::getAleStationAddress(uint8_t& data) {
	data = SAZHEN_NETWORK_ADDRESS;
	return true;
}

bool FS::getFhssKey(uint8_t& data) {
    data = RnKey;
	return false;
}

void FS::setFhssKey(uint8_t data) {
    RnKey = data;
}

bool FS::getVoiceFrequency(uint32_t &data) {
	data = voice_frequency;
	return true;
}

void FS::setVoiceFrequency(uint32_t data) {
	voice_frequency = data;
}

bool FS::getVoiceEmissionType(Multiradio::voice_emission_t &data) {
	data = voice_emission_type;
	return true;
}

void FS::setVoiceEmissionType(Multiradio::voice_emission_t data) {
	voice_emission_type = data;
}

bool FS::getVoiceChannelSpeed(Multiradio::voice_channel_speed_t &data) {
	data = voice_channel_speed;
	return true;
}

void FS::setVoiceChannelSpeed(Multiradio::voice_channel_speed_t data) {
	voice_channel_speed = data;
}

} /* namespace DataStorage */
