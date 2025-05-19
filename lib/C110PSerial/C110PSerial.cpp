#include "C110PSerial.h"

bool C110PSerial::send(const C110PCommand& msg)
{
    uint8_t buffer[MAX_SIZE] = {0};
    pb_ostream_t stream = pb_ostream_from_buffer(buffer, sizeof(buffer));
    if (!pb_encode(&stream, C110PCommand_fields, &msg))
    {
        std::cout << "Failed to encode C110PCommand message: " << PB_GET_ERROR(&stream) << std::endl;
        return false;
    }
    m_sentMessageBuffer.add(msg);
    m_messageInfoMap[msg.id] = {this->getSafeTimestamp(), 0};
    
    size_t len = stream.bytes_written;
    uint8_t crc = crc8.calculate(buffer, len);
    std::cout << "Sending data: [";
    for (size_t i = 0; i < len; ++i) {
        std::cout << std::hex << std::uppercase << static_cast<int>(buffer[i]);
        if (i < len - 1) std::cout << " ";
    }
    std::cout << "] LEN: " << len << " CRC: " << std::hex << std::uppercase << static_cast<int>(crc) << std::dec << std::endl;
    if (m_stream->write(START_BYTE) == 0 ||
        m_stream->write(len) == 0 ||
        m_stream->write(buffer, len) == 0 ||
        m_stream->write(crc) == 0)
    {
        return false;
    }
    return true;
}
