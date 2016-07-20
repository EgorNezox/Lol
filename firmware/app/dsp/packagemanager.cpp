#include <string.h>
#include "packagemanager.h"
#include "ui/texts.h"

PackageManager::PackageManager()
{
    makeTable(CRC_POLY1);
    koi_sim.insert(std::pair<char,int>(ch_key1[0],46));
    koi_sim.insert(std::pair<char,int>(ch_key1[1],44));
    koi_sim.insert(std::pair<char,int>(ch_key1[2],33));
    koi_sim.insert(std::pair<char,int>(ch_key1[3],63));
    koi_sim.insert(std::pair<char,int>(ch_key1[4],34));
    koi_sim.insert(std::pair<char,int>(ch_key1[5],58));
    koi_sim.insert(std::pair<char,int>(ch_key2[0],97));
    koi_sim.insert(std::pair<char,int>(ch_key2[1],98));
    koi_sim.insert(std::pair<char,int>(ch_key2[2],119));
    koi_sim.insert(std::pair<char,int>(ch_key2[3],103));
    koi_sim.insert(std::pair<char,int>(ch_key3[0],100));
    koi_sim.insert(std::pair<char,int>(ch_key3[1],101));
    koi_sim.insert(std::pair<char,int>(ch_key3[2],118));
    koi_sim.insert(std::pair<char,int>(ch_key3[3],122));
    koi_sim.insert(std::pair<char,int>(ch_key4[0],105));
    koi_sim.insert(std::pair<char,int>(ch_key4[1],106));
    koi_sim.insert(std::pair<char,int>(ch_key4[2],107));
    koi_sim.insert(std::pair<char,int>(ch_key4[3],108));
    koi_sim.insert(std::pair<char,int>(ch_key5[0],109));
    koi_sim.insert(std::pair<char,int>(ch_key5[1],110));
    koi_sim.insert(std::pair<char,int>(ch_key5[2],111));
    koi_sim.insert(std::pair<char,int>(ch_key5[3],112));
    koi_sim.insert(std::pair<char,int>(ch_key6[0],114));
    koi_sim.insert(std::pair<char,int>(ch_key6[1],115));
    koi_sim.insert(std::pair<char,int>(ch_key6[2],116));
    koi_sim.insert(std::pair<char,int>(ch_key6[3],117));
    koi_sim.insert(std::pair<char,int>(ch_key7[0],102));
    koi_sim.insert(std::pair<char,int>(ch_key7[1],104));
    koi_sim.insert(std::pair<char,int>(ch_key7[2],99));
    koi_sim.insert(std::pair<char,int>(ch_key7[3],126));
    koi_sim.insert(std::pair<char,int>(ch_key8[0],123));
    koi_sim.insert(std::pair<char,int>(ch_key8[1],125));
    koi_sim.insert(std::pair<char,int>(ch_key8[2],-1)); // ъ знака нет
    koi_sim.insert(std::pair<char,int>(ch_key8[3],121));
    koi_sim.insert(std::pair<char,int>(ch_key9[0],120));
    koi_sim.insert(std::pair<char,int>(ch_key9[1],124));
    koi_sim.insert(std::pair<char,int>(ch_key9[2],96));
    koi_sim.insert(std::pair<char,int>(ch_key9[3],113));
    koi_sim.insert(std::pair<char,int>(ch_key0[0],32));
    koi_sim.insert(std::pair<char,int>('1',49));
    koi_sim.insert(std::pair<char,int>('2',50));
    koi_sim.insert(std::pair<char,int>('3',51));
    koi_sim.insert(std::pair<char,int>('4',52));
    koi_sim.insert(std::pair<char,int>('5',53));
    koi_sim.insert(std::pair<char,int>('6',54));
    koi_sim.insert(std::pair<char,int>('7',55));
    koi_sim.insert(std::pair<char,int>('8',56));
    koi_sim.insert(std::pair<char,int>('9',57));
    koi_sim.insert(std::pair<char,int>(ch_key0[1],48));

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
        memcpy(&mes[i*7], rc, 7); // РїРѕ 7 СЃРёРјРІРѕР»РѕРІ
    }

    if ((7*index) < 700)
        for(int i = 7*index; i<700;i++)
            mes[i] = 0;

    for(int i = 700; i<704;i++)
        mes[i] = 0; // РїРѕСЃР»РµРґРЅРёРµ РЅСѓР»Рё


    for(int i = 0; i < 88; i++)
    {
        bool str[8];
        memcpy(&str,&mes[i*8] ,8);
        ConvertByte(str, sms[i]);
    }
}

void PackageManager::UnpackText(uint8_t *message,uint8_t *source)
{
	int cnt = 7;

	for(int i = 0; i<100;i++)
	 {
		source[i] = source[i] & 0x7F;

		for(int j = 0; j < 7;j++)
		{
		  source[cnt] = message[i] & (1 >> j);
		  ++cnt;
		}
	 }
}

void PackageManager::PackText(uint8_t *message, uint8_t* source)
{
	int cnt = 0;
	int index = 0;

	for(int i = 0; i<100;i++)
	{
		for(int j  = 0; j<8;j++)
		{
			int shift  = index % 7;
			source[index/8] = (source[index/8] | (message[i] & (1 << shift))); // 1-7 бит пишем в
			++index;
		}
	}
}



int PackageManager::decompressMass(uint8_t *srcBuf,
                                 uint16_t srcLen,
								 uint8_t *dstBuf,
								 uint16_t  dstLen,
								 uint16_t  numBits)
{
    // вспомогательная переменная для формирования готовых байтов
	uint8_t   byte, b1;
    // вспомогательная переменная номер бита в готовом байте
	uint8_t   nBit;

	uint16_t lenRet;

    if (dstLen < ((srcLen*8/numBits)))
        return -1;

    // обнуляем переменную конечной длины массива
    lenRet = 0;
    nBit = 0;
    byte = 0;
    uint16_t  nByte;
    uint16_t  bit;

    for (nByte = 0; nByte < srcLen; nByte++)
    {
        b1 = srcBuf[nByte];
        // побитно перебираем каждый байт собирая с 0 по 7 биты
        for (bit = 0; bit < 8; bit++)
        {
            // берём по одному биту, в скобках получаем 0 или 1
            // и сдвигаем на нужную позицию в конечном байте
            if (b1 & (1 << bit))
                byte |= (1 << nBit);
            // инкрементируем номер бита
            nBit++;
            // если позиция бита вылазит за границу значащих битов то байт готов
            if (nBit >= numBits)
            {
                // записываем его в тот же массив
                dstBuf[lenRet] = byte;
                // инкрементируем номер готового байта (длину конечного массива)
                lenRet++;
                byte = 0;
                nBit = 0;
            }
        }
    }

    // необходимо проверить есть ли у нас не записанные символы
    if (nBit)
    {
        // записываем его в тот же массив
        dstBuf[lenRet] = byte;
        // инкрементируем номер готового байта (длину конечного массива)
        lenRet++;
    }

    return lenRet;
}


int PackageManager::compressMass(uint8_t *srcBuf,int srcLen,int numBits)
{
    // вспомогательная переменная для формирования готовых байтов
	uint8_t  byte, b1;
    // вспомогательная переменная номер бита в готовом байте
	uint8_t  nBit;

	uint16_t lenDst;

    // обнуляем переменную конечной длины массива
    lenDst = 0;
    nBit = 0;
    byte = 0;
    uint16_t nByte;
    uint16_t bit;
    // отбрасываем нулевые биты во всём массиве
    for (nByte = 0; nByte < srcLen; nByte++)
    {
        b1 = srcBuf[nByte];
        // побитно перебираем каждый байт собирая с 0 по numBits биты
        for (bit = 0; bit < numBits; bit++)
        {
            // берём по одному биту, в скобках получаем 0 или 1
            // и сдвигаем на нужную позицию в конечном байте
            if (b1 & (1 << bit))
                byte |= (1 << nBit);
            // инкрементируем номер бита
            nBit++;
            // если позиция бита вылазит за границу байта то байт готов
            if (nBit >= 8)
            {
                // записываем его в тот же массив
                srcBuf[lenDst] = byte;
                // инкрементируем номер готового байта (длину конечного массива)
                lenDst++;
                byte = 0;
                nBit = 0;
            }
        }
    }

    // необходимо проверить есть ли у нас не записанные символы
    if (nBit)
    {
        // записываем его в тот же массив
        srcBuf[lenDst] = byte;
        // инкрементируем номер готового байта (длину конечного массива)
        lenDst++;
    }

    return lenDst;
}

void PackageManager::to_Koi7(uint8_t *message)
{
	int i = 0;
	while(message[i] != '\0')
	{
		auto it = koi_sim.find((char)message[i]);

		if (it != koi_sim.end())
			message[i] = it->second;
		++i;
	}
}


void PackageManager::to_Win1251(uint8_t *message)
{
	int i = 0;
		while(message[i] != '\0')
		{
			for(auto &it: koi_sim)
			{
				if (it.second == message[i]){
				message[i] = it.first;
				break;
				}
			}
			++i;
        }
}


// функция для сдвига 8-битного массива в 7-ми битный,
// input - входной массив
// output - выходной масисив,
// out_shift - номер элемента, с которого просходит добавление в выходной массив
// len - длинна
void PackageManager::shiftMasTo7Bit(uint8_t *input, uint8_t *output, int out_shift, int len)
{
    int sdvig = 0;
    for(int i = 0; i< len; i++)
    {
        sdvig = (i + 1) % 8;
        if (sdvig != 0)
            output[i + out_shift] = (input[i] << sdvig) + (input[i] >> sdvig);
        else
            output[i + out_shift] = input[i];
    }
}

void PackageManager::addBytetoBitsArray(uint8_t value, std::vector<bool> &data,int len)
{
    bool param;
    for(int i = 0; i< len; i++)
    {
        param = value & (1 << (len-i-1));
        data.push_back(param);
    }
}

void PackageManager::getArrayByteFromBit(std::vector<bool> &data, uint8_t *dest)
{
    int len  = data.size();
    int ost = len  % 8;
    if (ost != 0 ){ for(int i = 0; i< 8 - ost;i++) data.push_back(0);}

    int j = 0;int cnt = 0; uint8_t res = 0;
    for(int i = 0; i < data.size(); i++) {
        if ((i % 8) == 0 && (i !=0)) { dest[j] = res; ++j; cnt = 0; res = 0; }
        res += data.at(i) << (8 - cnt - 1);
        ++cnt;
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
