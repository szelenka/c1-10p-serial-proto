#pragma once

#include <cstdint>
#include <iostream>

class CRC8 {
public:
    // CRC-8 parameters (example, can be changed for different CRCs)
    static const uint8_t POLY = 0x07;
    static const uint8_t INIT = 0x00;
    static const uint8_t XOROUT = 0x00;

    CRC8()
    {
        build_table();
    };

    // Function to calculate CRC-8 for a single byte
    static uint8_t calculate_byte(uint8_t byte)
    {
        uint8_t crc = byte;
        for (int i = 0; i < 8; ++i)
        {
            if (crc & 0x80)
            {
                crc = (crc << 1) ^ POLY;
            }
            else
            {
                crc <<= 1;
            }
        }
        return crc;
    }

    // Function to build CRC-8 lookup table
    void build_table()
    {
        if (!m_table_built)
        {
            for (int i = 0; i < 256; ++i)
            {
                m_table[i] = calculate_byte(i);
            }
            m_table_built = true;
        }
    }

    // Function to calculate CRC-8 for data using lookup table
    uint8_t calculate(const uint8_t* data, size_t len)
    {
        uint8_t crc = INIT;
        for (size_t i = 0; i < len; ++i)
        {
            crc = m_table[crc ^ data[i]];
        }
        return crc ^ XOROUT;
    }

private:
    uint8_t m_table[256] = {0};
    bool m_table_built = false;
};

static CRC8 crc8;
