#include "BinToText.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstring>
#include <string>
#include <cstdint>
#include <string>
#include <iostream>
#include <vector>

static const char digit_tens[100] = {
    48, 48, 48, 48, 48, 48, 48, 48, 48, 48,
    49, 49, 49, 49, 49, 49, 49, 49, 49, 49,
    50, 50, 50, 50, 50, 50, 50, 50, 50, 50,
    51, 51, 51, 51, 51, 51, 51, 51, 51, 51,
    52, 52, 52, 52, 52, 52, 52, 52, 52, 52,
    53, 53, 53, 53, 53, 53, 53, 53, 53, 53,
    54, 54, 54, 54, 54, 54, 54, 54, 54 ,54,
    55, 55, 55, 55, 55, 55, 55, 55, 55, 55,
    56, 56, 56, 56, 56, 56, 56, 56, 56, 56,
    57, 57, 57, 57, 57, 57, 57, 57, 57, 57,
};

static const char digit_ones[100] = {
    48, 49, 50, 51, 52, 53, 54, 55, 56, 57,
    48, 49, 50, 51, 52, 53, 54, 55, 56, 57,
    48, 49, 50, 51, 52, 53, 54, 55, 56, 57,
    48, 49, 50, 51, 52, 53, 54, 55, 56, 57,
    48, 49, 50, 51, 52, 53, 54, 55, 56, 57,
    48, 49, 50, 51, 52, 53, 54, 55, 56, 57,
    48, 49, 50, 51, 52, 53, 54, 55, 56, 57,
    48, 49, 50, 51, 52, 53, 54, 55, 56, 57,
    48, 49, 50, 51, 52, 53, 54, 55, 56, 57,
    48, 49, 50, 51, 52, 53, 54, 55, 56, 57,
};

static uint32_t double_fac[] = {1, 10, 100, 1000, 10000, 100000, 1000000, 10000000, 100000000, 1000000000};

void WriteUint8(uint8_t value, char *&buf)
{
    WriteUint32(value, buf);
}

void WriteInt8(int8_t value, char *&buf)
{
    if(value < 0) {
        *buf++ = '-';
        value = -value;
    }
    WriteUint32(static_cast<uint32_t>(value), buf);
}

void WriteUint16(uint16_t value, char *&buf)
{
    WriteUint32(value, buf);
}

void WriteInt16(int16_t value, char *&buf)
{
    if(value < 0) {
        *buf++ = '-';
        value = -value;
    }
    WriteUint32(static_cast<uint32_t>(value), buf);
}

void WriteUint32(uint32_t value, char *&buf)
{
    char s[10];
    char *t = s + sizeof(s);
    while(value >= 65536) {
        unsigned q = value / 100;
        unsigned r = value - ((q << 6) + (q << 5) + (q << 2));
        value = q;
        *--t = digit_ones[r];
        *--t = digit_tens[r];
    }
    do {
        unsigned q = (value * 52429) >> 19;
        unsigned r = value - ((q << 3) + (q << 1));
        *--t = digit_ones[r];
        value = q;
    } while(value);
    size_t len = s + sizeof(s) - t;
    memcpy(buf, t, len);
    buf += len;
}

void WriteInt32(int32_t value, char *&buf)
{
    if (value && value == -value) [[unlikely]] {
        WriteStr("-2147483648", buf);
    } else {
        if(value < 0) {
            *buf++ = '-';
            value = -value;
        }
        WriteUint32(static_cast<uint32_t>(value), buf);
    }
}

void WriteUint64(uint64_t value, char *&buf)
{
    if (value > UINT32_MAX) {
        char s[10];
        char *t = s + sizeof(s);
        do {
            uint64_t q = value / 100;
            auto r = unsigned(value - ((q << 6) + (q << 5) + (q << 2)));
            value = q;
            *--t = digit_ones[r];
            *--t = digit_tens[r];
        } while (value >= UINT32_MAX);
        size_t len = s + sizeof(s) - t;
        memcpy(buf, t, len);
        buf += len;
    }
    WriteUint32((unsigned)value, buf);
}

void WriteInt64(int64_t value, char *&buf)
{
    if (value && value == -value) [[unlikely]] {
        WriteStr("-9223372036854775808", buf);
    } else {
        if(value < 0) {
            *buf++ = '-';
            value = -value;
        }
        WriteUint64((uint64_t)value, buf);
    }
}

void WriteFixPoint(uint64_t value, int nd, char *&buf)
{
    if(value && value == -value) [[unlikely]]
    {
        static const char *digits = "2147483648";
        *buf++ = '-';
        if(nd >= 10) {
            *buf++ = '0';
            *buf++ = '.';
            if(nd -= 10) {
                memset(buf, '0', nd);
                buf += nd; 
            }
            memcpy(buf, digits, 10);
            buf += 10;
        } else {
            int ni = 10 - nd;
            memcpy(buf, digits, ni);
            buf += ni;
            if(nd) {
                *buf++ = '.';
                memcpy(buf, digits + ni, nd);
                buf += nd;
            }
        }
    } else {
        if(value < 0) {
            *buf++ = '-';
            value = -value;
        }
        
        uint64_t fac = double_fac[nd];
        uint64_t ival = value / fac;
        uint64_t fval = value -  ival * fac;
        WriteUint32(ival, buf);
        if(nd) {
            *buf++ = '.';
            buf += nd;
            char *t = buf;
            while(fval >= 65536) {
                unsigned q = fval /100;
                unsigned r = fval - ((q << 6) + (q << 5) + (q << 2));
                fval = q;
                *--t = digit_ones[r];
                *--t = digit_tens[r];
                nd -= 2;
            }
            while(nd--){
                unsigned q = (fval * 52429) >> 19;
                unsigned r = fval - ((q << 3) + (q << 1));
                *--t = digit_ones[r];
                fval = q;
            }
        }
    }
}


void WriteDecimal(double value, int nd, char *&buf)
{
    if(value < 0)
    {
        *buf++ = '-';
        value = -value;
    }
    uint64_t val = llround(value * double_fac[nd]);
    // printf("val = %llu\n", val);
    WriteFixPoint(val, nd, buf);

}

void WriteString(const char *s, char *&buf) {
    size_t len = strlen(s);
    memcpy(buf, s, len);
    buf += len;
}

inline std::string &ltrim(std::string &s) {
    return s.erase(0, s.find_first_not_of(' '));
}

inline std::string &rtrim(std::string &s) {
    return s.erase(s.find_last_not_of(' ') + 1);
}

inline std::string &trim(std::string &s) {
    return ltrim(rtrim(s));
}

inline std::string trim(const std::string &s) {
    int st = -1, ed = -1;
    int size = s.size();
    for (st = 0; st < size && std::isspace(s[st]); ++st) {

    }
    if (st == size) {
        return "";
    }
    for (ed = size - 1; ed >= 0 && std::isspace(s[ed]); --ed) {

    }
    return s.substr(st, ed - st + 1);
}

std::vector<std::string> rrSplit(const std::string &text, char sep) {
    std::vector<std::string> tokens;
    std::string::size_type start = 0, end = 0;
    std::string name;
    while((end = text.find(sep, start)) != std::string::npos) {
        name = text.substr(start, end - start);
        tokens.push_back(trim(name));
        start = end + 1;
    }
    name = text.substr(start);
    tokens.push_back(trim(name));
    return tokens;
}


// int main()
// {
//     char buffer[100];
//     char *buf = buffer;
//     WriteDecimal(123456789.123456, 6, buf);
//     *buf = '\0';
//     printf("%s\n", buffer);
//     return 0;
// }