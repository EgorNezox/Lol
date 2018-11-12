//---------------------------------------
// Модуль фильтра для блока синхронизации
//---------------------------------------
//#include <float.h>
#include <math.h>
#include <stdbool.h>//----------------------------!!!!!!!!!!!!!!!!!
#include "filter.h"

static float k, delay1, delay2;     //коэффициент коррекции и состояние фильтра
static float t_buff;                // интегратор подсчета величины коррекции
static unsigned long n;             // число подряд следующих нулевых разностей
static bool first;                  // индикатор начального состояния вычислителя разности
static bool start_mode;             //индикатор состояния прогрева
static unsigned long start_counter; //таймер прогрева;
const long start_timeout = 2; //60 * 5;  //таймаут прогрева в сек
const long sw_timeout = 60 * 10;    //таймаут блокировки подстройки по GPS
bool old_gps = false;               // достоверность предидущей метки

//коэффициенты фильтра
static const float b[3] = { 1, 2, 1 };
static const float a[3] = { 1, -1.907501578, 0.9115945101 };
static const float gain = 0.001023217686;
//static koeff_reg = 0.2;
static const float sw_coeff = 0.05 * 12000000;
static const float gps_coeff = 0.01;
//внутренние функции модуля
static float filter_df2t(float aT);
static float filter(float d);

void init_corrector(corr_STATE *st)
{
	n = 0;
	first = true;
	k = st->k;
	delay1 = st->delay1;
	delay2 = st->delay2;
	t_buff = 0;
	start_mode = true;
	start_counter = start_timeout;
}

void get_corrector_state(corr_STATE* sr)
{
	sr->k = k;
	sr->delay1 = delay1;
	sr->delay2 = delay2;

}

float get_corrector_coeff(void)
{
	return (k);
}

float calculate_coeff(int diff, bool last_gps)
{
	float f, d;

	d = (float) diff * gps_coeff;

	if (start_mode)
	{
		if (start_counter == 0)
			start_mode = false;
		else
			start_counter--;
	}
	else
	{
		if (last_gps && old_gps)
		{
			f = filter((float)d);
			k = k + f;
		}
	}
	old_gps = last_gps;
	return k;
}

float calculate_coeff_sw(int diff,int frec_hz)
{
	float d ,f;
	d = (float) diff * sw_coeff / (frec_hz);
	start_mode = true;
	start_counter = sw_timeout;

	f = filter(d); // USE d, not diff
	k = k + f;
	return k;
}

// вычисление погрешности и ее усреднение
static float filter(float d)
{
	float r;
	r = filter_df2t(d);
	return r;
}

// direct form2 soc filter
static float filter_df2t(float aT)
{
	volatile float t = aT;
	volatile float p;
	t = t - delay1 * a[1];
	t = t - delay2 * a[2];
	p = t + delay1 * b[1];
	p = p + delay2 * b[2];
	delay2 = delay1;
	delay1 = t;
	p = p * gain;
	return p;
}
