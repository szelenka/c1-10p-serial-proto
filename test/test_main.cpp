#include <unity.h>
#include <ArduinoFake.h>


extern void test_crc8_suite();
extern void test_ringbuffer_suite();
extern void test_protoframe_suite();
extern void test_protoserial_suite();
extern int test_pb_suite();

void setUp(void)
{
    ArduinoFakeReset();
}

void tearDown(void) 
{

};

int main(void)
{
    UNITY_BEGIN();

    test_crc8_suite();
    test_ringbuffer_suite();
    test_protoframe_suite();
    test_protoserial_suite();
    test_pb_suite();

    return UNITY_END();
}
