#include <unity.h>
#include <iostream>
#include <fstream>

#include "pb.h"
#include "pb_encode.h"
#include "pb_decode.h"
#include "c110p_serial.pb.h"

void save_bytes_to_file(const char* filename, const uint8_t* buffer, size_t size) {
    std::string full_filename = std::string("test/encoded_msg/") + filename;
    std::ofstream ofs(full_filename, std::ios::binary);
    ofs.write(reinterpret_cast<const char*>(buffer), size);
    ofs.close();
}

void test_C110PCommand_EncodeDecodeRoundTrip_Led(void)
{
    C110PCommand original_cmd = C110PCommand_init_zero;
    original_cmd.id = 42;
    original_cmd.which_data = C110PCommand_led_tag;
    original_cmd.data.led.start = 1;
    original_cmd.data.led.end = 2;
    original_cmd.data.led.duration = 10;

    uint8_t buffer[1024] = {0};
    pb_ostream_t encodedStream = pb_ostream_from_buffer(buffer, sizeof(buffer));
    bool isEncoded = pb_encode(&encodedStream, C110PCommand_fields, &original_cmd);
    TEST_ASSERT_TRUE(isEncoded);

    // Save bytes to file
    save_bytes_to_file("cmd_led.bin", buffer, encodedStream.bytes_written);

    pb_istream_t decodedStream = pb_istream_from_buffer(buffer, encodedStream.bytes_written);
    C110PCommand decoded_cmd = C110PCommand_init_zero;
    bool isDecoded = pb_decode(&decodedStream, C110PCommand_fields, &decoded_cmd);
    TEST_ASSERT_TRUE(isDecoded);

    TEST_ASSERT_EQUAL(original_cmd.id, decoded_cmd.id);
    TEST_ASSERT_EQUAL(original_cmd.data.led.start, decoded_cmd.data.led.start);
    TEST_ASSERT_EQUAL(original_cmd.data.led.end, decoded_cmd.data.led.end);
    TEST_ASSERT_EQUAL(original_cmd.data.led.duration, decoded_cmd.data.led.duration);
    TEST_ASSERT_EQUAL(original_cmd.which_data, decoded_cmd.which_data);
}

void test_C110PCommand_EncodeDecodeRoundTrip_Ack(void)
{
    C110PCommand original_cmd = C110PCommand_init_zero;
    original_cmd.id = 43;
    original_cmd.which_data = C110PCommand_ack_tag;
    original_cmd.data.ack.acknowledged = true;
    strncpy(original_cmd.data.ack.reason, "Test reason", sizeof(original_cmd.data.ack.reason) - 1);

    uint8_t buffer[1024] = {0};
    pb_ostream_t encodedStream = pb_ostream_from_buffer(buffer, sizeof(buffer));
    bool isEncoded = pb_encode(&encodedStream, C110PCommand_fields, &original_cmd);
    TEST_ASSERT_TRUE(isEncoded);

    // Save bytes to file
    save_bytes_to_file("cmd_ack.bin", buffer, encodedStream.bytes_written);

    pb_istream_t decodedStream = pb_istream_from_buffer(buffer, encodedStream.bytes_written);
    C110PCommand decoded_cmd = C110PCommand_init_zero;
    bool isDecoded = pb_decode(&decodedStream, C110PCommand_fields, &decoded_cmd);
    TEST_ASSERT_TRUE(isDecoded);

    TEST_ASSERT_EQUAL(original_cmd.id, decoded_cmd.id);
    TEST_ASSERT_EQUAL(original_cmd.data.ack.acknowledged, decoded_cmd.data.ack.acknowledged);
    TEST_ASSERT_EQUAL_STRING(original_cmd.data.ack.reason, decoded_cmd.data.ack.reason);
    TEST_ASSERT_EQUAL(original_cmd.which_data, decoded_cmd.which_data);
}


void test_C110PCommand_EncodeDecodeRoundTrip_Ack_reason_none(void)
{
    C110PCommand original_cmd = C110PCommand_init_zero;
    original_cmd.id = 46;
    original_cmd.which_data = C110PCommand_ack_tag;
    original_cmd.data.ack.acknowledged = false;
    strncpy(original_cmd.data.ack.reason, "", sizeof(original_cmd.data.ack.reason) - 1);

    uint8_t buffer[1024] = {0};
    pb_ostream_t encodedStream = pb_ostream_from_buffer(buffer, sizeof(buffer));
    bool isEncoded = pb_encode(&encodedStream, C110PCommand_fields, &original_cmd);
    TEST_ASSERT_TRUE(isEncoded);

    // Save bytes to file
    save_bytes_to_file("cmd_ack_reason_none.bin", buffer, encodedStream.bytes_written);

    pb_istream_t decodedStream = pb_istream_from_buffer(buffer, encodedStream.bytes_written);
    C110PCommand decoded_cmd = C110PCommand_init_zero;
    bool isDecoded = pb_decode(&decodedStream, C110PCommand_fields, &decoded_cmd);
    TEST_ASSERT_TRUE(isDecoded);

    TEST_ASSERT_EQUAL(original_cmd.id, decoded_cmd.id);
    TEST_ASSERT_EQUAL(original_cmd.data.ack.acknowledged, decoded_cmd.data.ack.acknowledged);
    TEST_ASSERT_EQUAL_STRING(original_cmd.data.ack.reason, decoded_cmd.data.ack.reason);
    TEST_ASSERT_EQUAL(original_cmd.which_data, decoded_cmd.which_data);
}

void test_C110PCommand_EncodeDecodeRoundTrip_Movement(void)
{
    C110PCommand original_cmd = C110PCommand_init_zero;
    original_cmd.id = 44;
    original_cmd.which_data = C110PCommand_move_tag;
    original_cmd.data.move.target = C110PActuator_BODY_NECK;
    original_cmd.data.move.x = 100;
    original_cmd.data.move.y = 200;
    original_cmd.data.move.z = 300;

    uint8_t buffer[1024] = {0};
    pb_ostream_t encodedStream = pb_ostream_from_buffer(buffer, sizeof(buffer));
    bool isEncoded = pb_encode(&encodedStream, C110PCommand_fields, &original_cmd);
    TEST_ASSERT_TRUE(isEncoded);

    // Save bytes to file
    save_bytes_to_file("cmd_move.bin", buffer, encodedStream.bytes_written);

    pb_istream_t decodedStream = pb_istream_from_buffer(buffer, encodedStream.bytes_written);
    C110PCommand decoded_cmd = C110PCommand_init_zero;
    bool isDecoded = pb_decode(&decodedStream, C110PCommand_fields, &decoded_cmd);
    TEST_ASSERT_TRUE(isDecoded);

    TEST_ASSERT_EQUAL(original_cmd.id, decoded_cmd.id);
    TEST_ASSERT_EQUAL(original_cmd.data.move.target, decoded_cmd.data.move.target);
    TEST_ASSERT_EQUAL(original_cmd.data.move.x, decoded_cmd.data.move.x);
    TEST_ASSERT_EQUAL(original_cmd.data.move.y, decoded_cmd.data.move.y);
    TEST_ASSERT_EQUAL(original_cmd.data.move.z, decoded_cmd.data.move.z);
    TEST_ASSERT_EQUAL(original_cmd.which_data, decoded_cmd.which_data);
}

void test_C110PCommand_EncodeDecodeRoundTrip_Sound(void)
{
    C110PCommand original_cmd = C110PCommand_init_zero;
    original_cmd.id = 45;
    original_cmd.which_data = C110PCommand_sound_tag;
    original_cmd.data.sound.id = 7;
    original_cmd.data.sound.play = true;
    original_cmd.data.sound.syncToLeds = false;

    uint8_t buffer[1024] = {0};
    pb_ostream_t encodedStream = pb_ostream_from_buffer(buffer, sizeof(buffer));
    bool isEncoded = pb_encode(&encodedStream, C110PCommand_fields, &original_cmd);
    TEST_ASSERT_TRUE(isEncoded);

    // Save bytes to file
    save_bytes_to_file("cmd_sound.bin", buffer, encodedStream.bytes_written);

    pb_istream_t decodedStream = pb_istream_from_buffer(buffer, encodedStream.bytes_written);
    C110PCommand decoded_cmd = C110PCommand_init_zero;
    bool isDecoded = pb_decode(&decodedStream, C110PCommand_fields, &decoded_cmd);
    TEST_ASSERT_TRUE(isDecoded);

    TEST_ASSERT_EQUAL(original_cmd.id, decoded_cmd.id);
    TEST_ASSERT_EQUAL(original_cmd.data.sound.id, decoded_cmd.data.sound.id);
    TEST_ASSERT_EQUAL(original_cmd.data.sound.play, decoded_cmd.data.sound.play);
    TEST_ASSERT_EQUAL(original_cmd.data.sound.syncToLeds, decoded_cmd.data.sound.syncToLeds);
    TEST_ASSERT_EQUAL(original_cmd.which_data, decoded_cmd.which_data);
}

int test_pb_suite(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_C110PCommand_EncodeDecodeRoundTrip_Led);
    RUN_TEST(test_C110PCommand_EncodeDecodeRoundTrip_Ack);
    RUN_TEST(test_C110PCommand_EncodeDecodeRoundTrip_Ack_reason_none);
    RUN_TEST(test_C110PCommand_EncodeDecodeRoundTrip_Movement);
    RUN_TEST(test_C110PCommand_EncodeDecodeRoundTrip_Sound);
    return UNITY_END();
}