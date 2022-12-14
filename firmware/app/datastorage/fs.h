#ifndef FIRMWARE_APP_DATASTORAGE_FS_H_
#define FIRMWARE_APP_DATASTORAGE_FS_H_

#include <string>
#include <stdio.h>
#include "../multiradio.h"


namespace DataStorage {

class FS {
public:
    enum FileType { FT_SP = 0,
                    FT_CND  = 1,
					FT_GRP  = 2,
                    FT_SMS = 3,
                    FT_VM = 4 };

    enum TransitionFileType { TFT_RX = 0,
                              TFT_TX = 1};

    struct FileCounter {
        uint8_t count = 0;
        uint8_t maxCount = 10;
    };

    struct FileTypeInfo {
        FileCounter counter[2];
        std::string fileName;
    };

	FS(const std::string &dir);
	~FS();
	bool readFilterCoeff(float* data);
	bool writeFilterCoeff(float data);

	bool writeFileFromId(int id, uint8_t* data, uint16_t len);
	uint16_t getSizeDataFileFromId(int id);
	bool    getFastFileFromId(int id, uint16_t filesize, uint8_t *file_data);

	void setBugDetect();
	void resetBug();
	bool getBugState();

	bool getDiagnsticInfo();
	void findFilesToFiletree();
	void findFilesToFiletree(std::vector<std::string> &files);

	bool getVoiceChannelsTable(Multiradio::voice_channels_table_t &data, uint8_t &count);
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

	void setUsbOnOff(uint8_t data);
	bool getUsbOnOff(uint8_t &data);

	bool addVoiceChannelTable(uint8_t position, Multiradio::voice_channel_entry_t& entry);
	void setVoceChannelTable();

    bool readMessage(FileType fileType, TransitionFileType transFileType, std::vector<uint8_t> *data, uint8_t fileNumber);
    bool writeMessage(FileType fileType, TransitionFileType transFileType, std::vector<uint8_t> *data);
    void updateFileTree();
    std::vector<std::string> *getFileTree();
    void getFileNamesByType(std::vector<std::string>* typeFiles, FileType fileType);
    uint8_t getTransmitFileTypeCount(FS::FileType fileType, FS::TransitionFileType transFileType);
    uint8_t getFileNumber(FS::FileType fileType, uint8_t fileTreeTypeFocus);
    bool getSheldure(uint8_t *data);
    TransitionFileType getTransmitType(FS::FileType fileType, uint8_t fileTreeTypeFocus);
    bool setSheldure(uint8_t *data, uint16_t size);
    bool getGpsSynchroMode(uint8_t *data);
    void setGpsSynchroMode(uint8_t data);
    void setVoiceMode(bool data);
    bool getVoiceMode(bool *data);
    bool setAleStationAddress(uint8_t data);
    bool getAleDefaultWorkFreqs(Multiradio::ale_work_freqs_t &data);

    bool setTimeZone(uint8_t &zone);
    bool getTimeZone(uint8_t &zone);

    int getMessageCount(FileType mode, TransitionFileType txrx);

    bool getGenDacValue(int *data);
    bool setGenDacValue(int data);
private:
    bool renameFile(std::string oldfileName, std::string newFileName);
    bool deleteFile(std::string fileName);
    bool existFile(std::string fileName);
    std::string dir;
    std::vector<std::string> files;
    uint8_t transmitFileTypeCount[5][2] = {{0,0},{0,0},{0,0},{0,0},{0,0}};
    uint8_t maxFilesCount = 32;
    std::string errorFileName = "error";
    FileTypeInfo fileTypeInfo[5];
    uint8_t getFreeFileSlotCount();
    std::string trans[2];

    bool isBugDetect = false;
    bool isFileTreeReady = false;

    std::string prepareFileStorageToWriting (FileType fileType, TransitionFileType transFileType);
    std::string generateFileNameByNumber    (FileType fileType, TransitionFileType transFileType, uint8_t fileNumber);
    void prepareFreeFileSlot                (FS::FileType fileType, TransitionFileType transFileType);
};

} /* namespace DataStorage */

#endif /* FIRMWARE_APP_DATASTORAGE_FS_H_ */
