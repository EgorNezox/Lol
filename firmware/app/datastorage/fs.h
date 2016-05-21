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
};

} /* namespace DataStorage */

#endif /* FIRMWARE_APP_DATASTORAGE_FS_H_ */
