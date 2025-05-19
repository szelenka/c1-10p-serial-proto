#include <unity.h>
#include <ArduinoFake.h>

#include <stdint.h>
#include <string.h>

#include "RingBuffer.h"

using namespace fakeit;

struct TestData
{
    char payload[16];

    TestData(uint32_t ts = 0, const char* pl = "") {
        strncpy(payload, pl, sizeof(payload));
        payload[sizeof(payload)-1] = '\0';
    }
};

struct TestMessage
{
    TestData data;

    uint32_t id;
    TestMessage(uint32_t ts = 0, const char* pl = "") : id(ts), data(ts, pl) {}
};

void test_AddAndContains(void)
{
    RingBuffer<TestMessage> buf;
    TestMessage msg1(1, "foo");
    TestMessage msg2(2, "bar");

    buf.add(msg1);
    buf.add(msg2);

    TEST_ASSERT_TRUE(buf.contains(1));
    TEST_ASSERT_TRUE(buf.contains(2));
    TEST_ASSERT_FALSE(buf.contains(3));
}

void test_NoDuplicates(void)
{
    RingBuffer<TestMessage> buf;
    TestMessage msg1(1, "foo");
    buf.add(msg1);
    buf.add(msg1); // duplicate

    TEST_ASSERT_TRUE(buf.contains(1));
    TEST_ASSERT_EQUAL_UINT32(1u, buf.get(1)->id);
}

void test_GetReturnsCorrectMessage(void)
{
    RingBuffer<TestMessage> buf;
    buf.add(TestMessage(10, "abc"));
    buf.add(TestMessage(20, "def"));

    auto* msg = buf.get(20);
    TEST_ASSERT_NOT_NULL(msg);
    TEST_ASSERT_EQUAL_UINT32(20u, msg->id);
    TEST_ASSERT_EQUAL_STRING("def", msg->data.payload);
}

void test_GetReturnsNullptrIfNotFound(void)
{
    RingBuffer<TestMessage> buf;
    buf.add(TestMessage(10, "abc"));
    TEST_ASSERT_NULL(buf.get(99));
}

void test_BufferWrapsAndEvictsOldest(void)
{
    uint32_t OFFSET = 100;
    RingBuffer<TestMessage> buf;
    for (uint32_t i = 0; i < RING_BUFFER_SIZE; ++i) {
        buf.add(TestMessage(i, "x"));
    }
    buf.add(TestMessage(RING_BUFFER_SIZE+OFFSET, "new"));
    // Print buffer contents for debugging
    printf("Buffer contents:\n");
    // Print m_messageMap contents for debugging
    for (const auto& pair : buf.getMessageMap()) {
        printf("  key: %llu, value: %s\n", static_cast<unsigned long long>(pair.first), pair.second ? "true" : "false");
    }
    TEST_ASSERT_FALSE(buf.contains(0));
    TEST_ASSERT_TRUE(buf.contains(RING_BUFFER_SIZE+OFFSET));
    TEST_ASSERT_NULL(buf.get(0));
    TEST_ASSERT_NOT_NULL(buf.get(RING_BUFFER_SIZE+OFFSET));
}

void test_GetCurrentValueReturnsLastAdded(void)
{
    RingBuffer<TestMessage> buf;
    TEST_ASSERT_EQUAL_UINT32(0u, buf.getCurrentValue().id);

    buf.add(TestMessage(5, "a"));
    TEST_ASSERT_EQUAL_UINT32(5u, buf.getCurrentValue().id);

    buf.add(TestMessage(6, "b"));
    TEST_ASSERT_EQUAL_UINT32(6u, buf.getCurrentValue().id);
}

void test_ResetClearsBuffer(void)
{
    RingBuffer<TestMessage> buf;
    buf.add(TestMessage(1, "foo"));
    buf.add(TestMessage(2, "bar"));
    buf.reset();

    TEST_ASSERT_FALSE(buf.contains(1));
    TEST_ASSERT_FALSE(buf.contains(2));
    TEST_ASSERT_NULL(buf.get(1));
    TEST_ASSERT_EQUAL_UINT32(0u, buf.getCurrentValue().id);
}

void test_PutBehavesLikeAdd(void)
{
    RingBuffer<TestMessage> buf;
    buf.add(TestMessage(42, "hello"));
    TEST_ASSERT_TRUE(buf.contains(42));
    TEST_ASSERT_EQUAL_UINT32(42u, buf.get(42)->id);

    buf.add(TestMessage(42, "world"));
    TEST_ASSERT_EQUAL_STRING("hello", buf.get(42)->data.payload);
}

int test_ringbuffer_suite(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_AddAndContains);
    RUN_TEST(test_NoDuplicates);
    RUN_TEST(test_GetReturnsCorrectMessage);
    RUN_TEST(test_GetReturnsNullptrIfNotFound);
    RUN_TEST(test_BufferWrapsAndEvictsOldest);
    RUN_TEST(test_GetCurrentValueReturnsLastAdded);
    RUN_TEST(test_ResetClearsBuffer);
    RUN_TEST(test_PutBehavesLikeAdd);
    
    return UNITY_END();
}