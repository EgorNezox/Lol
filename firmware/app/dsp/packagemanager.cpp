#include "packagemanager.h"
#include "string.h"

PackageManager::PackageManager()
{
    //makeTable(CRC_POLY1);
}

void PackageManager::ConvertBit(uint8_t src, bool dest[7])
{
    for(int i = 0; i < 7; i++) {
        uint8_t temp = (src & (1 << i)) >> i;
        dest[i] = (bool)temp;
//        dest[7 - i] = (bool)((src & (1 << i)) >> i);
    }
}

void PackageManager::ConvertByte(bool* data, uint8_t& sum)
{
    for(int i = 0; i < 7; i++)
        if (data[i])
            sum += (uint8_t)pow(2, 6-i);
}

// 700 simbols
void PackageManager::Text(uint8_t *message, uint8_t* sms,int index)
{

    bool mes[704];

    for(int i = 0; i<index;i++)
    {
        bool rc[7] = {0,0,0,0,0,0,0};
        ConvertBit(message[i], rc);
        memcpy(&mes[i*7], rc, 7); // по 7 символов
    }

    if ((7*index) < 700)
        for(int i = 7*index; i<700;i++)
            mes[i] = 0;

    for(int i = 700; i<704;i++)
        mes[i] = 0; // последние нули


    for(int i = 0; i < 88; i++)
    {
        bool str[8];
        memcpy(&str,&mes[i*8] ,8);
        ConvertByte(str, sms[i]);
    }
}


void PackageManager::makeTable(int polynomChoosen)
{
    const unsigned int CRC_POLY1 = 0xEDB88320; //is used in Total Commander
    unsigned int i, j, r;

    for (i = 0; i < 256; i++)
    {
        for (r = i, j = 8; j; j--)
            r = r & 1? (r >> 1) ^ CRC_POLY1: r >> 1;
        table[i] = r;
    }

}


unsigned int PackageManager::CRC32(unsigned char* pData,  int len)
{
    const unsigned int CRC_MASK = 0xD202EF8D;
    unsigned int crc = 0;
    while (len--)
    {
        crc = table[(unsigned char)crc ^ (* pData ++)] ^ crc >> 8;
        crc ^= CRC_MASK;
    }
    return crc;
}
