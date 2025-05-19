#include <unity.h>
#include <Arduino.h>

#include "C110PSerial.h"

#include "pb.h"
#include "pb_encode.h"
#include "pb_decode.h"
#include <set>

using namespace fakeit;

// Helper: Create a valid C110PCommand message
C110PCommand createValidMsg(uint32_t timestamp = 1234)
{
    C110PCommand msg = C110PCommand_init_default;
    msg.id = timestamp;
    msg.which_data = C110PCommand_led_tag;
    msg.data.led = LedCommand_init_default;
    msg.data.led.start = 1;
    msg.data.led.end = 2;
    msg.data.led.duration = 10;
    return msg;
}

void test_send_successful(void)
{
    std::vector<uint8_t> written;

    // Get the Stream mock object (from ArduinoFake)
    Stream* streamPtr = ArduinoFakeMock(Stream);
    C110PSerial protoSerial(streamPtr);

    uint32_t timestamp = 1001;
    C110PCommand msg = createValidMsg(timestamp);

    When(OverloadedMethod(ArduinoFake(Stream), write, size_t(uint8_t)).Using(Any<uint8_t>()))
        .AlwaysDo([&written](uint8_t val) {
            std::cout << "Stream::write(uint8_t): " << static_cast<int>(val) << std::endl;
            written.push_back(val);
            return 1;
        });
    When(OverloadedMethod(ArduinoFake(Stream), write,  size_t(const uint8_t*, size_t)))
        .AlwaysDo([&written](const uint8_t* data, size_t len) {
            for (size_t i = 0; i < len; ++i) {
                written.push_back(data[i]);
            }
            return len;
        });

    // #define LOG_VAR(msg, var) \
    //     std::cout << (msg) << ": " << (var) << std::endl;

    bool result = protoSerial.send(msg);
    TEST_ASSERT_GREATER_THAN(0, written.size());
    std::cout << "Total bytes written: " << written.size() << std::endl;
    for (size_t i = 0; i < written.size(); ++i) {
        std::cout << "written[" << i << "] = 0x" << std::hex << std::uppercase
                  << static_cast<int>(written[i]) << std::dec << std::endl;
    }

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(1, protoSerial.getSentMessageBufferSize());
    TEST_ASSERT_EQUAL(1, protoSerial.getUnacknowledgedMessagesSize());
    TEST_ASSERT_TRUE(protoSerial.getUnacknowledgedMessage(timestamp));

    // Check if the message was encoded correctly
    // written[0] = START_BYTE, written[1] = length, written[2..2+len-1] = payload, written[2+len] = CRC
    uint8_t payload_len = written[1];
    pb_istream_t stream = pb_istream_from_buffer(written.data() + 2, payload_len);
    C110PCommand msgCopy = C110PCommand_init_zero;
    bool decode_result = pb_decode(&stream, C110PCommand_fields, &msgCopy);
    if (!decode_result) {
        std::cout << "pb_decode failed: " << PB_GET_ERROR(&stream) << std::endl;
    }
    TEST_ASSERT_TRUE(decode_result);
    TEST_ASSERT_EQUAL(msg.id, msgCopy.id);
    // Check stream writes: START_BYTE, len, buffer, crc
    TEST_ASSERT_EQUAL(protoSerial.START_BYTE, static_cast<int8_t>(written[0]));
    TEST_ASSERT_EQUAL(written.size() - 3, written[1]); // Length of the message
}

void test_send_stream_write_failure(void)
{
    Stream* streamMock = ArduinoFakeMock(Stream);
    C110PSerial protoSerial(streamMock);
    C110PCommand msg = createValidMsg(2002);
    
    When(OverloadedMethod(ArduinoFake(Stream), write, size_t(uint8_t)).Using(Any<uint8_t>()))
        .Return(0); // Simulate write failure for uint8_t
    When(OverloadedMethod(ArduinoFake(Stream), write,  size_t(const uint8_t*, size_t)))
        .Return(0); // Simulate write failure

    bool result = protoSerial.send(msg);

    // send() still returns true, but stream didn't write
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_EQUAL(1, protoSerial.getSentMessageBufferSize());
    TEST_ASSERT_EQUAL(1, protoSerial.getUnacknowledgedMessagesSize());
    TEST_ASSERT_TRUE(protoSerial.getUnacknowledgedMessage(2002));
}

void test_send_multiple_messages(void)
{
    std::vector<uint8_t> written;
    Stream* streamMock = ArduinoFakeMock(Stream);
    C110PSerial protoSerial(streamMock);
    C110PCommand msg1 = createValidMsg(1);
    C110PCommand msg2 = createValidMsg(2);

    When(OverloadedMethod(ArduinoFake(Stream), write, size_t(uint8_t)).Using(Any<uint8_t>()))
        .AlwaysDo([&written](uint8_t val) {
            std::cout << "Stream::write(uint8_t): " << static_cast<int>(val) << std::endl;
            written.push_back(val);
            return 1;
        });
    When(OverloadedMethod(ArduinoFake(Stream), write,  size_t(const uint8_t*, size_t)))
        .AlwaysDo([&written](const uint8_t* data, size_t len) {
            for (size_t i = 0; i < len; ++i) {
                written.push_back(data[i]);
            }
            return len;
        });

    protoSerial.send(msg1);
    protoSerial.send(msg2);

    TEST_ASSERT_EQUAL(2, protoSerial.getSentMessageBufferSize());
    TEST_ASSERT_EQUAL(2, protoSerial.getUnacknowledgedMessagesSize());
    TEST_ASSERT_TRUE(protoSerial.getUnacknowledgedMessage(1));
    TEST_ASSERT_TRUE(protoSerial.getUnacknowledgedMessage(2));
}

void test_createLedCommand(void)
{
    Stream* streamPtr = ArduinoFakeMock(Stream);
    C110PSerial proto(streamPtr, C110PRegion_REGION_DOME);
    C110PRegion target = C110PRegion_REGION_BODY;
    uint32_t start = 10, end = 20, duration = 30;
    C110PCommand cmd = proto.createLedCommand(target, start, end, duration);

    TEST_ASSERT_NOT_EQUAL(0, cmd.id);
    TEST_ASSERT_EQUAL(C110PRegion_REGION_DOME, cmd.source);
    TEST_ASSERT_EQUAL(target, cmd.target);
    TEST_ASSERT_EQUAL(C110PCommand_led_tag, cmd.which_data);
    TEST_ASSERT_EQUAL(start, cmd.data.led.start);
    TEST_ASSERT_EQUAL(end, cmd.data.led.end);
    TEST_ASSERT_EQUAL(duration, cmd.data.led.duration);
}

void test_createSoundCommand(void)
{
    Stream* streamPtr = ArduinoFakeMock(Stream);
    C110PSerial proto(streamPtr, C110PRegion_REGION_DOME);
    C110PRegion target = C110PRegion_REGION_BODY;
    uint32_t soundId = 42;
    bool play = true, syncToLeds = false;
    C110PCommand cmd = proto.createSoundCommand(target, soundId, play, syncToLeds);

    TEST_ASSERT_NOT_EQUAL(0, cmd.id);
    TEST_ASSERT_EQUAL(C110PRegion_REGION_DOME, cmd.source);
    TEST_ASSERT_EQUAL(target, cmd.target);
    TEST_ASSERT_EQUAL(C110PCommand_sound_tag, cmd.which_data);
    TEST_ASSERT_EQUAL(soundId, cmd.data.sound.id);
    TEST_ASSERT_EQUAL(play, cmd.data.sound.play);
    TEST_ASSERT_EQUAL(syncToLeds, cmd.data.sound.syncToLeds);
}

void test_createMoveCommand(void)
{
    Stream* streamPtr = ArduinoFakeMock(Stream);
    C110PSerial proto(streamPtr, C110PRegion_REGION_DOME);
    C110PRegion target = C110PRegion_REGION_BODY;
    uint32_t x = 1, y = 2, z = 3;
    C110PCommand cmd = proto.createMoveCommand(target, C110PActuator_BODY_NECK, x, y, z);

    TEST_ASSERT_NOT_EQUAL(0, cmd.id);
    TEST_ASSERT_EQUAL(C110PRegion_REGION_DOME, cmd.source);
    TEST_ASSERT_EQUAL(target, cmd.target);
    TEST_ASSERT_EQUAL(C110PCommand_move_tag, cmd.which_data);
    TEST_ASSERT_EQUAL(C110PActuator_BODY_NECK, cmd.data.move.target);
    TEST_ASSERT_EQUAL(x, cmd.data.move.x);
    TEST_ASSERT_EQUAL(y, cmd.data.move.y);
    TEST_ASSERT_EQUAL(z, cmd.data.move.z);
}

int test_protoserial_suite(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_send_successful);
    RUN_TEST(test_send_stream_write_failure);
    RUN_TEST(test_send_multiple_messages);
    RUN_TEST(test_createLedCommand);
    RUN_TEST(test_createSoundCommand);
    RUN_TEST(test_createMoveCommand);
    return UNITY_END();
}