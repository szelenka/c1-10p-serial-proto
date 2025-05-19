#pragma once

#include <Arduino.h>
#include <Stream.h>

#include "ProtoFrame.h"


class C110PSerial : private ProtoFrame
{
public:
    // Expose selected ProtoFrame methods/attributes as public
    using ProtoFrame::setTimestampProvider;
    using ProtoFrame::setLedCallback;
    using ProtoFrame::setSoundCallback;
    using ProtoFrame::setMovementCallback;
    using ProtoFrame::getSentMessageBufferSize;
    using ProtoFrame::getReceivedMessageBufferSize;
    using ProtoFrame::getUnacknowledgedMessagesSize;
    using ProtoFrame::getUnacknowledgedMessage;
    using ProtoFrame::getSafeTimestamp;
    using ProtoFrame::receive;
    using ProtoFrame::START_BYTE;
    using ProtoFrame::MAX_SIZE;

    explicit C110PSerial(Stream* stream, C110PRegion identifier = C110PRegion_REGION_UNSPECIFIED, uint64_t timeout = 1000)
        : ProtoFrame(stream, identifier, timeout)
    {
    }

    bool send(const C110PCommand& msg);

    void processQueue() {
        ProtoFrame::readFrame();
        retryMessages();
    }

    C110PCommand createLedCommand(C110PRegion target, uint32_t start, uint32_t end, uint32_t duration = 0) {
        C110PCommand cmd;
        cmd.id = this->getSafeTimestamp();
        cmd.source = m_regionId;
        cmd.target = target;
        cmd.which_data = C110PCommand_led_tag;
        cmd.data.led.start = start;
        cmd.data.led.end = end;
        cmd.data.led.duration = duration;
        return cmd;
    }

    C110PCommand createSoundCommand(C110PRegion target, uint32_t soundId, bool play = false, bool syncToLeds = false) {
        C110PCommand cmd;
        cmd.id = this->getSafeTimestamp();
        cmd.source = m_regionId;
        cmd.target = target;
        cmd.which_data = C110PCommand_sound_tag;
        cmd.data.sound.id = soundId;
        cmd.data.sound.play = play;
        cmd.data.sound.syncToLeds = syncToLeds;
        return cmd;
    }

    C110PCommand createMoveCommand(C110PRegion target, C110PActuator move_target, uint32_t x, uint32_t y = 0, uint32_t z = 0) {
        C110PCommand cmd;
        cmd.id = this->getSafeTimestamp();
        cmd.source = m_regionId;
        cmd.target = target;
        cmd.which_data = C110PCommand_move_tag;
        cmd.data.move.target = move_target;
        cmd.data.move.x = x;
        cmd.data.move.y = y;
        cmd.data.move.z = z;
        return cmd;
    }
};
