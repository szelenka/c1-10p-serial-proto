import pytest
from C110PSerial.C110PSerial import C110PSerial, C110PRegion_REGION_DOME, C110PRegion_REGION_BODY


def test_send_successful(stream_mock, C110PCommand):
    written = []

    def write_uint8(val):
        written.append(val)
        return 1
    def write_bytes(data):
        written.extend(data)
        return len(data)
    stream_mock.write.side_effect = lambda data: write_uint8(data) if isinstance(data, int) else write_bytes(data)

    protoSerial = C110PSerial(stream_mock)
    timestamp = 1001
    msg = C110PCommand(timestamp, cmd_type="led")
    msg["source"] = C110PRegion_REGION_DOME
    msg["target"] = C110PRegion_REGION_BODY
    msg["led"]["start"] = 1
    msg["led"]["end"] = 2
    msg["led"]["duration"] = 3

    result = protoSerial.send(msg)
    assert len(written) > 0
    assert result
    assert protoSerial.getSentMessageBufferSize() == 1
    assert protoSerial.getUnacknowledgedMessagesSize() == 1
    assert protoSerial.getUnacknowledgedMessage(timestamp)

def test_send_stream_write_failure(stream_mock, C110PCommand):
    stream_mock.write.return_value = 0  # Simulate write failure

    protoSerial = C110PSerial(stream_mock)
    msg = C110PCommand(2002, cmd_type="led")
    msg["source"] = C110PRegion_REGION_DOME
    msg["target"] = C110PRegion_REGION_BODY
    msg["led"]["start"] = 1
    msg["led"]["end"] = 2
    msg["led"]["duration"] = 3

    result = protoSerial.send(msg)
    assert not result
    assert protoSerial.getSentMessageBufferSize() == 0
    assert protoSerial.getUnacknowledgedMessagesSize() == 0
    assert not protoSerial.getUnacknowledgedMessage(2002)

def test_send_multiple_messages(stream_mock, C110PCommand):
    written = []

    def write_uint8(val):
        written.append(val)
        return 1
    def write_bytes(data):
        written.extend(data)
        return len(data)
    stream_mock.write.side_effect = lambda data: write_uint8(data) if isinstance(data, int) else write_bytes(data)

    protoSerial = C110PSerial(stream_mock)
    msg1 = C110PCommand(1, cmd_type="led")
    msg1["source"] = C110PRegion_REGION_DOME
    msg1["target"] = C110PRegion_REGION_BODY
    msg1["led"]["start"] = 1
    msg1["led"]["end"] = 2
    msg1["led"]["duration"] = 3
    msg2 = C110PCommand(2, cmd_type="led")
    msg2["source"] = C110PRegion_REGION_DOME
    msg2["target"] = C110PRegion_REGION_BODY
    msg2["led"]["start"] = 4
    msg2["led"]["end"] = 5
    msg2["led"]["duration"] = 6

    assert protoSerial.send(msg1)
    assert protoSerial.send(msg2)

    assert protoSerial.getSentMessageBufferSize() == 2
    assert protoSerial.getUnacknowledgedMessagesSize() == 2
    assert protoSerial.getUnacknowledgedMessage(1)
    assert protoSerial.getUnacknowledgedMessage(2)

def test_createLedCommand(stream_mock, C110PCommand):
    proto = C110PSerial(stream_mock, C110PRegion_REGION_DOME)
    target = C110PRegion_REGION_BODY
    start, end, duration = 10, 20, 30
    cmd = proto.createLedCommand(target, start, end, duration)

    assert cmd["id"] != 0
    assert cmd["source"] == C110PRegion_REGION_DOME
    assert cmd["target"] == target
    assert cmd["led"]["start"] == start
    assert cmd["led"]["end"] == end
    assert cmd["led"]["duration"] == duration

def test_createSoundCommand(stream_mock, C110PCommand):
    proto = C110PSerial(stream_mock, C110PRegion_REGION_DOME)
    target = C110PRegion_REGION_BODY
    soundId = 42
    play, syncToLeds = True, True
    cmd = proto.createSoundCommand(target, soundId, play, syncToLeds)

    assert cmd["id"] != 0
    assert cmd["source"] == C110PRegion_REGION_DOME
    assert cmd["target"] == target
    assert cmd["sound"]["id"] == soundId
    assert cmd["sound"]["play"] == play
    assert cmd["sound"]["syncToLeds"] == syncToLeds

def test_createMoveCommand(stream_mock, C110PCommand):
    proto = C110PSerial(stream_mock, C110PRegion_REGION_DOME)
    target = C110PRegion_REGION_BODY
    x, y, z = 1, 2, 3
    move_target = C110PRegion_REGION_DOME
    cmd = proto.createMoveCommand(target, move_target, x, y, z)

    assert cmd["id"] != 0
    assert cmd["source"] == C110PRegion_REGION_DOME
    assert cmd["target"] == target
    assert cmd["move"]["target"] == move_target
    assert cmd["move"]["x"] == x
    assert cmd["move"]["y"] == y
    assert cmd["move"]["z"] == z
