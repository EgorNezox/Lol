/**
  ******************************************************************************
  * @file    qmendian.h
  * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
  * @date    24.12.2015
  *
  ******************************************************************************
 */

#ifndef QMENDIAN_H_
#define QMENDIAN_H_

#include <stdint.h>
#include <string.h>

inline void qmbswap_helper(const uint8_t *src, uint8_t *dest, int size)
{
    for (int i = 0; i < size ; ++i)
    	dest[i] = src[size - 1 - i];
}

/*
 * qmbswap(const T src, const uint8_t *dest);
 * Changes the byte order of \a src from big endian to little endian or vice versa
 * and stores the result in \a dest.
 * There is no alignment requirements for \a dest.
*/
template <typename T> inline void qmbswap(const T src, uint8_t *dest)
{
    qmbswap_helper(reinterpret_cast<const uint8_t *>(&src), dest, sizeof(T));
}

// Used to implement a type-safe and alignment-safe copy operation
// If you want to avoid the memcopy, you must write specializations for this function
template <typename T> inline void qmToUnaligned(const T src, uint8_t *dest)
{
    // Using sizeof(T) inside memcpy function produces internal compiler error with
    // MSVC2008/ARM in tst_endian -> use extra indirection to resolve size of T.
    const size_t size = sizeof(T);
    memcpy(dest, &src, size);
}

/* T qmFromLittleEndian(const uint8_t *src)
 * This function will read a little-endian encoded value from \a src
 * and return the value in host-endian encoding.
 * There is no requirement that \a src must be aligned.
*/
template <typename T> inline T qmFromLittleEndian(const uint8_t *src);
template <> inline uint64_t qmFromLittleEndian<uint64_t>(const uint8_t *src)
{
    return 0
        | src[0]
        | src[1] * 0x0000000000000100ULL
        | src[2] * 0x0000000000010000ULL
        | src[3] * 0x0000000001000000ULL
        | src[4] * 0x0000000100000000ULL
        | src[5] * 0x0000010000000000ULL
        | src[6] * 0x0001000000000000ULL
        | src[7] * 0x0100000000000000ULL;
}
template <> inline uint32_t qmFromLittleEndian<uint32_t>(const uint8_t *src)
{
    return 0
        | src[0]
        | src[1] * uint32_t(0x00000100)
        | src[2] * uint32_t(0x00010000)
        | src[3] * uint32_t(0x01000000);
}
template <> inline uint16_t qmFromLittleEndian<uint16_t>(const uint8_t *src)
{
    return uint16_t(0
                   | src[0]
                   | src[1] * 0x0100);
}

// signed specializations
template <> inline int64_t qmFromLittleEndian<int64_t>(const uint8_t *src)
{ return static_cast<int64_t>(qmFromLittleEndian<uint64_t>(src)); }
template <> inline int32_t qmFromLittleEndian<int32_t>(const uint8_t *src)
{ return static_cast<int32_t>(qmFromLittleEndian<uint32_t>(src)); }
template <> inline int16_t qmFromLittleEndian<int16_t>(const uint8_t *src)
{ return static_cast<int16_t>(qmFromLittleEndian<uint16_t>(src)); }

// no-conversion specializations
template <> inline uint8_t qmFromLittleEndian<uint8_t>(const uint8_t *src)
{ return static_cast<uint8_t>(src[0]); }
template <> inline int8_t qmFromLittleEndian<int8_t>(const uint8_t *src)
{ return static_cast<int8_t>(src[0]); }

/* This function will read a big-endian (also known as network order) encoded value from \a src
 * and return the value in host-endian encoding.
 * There is no requirement that \a src must be aligned.
*/
template <class T> inline T qmFromBigEndian(const uint8_t *src);
template <> inline uint64_t qmFromBigEndian<uint64_t>(const uint8_t *src)
{
    return 0
        | src[7]
        | src[6] * 0x0000000000000100ULL
        | src[5] * 0x0000000000010000ULL
        | src[4] * 0x0000000001000000ULL
        | src[3] * 0x0000000100000000ULL
        | src[2] * 0x0000010000000000ULL
        | src[1] * 0x0001000000000000ULL
        | src[0] * 0x0100000000000000ULL;
}
template <> inline uint32_t qmFromBigEndian<uint32_t>(const uint8_t *src)
{
    return 0
        | src[3]
        | src[2] * uint32_t(0x00000100)
        | src[1] * uint32_t(0x00010000)
        | src[0] * uint32_t(0x01000000);
}
template <> inline uint16_t qmFromBigEndian<uint16_t>(const uint8_t *src)
{
    return uint16_t( 0
                    | src[1]
                    | src[0] * uint16_t(0x0100));
}

// signed specializations
template <> inline int64_t qmFromBigEndian<int64_t>(const uint8_t *src)
{ return static_cast<int64_t>(qmFromBigEndian<uint64_t>(src)); }
template <> inline int32_t qmFromBigEndian<int32_t>(const uint8_t *src)
{ return static_cast<int32_t>(qmFromBigEndian<uint32_t>(src)); }
template <> inline int16_t qmFromBigEndian<int16_t>(const uint8_t *src)
{ return static_cast<int16_t>(qmFromBigEndian<uint16_t>(src)); }

// no-conversion specializations
template <> inline uint8_t qmFromBigEndian<uint8_t>(const uint8_t *src)
{ return static_cast<uint8_t>(src[0]); }
template <> inline int8_t qmFromBigEndian<int8_t>(const uint8_t *src)
{ return static_cast<int8_t>(src[0]); }

/*
 * T qmbswap(T source).
 * Changes the byte order of a value from big endian to little endian or vice versa.
 * This function can be used if you are not concerned about alignment issues,
 * and it is therefore a bit more convenient and in most cases more efficient.
*/
template <typename T> T qmbswap(T source);
template <> inline uint64_t qmbswap<uint64_t>(uint64_t source)
{
    return 0
        | ((source & 0x00000000000000ffULL) << 56)
        | ((source & 0x000000000000ff00ULL) << 40)
        | ((source & 0x0000000000ff0000ULL) << 24)
        | ((source & 0x00000000ff000000ULL) << 8)
        | ((source & 0x000000ff00000000ULL) >> 8)
        | ((source & 0x0000ff0000000000ULL) >> 24)
        | ((source & 0x00ff000000000000ULL) >> 40)
        | ((source & 0xff00000000000000ULL) >> 56);
}
template <> inline uint32_t qmbswap<uint32_t>(uint32_t source)
{
    return 0
        | ((source & 0x000000ff) << 24)
        | ((source & 0x0000ff00) << 8)
        | ((source & 0x00ff0000) >> 8)
        | ((source & 0xff000000) >> 24);
}
template <> inline uint16_t qmbswap<uint16_t>(uint16_t source)
{
    return uint16_t( 0
                    | ((source & 0x00ff) << 8)
                    | ((source & 0xff00) >> 8) );
}

// signed specializations
template <> inline int64_t qmbswap<int64_t>(int64_t source)
{ return qmbswap<uint64_t>(uint64_t(source)); }
template <> inline int32_t qmbswap<int32_t>(int32_t source)
{ return qmbswap<uint32_t>(uint32_t(source)); }
template <> inline int16_t qmbswap<int16_t>(int16_t source)
{ return qmbswap<uint16_t>(uint16_t(source)); }

// no-conversion specializations
template <> inline uint8_t qmbswap<uint8_t>(uint8_t source)
{ return source; }
template <> inline int8_t qmbswap<int8_t>(int8_t source)
{ return source; }

#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__

template <typename T> inline T qmToBigEndian(T source)
{ return source; }
template <typename T> inline T qmFromBigEndian(T source)
{ return source; }
template <typename T> inline T qmToLittleEndian(T source)
{ return qmbswap<T>(source); }
template <typename T> inline T qmFromLittleEndian(T source)
{ return qmbswap<T>(source); }
template <typename T> inline void qmToBigEndian(T src, uint8_t *dest)
{ qmToUnaligned<T>(src, dest); }
template <typename T> inline void qmToLittleEndian(T src, uint8_t *dest)
{ qmbswap<T>(src, dest); }

#elif __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__

template <typename T> inline T qmToBigEndian(T source)
{ return qmbswap<T>(source); }
template <typename T> inline T qmFromBigEndian(T source)
{ return qmbswap<T>(source); }
template <typename T> inline T qmToLittleEndian(T source)
{ return source; }
template <typename T> inline T qmFromLittleEndian(T source)
{ return source; }
template <typename T> inline void qmToBigEndian(T src, uint8_t *dest)
{ qmbswap<T>(src, dest); }
template <typename T> inline void qmToLittleEndian(T src, uint8_t *dest)
{ qmToUnaligned<T>(src, dest); }

#else
#error "Cannot detect supported processor endianess"
#endif // __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__

#endif /* QMENDIAN_H_ */
