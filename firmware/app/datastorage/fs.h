#ifndef FIRMWARE_APP_DATASTORAGE_FS_H_
#define FIRMWARE_APP_DATASTORAGE_FS_H_

#include <string>
#include <stdio.h>
#include "multiradio.h"

namespace DataStorage {

class FS {
public:
    enum FileType { FT_SMS = 0,
                    FT_VM  = 1,
					FT_CND = 2,
                    FT_GRP = 3};

    struct FileTypeInfo {uint8_t count = 0;
                         uint8_t maxCount = 10;
                         std::string fileName;};

	FS(const std::string &dir);
	~FS();
	bool getVoiceChannelsTable(Multiradio::voice_channels_table_t &data);
	bool getAleDefaultCallFreqs(Multiradio::ale_call_freqs_t &data);
	bool getAleStationAddress(uint8_t &data);
	bool getFhssKey(uint16_t &data);
	void setFhssKey(uint16_t data);
	bool getVoiceFrequency(uint32_t &data);
	void setVoiceFrequency(uint32_t data);
	bool getVoiceEmissionType(Multiradio::voice_emission_t &data);
	void setVoiceEmissionType(Multiradio::voice_emission_t data);
	bool getVoiceChannelSpeed(Multiradio::voice_channel_speed_t &data);
	void setVoiceChannelSpeed(Multiradio::voice_channel_speed_t data);
	bool getAnalogHeadsetChannel(uint8_t &data);
	void setAnalogHeadsetChannel(uint8_t data);

    bool getCondCommand(std::vector<uint8_t>* data, uint8_t number);
    void setCondCommand(std::vector<uint8_t> *data);
    bool getGroupCondCommand(std::vector<uint8_t> *data, uint8_t number);
    void setGroupCondCommand(uint8_t *data, uint16_t size);
    bool getSms(std::vector<uint8_t> *data, uint8_t number);
    void setSms(uint8_t* data, uint16_t size);
    bool getVoiceMail(std::vector<uint8_t> *data, uint8_t number);
    void setVoiceMail(std::vector<uint8_t> *data);
    void updateFileTree();
    std::vector<std::string> *getFileTree();
    void getFileNamesByType(std::vector<std::string>* typeFiles, FileType fileType);
private:
    bool renameFile(std::string oldfileName, std::string newFileName);
    bool deleteFile(std::string fileName);
    bool existFile(std::string fileName);
    std::string dir;
    std::vector<std::string> files;
    uint8_t maxFilesCount = 10;
    std::string errorFileName = "error";
    FileTypeInfo fileTypeInfo[4];
    uint8_t getFreeFileSlotCount();

    std::string prepareFileStorageToWriting(FileType fileType);
    std::string generateFileNameByNumber(FileType fileType, uint8_t number);
    void prepareFreeFileSlot(FS::FileType fileType);
};

} /* namespace DataStorage */

#endif /* FIRMWARE_APP_DATASTORAGE_FS_H_ */
