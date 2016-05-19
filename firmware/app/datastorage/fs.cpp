#include "qm.h"

#include "fs.h"

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
}

bool FS::getVoiceChannelsTable(Multiradio::voice_channels_table_t& data) {
	Multiradio::voice_channel_entry_t ch_entry;
	ch_entry.speed = Multiradio::voicespeed1200;

	ch_entry.type = Multiradio::channelOpen;
	ch_entry.frequency = 1500000;
	data.push_back(ch_entry);
	ch_entry.type = Multiradio::channelClose;
	ch_entry.frequency = 3000000;
	data.push_back(ch_entry);
	ch_entry.type = Multiradio::channelOpen;
	ch_entry.frequency = 5000000;
	data.push_back(ch_entry);
	ch_entry.type = Multiradio::channelClose;
	ch_entry.frequency = 5000000;
	data.push_back(ch_entry);
	ch_entry.type = Multiradio::channelOpen;
	ch_entry.frequency = 8000000;
	data.push_back(ch_entry);
	ch_entry.type = Multiradio::channelClose;
	ch_entry.frequency = 10000000;
	data.push_back(ch_entry);
	ch_entry.type = Multiradio::channelOpen;
	ch_entry.frequency = 15000000;
	data.push_back(ch_entry);
	ch_entry.type = Multiradio::channelClose;
	ch_entry.frequency = 17000000;
	data.push_back(ch_entry);
	ch_entry.type = Multiradio::channelOpen;
	ch_entry.frequency = 27000000;
	data.push_back(ch_entry);
	ch_entry.type = Multiradio::channelClose;
	ch_entry.frequency = 30000000;
	ch_entry.speed = Multiradio::voicespeed4800;
	data.push_back(ch_entry);
	ch_entry.type = Multiradio::channelOpen;
	ch_entry.frequency = 31000000;
	data.push_back(ch_entry);
	ch_entry.type = Multiradio::channelClose;
	ch_entry.frequency = 35000000;
	data.push_back(ch_entry);
	ch_entry.type = Multiradio::channelOpen;
	ch_entry.frequency = 40000000;
	data.push_back(ch_entry);
	ch_entry.type = Multiradio::channelClose;
	ch_entry.frequency = 45000000;
	data.push_back(ch_entry);
	ch_entry.type = Multiradio::channelOpen;
	ch_entry.frequency = 48000000;
	data.push_back(ch_entry);
	ch_entry.type = Multiradio::channelClose;
	ch_entry.frequency = 50000000;
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

} /* namespace DataStorage */
