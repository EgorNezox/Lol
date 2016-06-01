/**
 ******************************************************************************
 * @file    qmcrc.h
 * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
 * @date    18.01.2016
 *
 * Реализация различных алгоритмов вычисления контрольных сумм CRC по методике и классификации
 * Ross N. Williams (статья "A painless guide to CRC error detection algorithms", version 3, 19 авг. 1993)
 * с использованием высокоскоростного табличного подхода.
 * В данной реализации параметризация ограничена алгоритмами, у которых refin=refout.
 *
 ******************************************************************************
 */

#ifndef QMCRC_H_
#define QMCRC_H_

#include <stdint.h>

template <
typename ValueType,
int width,			/* width in bits [8,32] */
uint32_t poly,		/* the algorithm's polynomial */
uint32_t init,		/* initial register value (normal) */
bool reflected,		/* reflect input bytes and output CRC ? */
uint32_t xorout		/* XOR this to output CRC */
>
class QmCrc {
public:
	QmCrc() {
		if (!precalc_initialized)
			initPrecalc();
		value = (reflected)?(precalc_init_reflected):init;
	}
	void update(const unsigned char *data, unsigned long size) {
		if (reflected) {
			while (size--)
				value = precalc_table[(value ^ *data++) & 0xFFL] ^ ((value >> 8)*precalc_xor);
		} else {
			while (size--)
				value = precalc_table[((value >> precalc_offset) ^ *data++) & 0xFFL] ^ ((value << 8)*precalc_xor);
		}
	}
	inline ValueType result() {
		return (value ^ xorout);
	}
private:
	ValueType value;
	static bool precalc_initialized;
	static uint32_t precalc_init_reflected;
	static uint8_t precalc_offset;
	static bool precalc_xor;
	static uint32_t precalc_table[];

private:
	static inline uint32_t bitmask(int offset) {
		return (1L << offset);
	}
	static uint32_t reflect(uint32_t value, int bitscount) {
		uint32_t t = value;
		for (int i = 0; i < bitscount; i++) {
			if (t & 1L)
				value |= bitmask((bitscount-1)-i);
			else
				value &= ~bitmask((bitscount-1)-i);
			t >>= 1;
		}
		return value;
	}
	static void __attribute__((constructor)) initPrecalc() {
		precalc_init_reflected = reflect(init, width);
		switch (width) {
		case 8:
			precalc_offset = 0;
			precalc_xor = false;
			break;
		case 16:
			precalc_offset = 8;
			precalc_xor = true;
			break;
		case 32:
			precalc_offset = 24;
			precalc_xor = true;
			break;
		}
		for (uint32_t i = 0; i < 256; i++) {
			uint32_t topbit = bitmask(width-1);
			uint32_t inbyte = i;
			if (reflected)
				inbyte = reflect(inbyte, 8);
			uint32_t value = inbyte << (width-8);
			for (int j = 0; j < 8; j++) {
				if (value & topbit)
					value = (value << 1) ^ poly;
				else
					value <<= 1;
			}
			if (reflected)
				value = reflect(value, width);
			precalc_table[i] = value & ((((1UL<<(width-1))-1)<<1)|1UL);
		}
		precalc_initialized = true;
	}
};
template <typename ValueType, int width, uint32_t poly, uint32_t init, bool reflected, uint32_t xorout>
bool QmCrc<ValueType, width, poly, init, reflected, xorout>::precalc_initialized = false;
template <typename ValueType, int width, uint32_t poly, uint32_t init, bool reflected, uint32_t xorout>
uint32_t QmCrc<ValueType, width, poly, init, reflected, xorout>::precalc_init_reflected;
template <typename ValueType, int width, uint32_t poly, uint32_t init, bool reflected, uint32_t xorout>
uint8_t QmCrc<ValueType, width, poly, init, reflected, xorout>::precalc_offset;
template <typename ValueType, int width, uint32_t poly, uint32_t init, bool reflected, uint32_t xorout>
bool QmCrc<ValueType, width, poly, init, reflected, xorout>::precalc_xor;
template <typename ValueType, int width, uint32_t poly, uint32_t init, bool reflected, uint32_t xorout>
uint32_t QmCrc<ValueType, width, poly, init, reflected, xorout>::precalc_table[256];

#endif /* QMCRC_H_ */
