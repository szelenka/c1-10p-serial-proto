#include "unity.h"

#include "CRC8.h"


// Test CRC8::calculate_byte for known values
void test_CRC8_CalculateByteKnownValues(void) {
    TEST_ASSERT_EQUAL_HEX8(0x00, CRC8::calculate_byte(0x00));
    TEST_ASSERT_EQUAL_HEX8(0xF3, CRC8::calculate_byte(0xFF));
    TEST_ASSERT_EQUAL_HEX8(0x72, CRC8::calculate_byte(0xA5));
    TEST_ASSERT_EQUAL_HEX8(0x07, CRC8::calculate_byte(0x01));
}

// Test CRC8::calculate with empty data
void test_CRC8_CalculateEmptyData(void) {
    uint8_t data[] = {};
    TEST_ASSERT_EQUAL_HEX8(CRC8::INIT ^ CRC8::XOROUT, crc8.calculate(data, 0));
}

// Test CRC8::calculate with single byte
void test_CRC8_CalculateSingleByte(void) {
    uint8_t data[] = {0xAB};
    TEST_ASSERT_EQUAL_HEX8(CRC8::calculate_byte(0xAB), crc8.calculate(data, 1));
}

// Test CRC8::calculate with multiple bytes
void test_CRC8_CalculateMultipleBytes(void) {
    uint8_t data[] = {0x01, 0x02, 0x03, 0x04};
    TEST_ASSERT_EQUAL_HEX8(0xE3, crc8.calculate(data, 4));
}

// Test CRC8::calculate with all zeros
void test_CRC8_CalculateAllZeros(void) {
    uint8_t data[8] = {0};
    TEST_ASSERT_EQUAL_HEX8(0x00, crc8.calculate(data, 8));
}

// Test CRC8::calculate with all ones
void test_CRC8_CalculateAllOnes(void) {
    uint8_t data[8];
    for (int i = 0; i < 8; ++i) data[i] = 0xFF;
    TEST_ASSERT_EQUAL_HEX8(0xD7, crc8.calculate(data, 8));
}

// Test CRC8::calculate with incremental data
void test_CRC8_CalculateIncrementalData(void) {
    uint8_t data[16];
    for (int i = 0; i < 16; ++i) data[i] = i;
    TEST_ASSERT_EQUAL_HEX8(0x41, crc8.calculate(data, 16));
}

// Test CRC8::calculate with alternating pattern
void test_CRC8_CalculateAlternatingPattern(void) {
    uint8_t data[8] = {0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55};
    TEST_ASSERT_EQUAL_HEX8(0x3F, crc8.calculate(data, 8));
}

int test_crc8_suite(void) {
    UNITY_BEGIN();
    RUN_TEST(test_CRC8_CalculateByteKnownValues);
    RUN_TEST(test_CRC8_CalculateEmptyData);
    RUN_TEST(test_CRC8_CalculateSingleByte);
    RUN_TEST(test_CRC8_CalculateMultipleBytes);
    RUN_TEST(test_CRC8_CalculateAllZeros);
    RUN_TEST(test_CRC8_CalculateAllOnes);
    RUN_TEST(test_CRC8_CalculateIncrementalData);
    RUN_TEST(test_CRC8_CalculateAlternatingPattern);
    return UNITY_END();
}