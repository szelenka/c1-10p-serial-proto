#include "ProtoFrame.h"

bool ProtoFrame::readFrame()
{
    uint32_t timestamp = this->getSafeTimestamp();
    while (m_stream->available())
    {
        // std::cout << "[DEBUG] m_inputIndex: " << m_inputIndex << std::endl;
        // std::cout << "[DEBUG] m_inputLength: " << m_inputLength << std::endl;
        // Check elapsed time
        uint32_t now = this->getSafeTimestamp();
        if (now - timestamp > m_messageTimeout) {
            // Exceeded max duration, exit loop
            break;
        }

        // returns an int so that it can return all 255 possible 8 bit codes
        // plus still be able to return a -1 (0xFFFF) to indicate that nothing was actually read
        int8_t c = m_stream->read();
        std::cout << "[DEBUG] Read byte: 0x" << std::hex << static_cast<int>(c) << std::dec << std::endl;
        if (c == -1)
        {
            // No data available
            break;
        }
        if (m_inputIndex == 0 && c == START_BYTE)
        {
            // 
            std::cout << "[DEBUG] Start of new message" << std::endl;
            m_inputIndex = 1;
            continue;
        }
        else if (m_inputIndex == 1)
        {
            // 
            std::cout << "[DEBUG] Second byte should be the length" << std::endl;
            m_inputLength = static_cast<size_t>(c);
            if (m_inputLength > MAX_SIZE - 1)
            {
                // 
                std::cout << "[DEBUG] Invalid length: reset" << std::endl;
                m_inputIndex = 0;
                m_inputLength = 0;
                m_inputCrc = 0;
                // sendNack(0, "Invalid length");
                return false;
            }
            else
            {
                m_inputIndex++;
            }
            continue;
        }
        else if (m_inputIndex == m_inputLength + 2)
        {
            // should be the CRC
            m_inputCrc = static_cast<uint8_t>(c);
            // 
            std::cout << "[DEBUG] verify CRC: received=" << static_cast<int>(m_inputCrc)
                      << ", calculated=" << static_cast<int>(crc8.calculate(m_inputBuffer, m_inputLength)) << std::endl;
            std::cout << "[DEBUG] m_inputBuffer: ";
            for (size_t i = 0; i < m_inputLength; ++i) {
                std::cout << std::hex << std::setw(2) << std::setfill('0')
                          << static_cast<int>(m_inputBuffer[i]) << " ";
            }
            std::cout << std::dec << std::endl;
            if (crc8.calculate(m_inputBuffer, m_inputLength) == m_inputCrc)
            {
                receiveMessage(m_inputBuffer, m_inputLength);
                m_inputIndex = 0; 
                m_inputLength = 0;
                m_inputCrc = 0;
                return true;
            }
            else
            {
                std::cout << "[DEBUG] CRC mismatch: reset" << std::endl;
                m_inputIndex = 0; 
                m_inputLength = 0;
                m_inputCrc = 0;
                return false;
            }
            continue;
        }
        else if (m_inputIndex > 1)
        {
            // Middle of message
            if (m_inputIndex < BUFFER_MESSAGE_MAX_SIZE - 1)
            {
                m_inputBuffer[(m_inputIndex++)-2] = static_cast<uint8_t>(c);
            }
            else
            {
                // 
                std::cout << "[DEBUG] Buffer overflow: reset" << std::endl;
                m_inputIndex = 0;
                // sendNack(0, "Input buffer overflow");
            }
            continue;
        }
    }
    std::cout << "[DEBUG] Exiting readFrame (no complete message)" << std::endl;
    return false;
}

void ProtoFrame::handleAck(uint32_t timestamp)
{
    std::cout << "[DEBUG] handleAck: " << timestamp << std::endl;
    m_messageInfoMap.erase(timestamp);
}

void ProtoFrame::sendAck(uint32_t timestamp)
{
    AckCommand ack = { true };
    C110PCommand msg = C110PCommand_init_default;
    msg.id = timestamp;
    msg.data.ack = ack;
    send(msg);
}

void ProtoFrame::sendNack(uint32_t timestamp, const char* reason /* = "Unknown" */)
{
    AckCommand ack = AckCommand_init_default;
    ack.acknowledged = false;
    strncpy(ack.reason, reason, sizeof(ack.reason) - 1);
    // // Set up the callback to encode the reason string
    // ack.reason.funcs.encode = [](pb_ostream_t *stream, const pb_field_t *field, void * const *arg) -> bool {
    //     const char* str = static_cast<const char*>(*arg);
    //     return pb_encode_tag_for_field(stream, field) &&
    //            pb_encode_string(stream, (const uint8_t*)str, strlen(str));
    // };
    // ack.reason.arg = (void*)reason;
    C110PCommand msg = C110PCommand_init_default;
    msg.id = timestamp;
    msg.data.ack = ack;
    send(msg);
}

void ProtoFrame::handleNack(uint32_t timestamp)
{
    // For now, treat NACK like a retriable failure
    C110PCommand* msg = m_sentMessageBuffer.get(timestamp);
    if (msg && m_messageInfoMap.count(timestamp) == 1)
    {
        resendMessage(*msg);
    }
}

void ProtoFrame::retryMessages()
{
    uint64_t currentTime = this->getSafeTimestamp();
    // Retry unacknowledged messages from the sent buffer
    for (auto& pair : m_messageInfoMap)
    {
        uint32_t timestamp = pair.first;
        C110PCommand* msg = m_sentMessageBuffer.get(timestamp);
        if (msg && (currentTime - pair.second.lastProcessedTimestamp >= m_messageTimeout) && pair.second.retryCount < m_maxRetries)
        {
            std::cout << "[DEBUG] Retrying message with timestamp: " << timestamp << std::endl;
            // Resend the message
            resendMessage(*msg);
        }
        else if (msg && pair.second.retryCount >= m_maxRetries)
        {
            std::cout << "[DEBUG] Max retries reached for message with timestamp: " << timestamp << std::endl;
            m_messageInfoMap.erase(timestamp);
        }
    }
}

void ProtoFrame::resendMessage(C110PCommand& message)
{
    // message.processedTimestamp = this->getSafeTimestamp();
    auto it = m_messageInfoMap.find(message.id);
    if (it != m_messageInfoMap.end()) {
        it->second.lastProcessedTimestamp = this->getSafeTimestamp();
        it->second.retryCount++;
    }
    send(message);
}

void ProtoFrame::receiveMessage(const uint8_t* rawMessage, size_t length)
{
    C110PCommand msg = C110PCommand_init_zero;
    std::cout << "Received message" << std::endl;

    pb_istream_t stream = pb_istream_from_buffer(rawMessage, length);
    if (!pb_decode(&stream, C110PCommand_fields, &msg))
    {
        // sendNack(0, "Protobuf decode failed");
        return;
    }
    
    if (m_receivedMessageBuffer.contains(msg.id))
    {
        // Duplicate message: already processed, just re-ACK
        sendAck(msg.id);
    }
    else
    {
        m_receivedMessageBuffer.add(msg);
        sendAck(msg.id);
        processCallback(msg);
    }
}

void ProtoFrame::processCallback(const C110PCommand& message)
{
    std::cout << "[DEBUG] processCallback: which_data=" << message.which_data << std::endl;
    switch(message.which_data)
    {
        case C110PCommand_ack_tag:
            if (message.data.ack.acknowledged)
            {
                std::cout << "[DEBUG] Received ACK for timestamp: " << message.id << std::endl;
                handleAck(message.id);
            }
            else
            {
                std::cout << "[DEBUG] Received NACK for timestamp: " << message.id << std::endl;
                handleNack(message.id);
            }
            break;
        case C110PCommand_led_tag:
            if (m_LedCallback)
            {
                m_LedCallback(message.data.led);
            }
            break;
        case C110PCommand_move_tag:
            if (m_MovementCallback)
            {
                m_MovementCallback(message.data.move);
            }
            break;
        case C110PCommand_sound_tag:
            if (m_SoundCallback)
            {
                m_SoundCallback(message.data.sound);
            }
            break;
        default:
            // Console.println("Unknown message type");
            break;
    }
}