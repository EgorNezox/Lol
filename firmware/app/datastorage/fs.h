#ifndef FIRMWARE_APP_DATASTORAGE_FS_H_
#define FIRMWARE_APP_DATASTORAGE_FS_H_

#include "multiradio.h"

namespace DataStorage {

class FS {
public:
	FS(int flash_spi_resource);
	~FS();
	void init();
	bool getVoiceChannelsTable(Multiradio::voice_channels_table_t &data);
	bool getAleDefaultCallFreqs(Multiradio::ale_call_freqs_t &data);
	bool getAleStationAddress(uint8_t &data);
	bool getFhssKey(uint8_t &data);
	void setFhssKey(uint8_t data);
	void getTimeSyncMode(Multiradio::time_sync_mode_t &data);
	void setTimeSyncMode(Multiradio::time_sync_mode_t data);
};

} /* namespace DataStorage */

#endif /* FIRMWARE_APP_DATASTORAGE_FS_H_ */
