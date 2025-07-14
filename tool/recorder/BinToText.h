#pragma once

#include <cmath>
#include <cstdint>

constexpr inline double RadianToDegree(double radian) {
    return radian * (180.0 / M_PI);
}

void WriteUint8(uint8_t value, char *&buffer);
void WriteUint16(uint16_t value, char *&buffer);
void WriteUint32(uint32_t value, char *&buffer);
void WriteUint64(uint64_t value, char *&buffer);
void WriteInt8(int8_t value, char *&buffer);
void WriteInt16(int16_t value, char *&buffer);
void WriteInt32(int32_t value, char *&buffer);
void WriteInt64(int64_t value, char *&buffer);

void WriteDecimal(double value, int nd, char *&buffer);
void WriteFixPoint(uint64_t val, int nd,char *&buffer);

void WriteString(const char *s, char *&buf);

#define WriteStr(str, buf) \
    do { \
        memcpy(buf, str, sizeof(str)-1); \
        buf += sizeof(str) - 1; \
    } while (0)

#define REP(i,n) for (int32_t i = 0; i < (n); ++i)

#define FOR(i, m, n) for(int32_t i = (m); i < (n); ++i)

template<class T>
inline void checkmin(T &a, T b) {
    if (b < a) a = b;
}

template<class T>
inline void checkmax(T &a, T b) {
    if (b > a) a = b;
}