#include "qm.h"

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
}

bool FS::getVoiceChannelsTable(Multiradio::voice_channels_table_t& data) {
	data.clear();

	Multiradio::voice_channel_entry_t ch_entry;
	ch_entry.speed = Multiradio::voicespeed1200;

	//КВ: 1-20
	ch_entry.type = Multiradio::channelOpen;
	ch_entry.frequency = 3288000;
	data.push_back(ch_entry);
	ch_entry.type = Multiradio::channelClose;
	ch_entry.frequency = 3288000;
	data.push_back(ch_entry);
	ch_entry.type = Multiradio::channelOpen;
	ch_entry.frequency = 3637000;
	data.push_back(ch_entry);
	ch_entry.type = Multiradio::channelClose;
	ch_entry.frequency = 3637000;
	data.push_back(ch_entry);
	ch_entry.type = Multiradio::channelOpen;
	ch_entry.frequency = 3810000;
	data.push_back(ch_entry);
	ch_entry.type = Multiradio::channelClose;
	ch_entry.frequency = 3810000;
	data.push_back(ch_entry);
	ch_entry.type = Multiradio::channelOpen;
	ch_entry.frequency = 3844000;
	data.push_back(ch_entry);
	ch_entry.type = Multiradio::channelClose;
	ch_entry.frequency = 3844000;
	data.push_back(ch_entry);
	ch_entry.type = Multiradio::channelOpen;
	ch_entry.frequency = 3871000;
	data.push_back(ch_entry);
	ch_entry.type = Multiradio::channelClose;
	ch_entry.frequency = 3871000;
	data.push_back(ch_entry);
	ch_entry.type = Multiradio::channelOpen;
	ch_entry.frequency = 3975000;
	data.push_back(ch_entry);
	ch_entry.type = Multiradio::channelClose;
	ch_entry.frequency = 3975000;
	data.push_back(ch_entry);
	ch_entry.type = Multiradio::channelOpen;
	ch_entry.frequency = 4517000;
	data.push_back(ch_entry);
	ch_entry.type = Multiradio::channelClose;
	ch_entry.frequency = 4517000;
	data.push_back(ch_entry);
	ch_entry.type = Multiradio::channelOpen;
	ch_entry.frequency = 4609000;
	data.push_back(ch_entry);
	ch_entry.type = Multiradio::channelClose;
	ch_entry.frequency = 4609000;
	data.push_back(ch_entry);
	ch_entry.type = Multiradio::channelOpen;
	ch_entry.frequency = 4807000;
	data.push_back(ch_entry);
	ch_entry.type = Multiradio::channelClose;
	ch_entry.frequency = 4807000;
	data.push_back(ch_entry);
	ch_entry.type = Multiradio::channelOpen;
	ch_entry.frequency = 4827000;
	data.push_back(ch_entry);
	ch_entry.type = Multiradio::channelClose;
	ch_entry.frequency = 4827000;
	data.push_back(ch_entry);
	ch_entry.type = Multiradio::channelOpen;
	ch_entry.frequency = 4889000;
	data.push_back(ch_entry);
	ch_entry.type = Multiradio::channelClose;
	ch_entry.frequency = 4889000;
	data.push_back(ch_entry);
	ch_entry.type = Multiradio::channelOpen;
	ch_entry.frequency = 5770000;
	data.push_back(ch_entry);
	ch_entry.type = Multiradio::channelClose;
	ch_entry.frequency = 5770000;
	data.push_back(ch_entry);
	ch_entry.type = Multiradio::channelOpen;
	ch_entry.frequency = 5786000;
	data.push_back(ch_entry);
	ch_entry.type = Multiradio::channelClose;
	ch_entry.frequency = 5786000;
	data.push_back(ch_entry);
	ch_entry.type = Multiradio::channelOpen;
	ch_entry.frequency = 5806000;
	data.push_back(ch_entry);
	ch_entry.type = Multiradio::channelClose;
	ch_entry.frequency = 5806000;
	data.push_back(ch_entry);
	ch_entry.type = Multiradio::channelOpen;
	ch_entry.frequency = 5851000;
	data.push_back(ch_entry);
	ch_entry.type = Multiradio::channelClose;
	ch_entry.frequency = 5851000;
	data.push_back(ch_entry);
	ch_entry.type = Multiradio::channelOpen;
	ch_entry.frequency = 5961000;
	data.push_back(ch_entry);
	ch_entry.type = Multiradio::channelClose;
	ch_entry.frequency = 5961000;
	data.push_back(ch_entry);
	ch_entry.type = Multiradio::channelOpen;
	ch_entry.frequency = 6197000;
	data.push_back(ch_entry);
	ch_entry.type = Multiradio::channelClose;
	ch_entry.frequency = 6197000;
	data.push_back(ch_entry);
	ch_entry.type = Multiradio::channelOpen;
	ch_entry.frequency = 6323000;
	data.push_back(ch_entry);
	ch_entry.type = Multiradio::channelClose;
	ch_entry.frequency = 6323000;
	data.push_back(ch_entry);
	ch_entry.type = Multiradio::channelOpen;
	ch_entry.frequency = 6425000;
	data.push_back(ch_entry);
	ch_entry.type = Multiradio::channelClose;
	ch_entry.frequency = 6425000;
	data.push_back(ch_entry);
	ch_entry.type = Multiradio::channelOpen;
	ch_entry.frequency = 6779000;
	data.push_back(ch_entry);
	ch_entry.type = Multiradio::channelClose;
	ch_entry.frequency = 6779000;
	data.push_back(ch_entry);

	//УКВ: 11-24
	ch_entry.type = Multiradio::channelOpen;
	ch_entry.frequency = 37525000;
	data.push_back(ch_entry);
	ch_entry.type = Multiradio::channelClose;
	ch_entry.frequency = 37525000;
	data.push_back(ch_entry);
	ch_entry.type = Multiradio::channelOpen;
	ch_entry.frequency = 38450000;
	data.push_back(ch_entry);
	ch_entry.type = Multiradio::channelClose;
	ch_entry.frequency = 38450000;
	data.push_back(ch_entry);
	ch_entry.type = Multiradio::channelOpen;
	ch_entry.frequency = 39575000;
	data.push_back(ch_entry);
	ch_entry.type = Multiradio::channelClose;
	ch_entry.frequency = 39575000;
	data.push_back(ch_entry);
	ch_entry.type = Multiradio::channelOpen;
	ch_entry.frequency = 40450000;
	data.push_back(ch_entry);
	ch_entry.type = Multiradio::channelClose;
	ch_entry.frequency = 40450000;
	data.push_back(ch_entry);
	ch_entry.type = Multiradio::channelOpen;
	ch_entry.frequency = 41550000;
	data.push_back(ch_entry);
	ch_entry.type = Multiradio::channelClose;
	ch_entry.frequency = 41550000;
	data.push_back(ch_entry);
	ch_entry.type = Multiradio::channelOpen;
	ch_entry.frequency = 41950000;
	data.push_back(ch_entry);
	ch_entry.type = Multiradio::channelClose;
	ch_entry.frequency = 41950000;
	data.push_back(ch_entry);
	ch_entry.type = Multiradio::channelOpen;
	ch_entry.frequency = 42425000;
	data.push_back(ch_entry);
	ch_entry.type = Multiradio::channelClose;
	ch_entry.frequency = 42425000;
	data.push_back(ch_entry);
	ch_entry.type = Multiradio::channelOpen;
	ch_entry.frequency = 43525000;
	data.push_back(ch_entry);
	ch_entry.type = Multiradio::channelClose;
	ch_entry.frequency = 43525000;
	data.push_back(ch_entry);
	ch_entry.type = Multiradio::channelOpen;
	ch_entry.frequency = 44575000;
	data.push_back(ch_entry);
	ch_entry.type = Multiradio::channelClose;
	ch_entry.frequency = 44575000;
	data.push_back(ch_entry);
	ch_entry.type = Multiradio::channelOpen;
	ch_entry.frequency = 45600000;
	data.push_back(ch_entry);
	ch_entry.type = Multiradio::channelClose;
	ch_entry.frequency = 45600000;
	data.push_back(ch_entry);
	ch_entry.type = Multiradio::channelOpen;
	ch_entry.frequency = 46775000;
	data.push_back(ch_entry);
	ch_entry.type = Multiradio::channelClose;
	ch_entry.frequency = 46775000;
	data.push_back(ch_entry);
	ch_entry.type = Multiradio::channelOpen;
	ch_entry.frequency = 47425000;
	data.push_back(ch_entry);
	ch_entry.type = Multiradio::channelClose;
	ch_entry.frequency = 47425000;
	data.push_back(ch_entry);
	ch_entry.type = Multiradio::channelOpen;
	ch_entry.frequency = 48575000;
	data.push_back(ch_entry);
	ch_entry.type = Multiradio::channelClose;
	ch_entry.frequency = 48575000;
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

} /* namespace DataStorage */
