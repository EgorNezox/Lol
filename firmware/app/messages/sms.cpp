#include "sms.h"
#include <cstdlib>
#include <string.h>
#include <math.h>

sms::sms()
{

}

char *sms::Crc32()
{
   return (char*)0;
}

char *sms::get8Byte()
{
    return (char*)0;
}

// int содержит 0 и 1
void sms::getPolynom(int *input)
{
    int *random = getRandomBits(12);
    memcpy(input,random,12*sizeof(int));
    int *output = (int*) malloc(255*sizeof(int));

    // задать параметры settings
    // 1. Генерируем полином
    gen_poly(&setting);
    // 2. Получаем поле Галуа
    GenerateGaloisField(&setting);
    // 3. Выполняем кодирование
    rs_encode(&setting,input,output,255);
    int *outputs = (int*)malloc(259*sizeof(int));

    for(int i = 0; i<255;i++)
        outputs[i] = output[i];
    for(int i = 255;i<259;i++)
        outputs[i] = getRandomBits(4)[i - 255];

    for(int i = 0; i<37;i++)
    {
        //        memcpy((output_post+i),output,255*sizeof(int));
        //        memcpy((output_post[i]),output,4*sizeof(int));
    }

}


int *sms::getRandomBits(int len)
{
   int *RandomBits = (int*)malloc(len);
   for(int i =0; i<len;i++)
   {
       RandomBits[i] = 0;
   }
   return RandomBits;
}

int sms::getCALL_LCODE(int CYC_N,char *date)
{
    int R_ADR;
    int S_ADR;
    int RN_KEY;

    int SEC;
    int MIN;
    int HRS;
    int DAY;
    int CALL_L_CODE;

    return CALL_L_CODE = fmod(R_ADR + S_ADR + CYC_N + RN_KEY + SEC + MIN + HRS + DAY, 100);

}



