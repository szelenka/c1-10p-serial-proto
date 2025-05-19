#include <unity.h>
#include <ArduinoFake.h>

using namespace fakeit;

#include "ProtoFrame.h"

void test_readFrame_valid_frame()
{
    Stream* streamPtr = ArduinoFakeMock(Stream);
    ProtoFrame protoFrame(streamPtr);
    // Frame: [START_BYTE][LEN][DATA...][CRC]
    const uint8_t START_BYTE = ProtoFrame::START_BYTE;
    const uint8_t DATA[] = {0x10, 0x20, 0x30};
    const size_t DATA_LEN = sizeof(DATA);
    const uint8_t CRC = crc8.calculate(DATA, DATA_LEN);
    printf("Calculated CRC: 0x%02X\n", CRC);

    // Simulate stream: available returns true for each byte, then false
    When(Method(ArduinoFake(Stream), available)).Return(
        1, 1, 1, 1, 1, 1, 0 // START_BYTE, DATA_LEN, DATA[0], DATA[1], DATA[2], CRC, end of stream
    );
    When(OverloadedMethod(ArduinoFake(Stream), read, int()))
        .Return(START_BYTE, DATA_LEN, DATA[0], DATA[1], DATA[2], CRC);


    bool result = protoFrame.readFrame();
    TEST_ASSERT_TRUE(result);

    // Check if the data was received correctly
    const C110PCommand lastReceivedMessage = protoFrame.getLastReceivedMessage();
    TEST_ASSERT_NOT_NULL(&lastReceivedMessage);
    TEST_ASSERT_EQUAL_UINT32(0, lastReceivedMessage.id);
}

void test_readFrame_invalid_crc()
{
    Stream* streamPtr = ArduinoFakeMock(Stream);
    ProtoFrame protoFrame(streamPtr);
    const uint8_t START_BYTE = ProtoFrame::START_BYTE;
    const uint8_t DATA[] = {0x01, 0x02};
    const size_t DATA_LEN = sizeof(DATA);
    const uint8_t BAD_CRC = 0x00; // Wrong CRC

    When(Method(ArduinoFake(Stream), available)).Return(
        1, 1, 1, 1, 1, 0
    );
    When(OverloadedMethod(ArduinoFake(Stream), read, int()))
        .Return(START_BYTE, DATA_LEN, DATA[0], DATA[1], BAD_CRC);

    bool result = protoFrame.readFrame();

    TEST_ASSERT_FALSE(result);
}

void test_readFrame_length_too_large()
{
    Stream* streamPtr = ArduinoFakeMock(Stream);
    ProtoFrame protoFrame(streamPtr);
    const uint8_t START_BYTE = ProtoFrame::START_BYTE;
    const uint8_t BAD_LEN = ProtoFrame::MAX_SIZE + 1;

    When(Method(ArduinoFake(Stream), available)).Return(1, 1, 0);
    When(OverloadedMethod(ArduinoFake(Stream), read, int()))
        .Return(START_BYTE, BAD_LEN);

    bool result = protoFrame.readFrame();

    TEST_ASSERT_FALSE(result);
}

void test_readFrame_wrong_start_byte()
{
    Stream* streamPtr = ArduinoFakeMock(Stream);
    ProtoFrame protoFrame(streamPtr);
    const int8_t WRONG_START_BYTE = ProtoFrame::START_BYTE + 0x1;
    const uint8_t DATA[] = {0x01, 0x02};
    const size_t DATA_LEN = sizeof(DATA);
    const uint8_t CRC = crc8.calculate(DATA, DATA_LEN);

    When(Method(ArduinoFake(Stream), available)).Return(
        1, 1, 1, 1, 1, 0
    );
    When(OverloadedMethod(ArduinoFake(Stream), read, int()))
        .Return(WRONG_START_BYTE, DATA_LEN, DATA[0], DATA[1], CRC);

    bool result = protoFrame.readFrame();

    TEST_ASSERT_FALSE(result);
}

void test_handleAck_acknowledges_message()
{
    // Arrange
    Stream* streamPtr = ArduinoFakeMock(Stream);
    ProtoFrame protoFrame(streamPtr);

    // Simulate sending a message and storing it in the buffer
    C110PCommand sentMsg;
    sentMsg.id = 12345;
    protoFrame.m_sentMessageBuffer.add(sentMsg);
    protoFrame.m_messageInfoMap[sentMsg.id] = {0, 0};

    // Act
    protoFrame.handleAck(sentMsg.id);

    // Assert
    C110PCommand* msgPtr = protoFrame.m_sentMessageBuffer.get(sentMsg.id);
    TEST_ASSERT_NOT_NULL(msgPtr);
    TEST_ASSERT_EQUAL_UINT32(0, protoFrame.m_messageInfoMap.count(sentMsg.id));
}

void test_handleAck_message_not_found()
{
    // Arrange
    Stream* streamPtr = ArduinoFakeMock(Stream);
    ProtoFrame protoFrame(streamPtr);

    // No message with this timestamp in buffer
    uint32_t missingTimestamp = 99999;
    protoFrame.m_messageInfoMap.erase(missingTimestamp);

    // Act
    protoFrame.handleAck(missingTimestamp);

    // Assert
    // Should not crash, and unacknowledgedMessages should remain unchanged
    TEST_ASSERT_EQUAL_UINT32(0, protoFrame.m_messageInfoMap.count(missingTimestamp));
}

void test_handleNack_message_not_found()
{
    // Arrange
    Stream* streamPtr = ArduinoFakeMock(Stream);
    ProtoFrame protoFrame(streamPtr);

    // No message with this timestamp in buffer
    uint32_t missingTimestamp = 55555;

    // Act
    protoFrame.handleNack(missingTimestamp);

    // Assert
    // Should not crash, nothing to check as nothing should happen
    C110PCommand* msgPtr = protoFrame.m_sentMessageBuffer.get(missingTimestamp);
    TEST_ASSERT_NULL(msgPtr);
}

void test_handleNack_message_already_acknowledged()
{
    // Arrange
    Stream* streamPtr = ArduinoFakeMock(Stream);
    ProtoFrame protoFrame(streamPtr);

    C110PCommand sentMsg;
    sentMsg.id = 22222;
    protoFrame.m_sentMessageBuffer.add(sentMsg);
    protoFrame.m_messageInfoMap[sentMsg.id] = {0, 0};
    protoFrame.m_messageInfoMap.erase(sentMsg.id);

    // Act
    protoFrame.handleNack(sentMsg.id);

    // Assert
    C110PCommand* msgPtr = protoFrame.m_sentMessageBuffer.get(sentMsg.id);
    TEST_ASSERT_NOT_NULL(msgPtr);
}

void test_handleNack_message_not_acknowledged_resends_and_sets_processedTimestamp()
{
    // Arrange
    Stream* streamPtr = ArduinoFakeMock(Stream);
    ProtoFrame protoFrame(streamPtr);

    C110PCommand sentMsg;
    sentMsg.id = 33333;

    protoFrame.m_sentMessageBuffer.add(sentMsg);
    protoFrame.m_messageInfoMap[sentMsg.id] = {0, 0};

    // Act
    protoFrame.handleNack(sentMsg.id);

    // Assert
    C110PCommand* msgPtr = protoFrame.m_sentMessageBuffer.get(sentMsg.id);
    TEST_ASSERT_NOT_NULL(msgPtr);
    TEST_ASSERT_EQUAL_UINT32(1, protoFrame.m_messageInfoMap.count(sentMsg.id));
    TEST_ASSERT_NOT_EQUAL_UINT32(0, protoFrame.m_messageInfoMap[sentMsg.id].lastProcessedTimestamp);
}

void test_sendAck_calls_send_with_correct_message()
{
    // Arrange
    Stream* streamMock = ArduinoFakeMock(Stream);
    ProtoFrame protoFrame(streamMock);

    // Act
    uint32_t testTimestamp = 123456;
    protoFrame.sendAck(testTimestamp);

    C110PCommand msg = *protoFrame.m_sentMessageBuffer.get(testTimestamp);

    // Assert
    TEST_ASSERT_EQUAL_UINT32(0, protoFrame.m_messageInfoMap.count(testTimestamp));
    TEST_ASSERT_EQUAL_UINT32(testTimestamp, msg.id);
    TEST_ASSERT_TRUE(msg.data.ack.acknowledged);
}

void test_sendNack_calls_send_with_correct_message_and_reason()
{
    // Arrange
    Stream* streamMock = ArduinoFakeMock(Stream);
    ProtoFrame protoFrame(streamMock);

    uint32_t testTimestamp = 123456;
    const char* reason = "TestReason";

    // Act
    protoFrame.sendNack(testTimestamp, reason);

    C110PCommand msg = *protoFrame.m_sentMessageBuffer.get(testTimestamp);

    // Assert
    TEST_ASSERT_EQUAL_UINT32(0, protoFrame.m_messageInfoMap.count(testTimestamp));
    TEST_ASSERT_EQUAL_UINT32(testTimestamp, msg.id);
    // Extract the string from pb_callback_t (assuming it is stored in 'arg' as a char*)
    char actual_reason[64] = {0};
    strncpy(actual_reason, msg.data.ack.reason, sizeof(actual_reason) - 1);
    TEST_ASSERT_EQUAL_STRING(reason, actual_reason);
}

void test_processCallback_calls_handleAck_when_acknowledged()
{
    // Arrange
    Stream* streamPtr = ArduinoFakeMock(Stream);

    C110PCommand msg;
    msg.which_data = C110PCommand_ack_tag;
    msg.id = 1111;
    msg.data.ack.acknowledged = true;

    // We'll use FakeIt to spy on handleAck
    struct : ProtoFrame {
        using ProtoFrame::ProtoFrame;
        int called = 0;
        uint32_t lastTimestamp = 0;
        void handleAck(uint32_t timestamp) override
        {                
            called++;
            lastTimestamp = timestamp;
        }
    } spyProtoFrame(streamPtr);

    // Act
    spyProtoFrame.processCallback(msg);

    // Assert
    TEST_ASSERT_EQUAL_INT(1, spyProtoFrame.called);
    TEST_ASSERT_EQUAL_UINT32(msg.id, spyProtoFrame.lastTimestamp);
}

void test_processCallback_calls_handleNack_when_not_acknowledged()
{
    Stream* streamPtr = ArduinoFakeMock(Stream);

    C110PCommand msg;
    msg.which_data = C110PCommand_ack_tag;
    msg.data.ack.acknowledged = false;
    msg.id = 2222;

    struct : ProtoFrame {
        using ProtoFrame::ProtoFrame;
        int called = 0;
        uint32_t lastTimestamp = 0;
        void handleNack(uint32_t timestamp) override {
            called++;
            lastTimestamp = timestamp;
        }
    } spyProtoFrame(streamPtr);

    spyProtoFrame.processCallback(msg);

    TEST_ASSERT_EQUAL_INT(1, spyProtoFrame.called);
    TEST_ASSERT_EQUAL_UINT32(msg.id, spyProtoFrame.lastTimestamp);
}

void test_processCallback_calls_LedCallback_when_cmd_led()
{
    Stream* streamPtr = ArduinoFakeMock(Stream);
    ProtoFrame protoFrame(streamPtr);

    static bool callbackCalled = false;
    static int ledValue = 0;
    static int ledStart = 0;
    static int ledEnd = 0;

    // Use a lambda without captures or a static function
    auto cb = [](const C110PCommand_data_led_MSGTYPE& d)
    {
        callbackCalled = true;
        ledValue = d.duration;
        ledStart = d.start;
        ledEnd = d.end;
    };
    protoFrame.setLedCallback(cb);

    C110PCommand msg;
    msg.which_data = C110PCommand_led_tag;
    msg.data.led.duration = 42;
    msg.data.led.start = 0;
    msg.data.led.end = 200;

    protoFrame.processCallback(msg);

    TEST_ASSERT_TRUE(callbackCalled);
    TEST_ASSERT_EQUAL_INT(msg.data.led.duration, ledValue);
    TEST_ASSERT_EQUAL_INT(msg.data.led.start, ledStart);
    TEST_ASSERT_EQUAL_INT(msg.data.led.end, ledEnd);
}

void test_processCallback_calls_MovementCallback_when_cmd_move()
{
    Stream* streamPtr = ArduinoFakeMock(Stream);
    ProtoFrame protoFrame(streamPtr);

    static bool callbackCalled = false;
    static int target = 0;
    static int x = 0;
    static int y = 0;
    static int z = 0;

    // Use a lambda without captures or a static function
    auto cb = [](const C110PCommand_data_move_MSGTYPE& d)
    {
        callbackCalled = true;
        target = d.target;
        x = d.x;
        y = d.y;
        z = d.z;
    };
    protoFrame.setMoveCallback(cb);

    C110PCommand msg;
    msg.which_data = C110PCommand_move_tag;
    msg.data.move.target = C110PActuator_BODY_NECK;
    msg.data.move.x = 7;
    msg.data.move.y = 10;
    msg.data.move.z = 255;

    protoFrame.processCallback(msg);

    TEST_ASSERT_TRUE(callbackCalled);
    TEST_ASSERT_EQUAL_INT(msg.data.move.target, target);
    TEST_ASSERT_EQUAL_INT(msg.data.move.x, x);
    TEST_ASSERT_EQUAL_INT(msg.data.move.y, y);
    TEST_ASSERT_EQUAL_INT(msg.data.move.z, z);
}

void test_processCallback_calls_SoundCallback_when_cmd_sound()
{
    Stream* streamPtr = ArduinoFakeMock(Stream);
    ProtoFrame protoFrame(streamPtr);

    static bool callbackCalled = false;
    static int id = 0;
    static bool play = false;
    static bool sync = false;

    // Use a lambda without captures or a static function
    auto cb = [](const C110PCommand_data_sound_MSGTYPE& d)
    {
        callbackCalled = true;
        id = d.id;
        play = d.play;
        sync = d.syncToLeds;
    };
    protoFrame.setSoundCallback(cb);

    C110PCommand msg;
    msg.which_data = C110PCommand_sound_tag;
    msg.data.sound.id = 7;
    msg.data.sound.play = true;
    msg.data.sound.syncToLeds = true;

    protoFrame.processCallback(msg);

    TEST_ASSERT_TRUE(callbackCalled);
    TEST_ASSERT_EQUAL_INT(msg.data.sound.id, id);
    TEST_ASSERT_EQUAL_INT(msg.data.sound.play, play);
    TEST_ASSERT_EQUAL_INT(msg.data.sound.syncToLeds, sync);
}

void test_processCallback_does_nothing_on_unknown_type()
{
    Stream* streamPtr = ArduinoFakeMock(Stream);
    ProtoFrame protoFrame(streamPtr);

    // Should not crash or call any callback
    auto ledCb = [](const C110PCommand_data_led_MSGTYPE& d) { TEST_FAIL_MESSAGE("LedCallback should not be called"); };
    protoFrame.setLedCallback(ledCb);
    auto moveCb = [](const C110PCommand_data_move_MSGTYPE& d) { TEST_FAIL_MESSAGE("MovementCallback should not be called"); };
    protoFrame.setMoveCallback(moveCb);
    auto soundCb = [](const C110PCommand_data_sound_MSGTYPE& d) { TEST_FAIL_MESSAGE("SoundCallback should not be called"); };
    protoFrame.setSoundCallback(soundCb);;

    C110PCommand msg;
    msg.which_data = 0xFF; // Unknown tag

    protoFrame.processCallback(msg);

    // If we reach here, test passes
    TEST_PASS();
}

void test_retryMessages_retries_unacknowledged_messages_after_timeout()
{
    // Arrange
    Stream* streamPtr = ArduinoFakeMock(Stream);

    struct : ProtoFrame
    {
        using ProtoFrame::ProtoFrame;
        int resendCalled = 0;
        uint32_t lastResentTimestamp = 0;
        void resendMessage(C110PCommand& message) override
        {
            resendCalled++;
            lastResentTimestamp = message.id;
            ProtoFrame::resendMessage(message);
        }
        uint32_t fakeTime = 10000;
        uint32_t getSafeTimestamp() const override { return fakeTime; }
    } protoFrame(streamPtr);

    protoFrame.m_messageTimeout = 1000;

    C110PCommand msg;
    msg.id = 42;

    protoFrame.m_sentMessageBuffer.add(msg);
    protoFrame.m_messageInfoMap[msg.id] = {8000, 1};

    // Act
    protoFrame.retryMessages();

    // Assert
    TEST_ASSERT_EQUAL_INT(1, protoFrame.resendCalled);
    TEST_ASSERT_EQUAL_UINT32(42, protoFrame.lastResentTimestamp);

    C110PCommand* updatedMsg = protoFrame.m_sentMessageBuffer.get(msg.id);
    TEST_ASSERT_NOT_NULL(updatedMsg);
    TEST_ASSERT_EQUAL_UINT64(protoFrame.fakeTime, protoFrame.m_messageInfoMap[msg.id].lastProcessedTimestamp);
}

void test_retryMessages_does_not_retry_acknowledged_messages()
{
    Stream* streamPtr = ArduinoFakeMock(Stream);

    struct : ProtoFrame
    {
        using ProtoFrame::ProtoFrame;
        int resendCalled = 0;
        void resendMessage(C110PCommand&) override { resendCalled++; }
        uint32_t getSafeTimestamp() const override { return 10000; }
    } protoFrame(streamPtr);

    protoFrame.m_messageTimeout = 1000;

    C110PCommand msg;
    msg.id = 99;

    protoFrame.m_sentMessageBuffer.add(msg);

    protoFrame.retryMessages();

    TEST_ASSERT_EQUAL_INT(0, protoFrame.resendCalled);
}

void test_retryMessages_does_not_retry_if_timeout_not_reached()
{
    Stream* streamPtr = ArduinoFakeMock(Stream);

    struct : ProtoFrame
    {
        using ProtoFrame::ProtoFrame;
        int resendCalled = 0;
        void resendMessage(C110PCommand&) override { resendCalled++; }
        uint32_t getSafeTimestamp()const override { return 10000; }
    } protoFrame(streamPtr);

    protoFrame.m_messageTimeout = 1000;

    C110PCommand msg;
    msg.id = 77;
    protoFrame.m_sentMessageBuffer.add(msg);
    protoFrame.m_messageInfoMap[msg.id] = {9500, 1};

    protoFrame.retryMessages();

    TEST_ASSERT_EQUAL_INT(0, protoFrame.resendCalled);
}

void test_resendMessage_sets_processedTimestamp_and_calls_send()
{
    Stream* streamPtr = ArduinoFakeMock(Stream);

    struct : ProtoFrame
    {
        using ProtoFrame::ProtoFrame;
        int sendCalled = 0;
        C110PCommand lastSent;
        uint64_t fakeTime = 12345;
        bool send(const C110PCommand& msg) override
        {
            sendCalled++;
            lastSent = msg;
            return true;
        }
        uint32_t getSafeTimestamp() const override { return fakeTime; }
    } protoFrame(streamPtr);

    C110PCommand msg;
    msg.id = 555;
    protoFrame.m_messageInfoMap[msg.id] = {0, 1}; // 1000ms ago

    protoFrame.resendMessage(msg);

    TEST_ASSERT_EQUAL_INT(1, protoFrame.sendCalled);
    TEST_ASSERT_EQUAL_UINT32(555, protoFrame.lastSent.id);
    TEST_ASSERT_EQUAL_UINT64(protoFrame.fakeTime, protoFrame.m_messageInfoMap[msg.id].lastProcessedTimestamp);
}

void test_receiveMessage_decodes_and_processes_new_message()
{
    Stream* streamPtr = ArduinoFakeMock(Stream);

    struct : ProtoFrame
    {
        using ProtoFrame::ProtoFrame;
        int processCalled = 0;
        C110PCommand lastMsg;
        void processCallback(const C110PCommand& msg) override
        {
            processCalled++;
            lastMsg = msg;
        }
        int sendAckCalled = 0;
        uint32_t lastAckTimestamp = 0;
        void sendAck(uint32_t ts) override
        {
            sendAckCalled++;
            lastAckTimestamp = ts;
        }
    } protoFrame(streamPtr);

    // Prepare a valid C110PCommand message
    C110PCommand msg;
    msg.id = 1234;
    msg.which_data = C110PCommand_led_tag;
    msg.data.led.duration = 10;
    msg.data.led.start = 1;
    msg.data.led.end = 2;

    uint8_t buffer[64];
    pb_ostream_t ostream = pb_ostream_from_buffer(buffer, sizeof(buffer));
    bool encodeOk = pb_encode(&ostream, C110PCommand_fields, &msg);
    TEST_ASSERT_TRUE(encodeOk);

    // Act
    protoFrame.receiveMessage(buffer, ostream.bytes_written);

    // Assert
    TEST_ASSERT_EQUAL_INT(1, protoFrame.processCalled);
    TEST_ASSERT_EQUAL_UINT32(1234, protoFrame.lastMsg.id);
    TEST_ASSERT_EQUAL_INT(1, protoFrame.sendAckCalled);
    TEST_ASSERT_EQUAL_UINT32(1234, protoFrame.lastAckTimestamp);
    TEST_ASSERT_TRUE(protoFrame.m_receivedMessageBuffer.contains(1234));
}

void test_receiveMessage_duplicate_message_only_acks()
{
    Stream* streamPtr = ArduinoFakeMock(Stream);

    struct : ProtoFrame
    {
        using ProtoFrame::ProtoFrame;
        int processCalled = 0;
        int sendAckCalled = 0;
        uint32_t lastAckTimestamp = 0;
        void processCallback(const C110PCommand&) override { processCalled++; }
        void sendAck(uint32_t ts) override { sendAckCalled++; lastAckTimestamp = ts; }
    } protoFrame(streamPtr);

    C110PCommand msg;
    msg.id = 4321;
    msg.which_data = C110PCommand_led_tag;

    // Add to received buffer to simulate duplicate
    protoFrame.m_receivedMessageBuffer.add(msg);

    uint8_t buffer[64];
    pb_ostream_t ostream = pb_ostream_from_buffer(buffer, sizeof(buffer));
    bool encodeOk = pb_encode(&ostream, C110PCommand_fields, &msg);
    TEST_ASSERT_TRUE(encodeOk);

    protoFrame.receiveMessage(buffer, ostream.bytes_written);

    TEST_ASSERT_EQUAL_INT(0, protoFrame.processCalled);
    TEST_ASSERT_EQUAL_INT(1, protoFrame.sendAckCalled);
    TEST_ASSERT_EQUAL_UINT32(4321, protoFrame.lastAckTimestamp);
}

void test_receiveMessage_invalid_protobuf_does_nothing()
{
    Stream* streamPtr = ArduinoFakeMock(Stream);

    struct : ProtoFrame
    {
        using ProtoFrame::ProtoFrame;
        int processCalled = 0;
        int sendAckCalled = 0;
        void processCallback(const C110PCommand&) override { processCalled++; }
        void sendAck(uint32_t) override { sendAckCalled++; }
    } protoFrame(streamPtr);

    // Provide invalid buffer (not a valid protobuf)
    uint8_t buffer[4] = {0xFF, 0xFF, 0xFF, 0xFF};

    protoFrame.receiveMessage(buffer, sizeof(buffer));

    TEST_ASSERT_EQUAL_INT(0, protoFrame.processCalled);
    TEST_ASSERT_EQUAL_INT(0, protoFrame.sendAckCalled);
}

int test_protoframe_suite(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_readFrame_valid_frame);
    RUN_TEST(test_readFrame_invalid_crc);
    RUN_TEST(test_readFrame_length_too_large);
    RUN_TEST(test_readFrame_wrong_start_byte);

    RUN_TEST(test_handleAck_acknowledges_message);
    RUN_TEST(test_handleAck_message_not_found);

    RUN_TEST(test_sendAck_calls_send_with_correct_message);

    RUN_TEST(test_handleNack_message_not_found);
    RUN_TEST(test_handleNack_message_already_acknowledged);
    RUN_TEST(test_handleNack_message_not_acknowledged_resends_and_sets_processedTimestamp);
    
    RUN_TEST(test_sendNack_calls_send_with_correct_message_and_reason);

    RUN_TEST(test_processCallback_calls_handleAck_when_acknowledged);
    RUN_TEST(test_processCallback_calls_handleNack_when_not_acknowledged);
    RUN_TEST(test_processCallback_calls_LedCallback_when_cmd_led);
    RUN_TEST(test_processCallback_calls_MovementCallback_when_cmd_move);
    RUN_TEST(test_processCallback_calls_SoundCallback_when_cmd_sound);
    RUN_TEST(test_processCallback_does_nothing_on_unknown_type);

    RUN_TEST(test_retryMessages_retries_unacknowledged_messages_after_timeout);
    RUN_TEST(test_retryMessages_does_not_retry_acknowledged_messages);
    RUN_TEST(test_retryMessages_does_not_retry_if_timeout_not_reached);
    RUN_TEST(test_resendMessage_sets_processedTimestamp_and_calls_send);
    RUN_TEST(test_receiveMessage_decodes_and_processes_new_message);
    RUN_TEST(test_receiveMessage_duplicate_message_only_acks);
    RUN_TEST(test_receiveMessage_invalid_protobuf_does_nothing);

    return UNITY_END();
}
