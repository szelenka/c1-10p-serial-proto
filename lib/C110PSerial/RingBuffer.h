#pragma once

#include <cstdint>
#include <unordered_map>
#include <cstring>

#define RING_BUFFER_SIZE 25

template<typename T>
class RingBuffer
{
public:
    RingBuffer() 
        : 
        m_head(0), 
        m_tail(0), 
        m_size(0)
    {

    }

    // Function to reset the ring buffer
    void reset()
    {
        m_head = 0;
        m_tail = 0;
        m_size = 0;
        m_messageMap.clear();
    }

    // Function to add a new message to the buffer
    void add(const T& message)
    {
        uint32_t timestamp = message.id; // Assumes T has id
        if (contains(timestamp))
        {
            return;
        }

        if (m_size < RING_BUFFER_SIZE)
        {
            ++m_size;
        }
        else
        {
            // Remove the oldest message from the map
            m_messageMap.erase(m_buffer[m_tail].id);
            m_tail = (m_tail + 1) % RING_BUFFER_SIZE;
        }
        m_buffer[m_head] = message;
        m_messageMap[timestamp] = true;
        m_head = (m_head + 1) % RING_BUFFER_SIZE;
    }
    
    // Function to check if the message timestamp already exists in the buffer
    bool contains(uint32_t timestamp)
    {
        return m_messageMap.find(timestamp) != m_messageMap.end();
    }

    // Function to get the Message by timestamp
    T* get(uint32_t timestamp)
    {
        for (int i = 0; i < m_size; ++i) {
            int idx = (m_tail + i) % RING_BUFFER_SIZE;
            if (m_buffer[idx].id == timestamp) {
                return &m_buffer[idx];
            }
        }
        return nullptr;
    }

    // Function to get the value at the current position (head - 1)
    T getCurrentValue() const
    {
        if (m_size == 0)
        {
            return T{};
        }
        int idx = (m_head == 0) ? (RING_BUFFER_SIZE - 1) : (m_head - 1);
        return m_buffer[idx];
    }

    std::unordered_map<uint32_t, bool> getMessageMap() const
    {
        return m_messageMap;
    }

    uint32_t size() const
    {
        return m_size;
    }

private:
    T m_buffer[RING_BUFFER_SIZE];  // Array to store messages
    int m_head;  // Points to the next position to insert a new message
    int m_tail;  // Points to the oldest message
    int m_size;  // Current number of elements in the buffer
    std::unordered_map<uint32_t, bool> m_messageMap;  // Hash table for fast lookup
};
