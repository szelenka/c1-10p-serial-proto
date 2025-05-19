import pytest
from unittest.mock import MagicMock, patch, call

import time

from C110PSerial.ProtoFrame import ProtoFrame
from C110PSerial.proto_encode import encode_command
from C110PSerial.proto_decode import decode_command


def test_readFrame_valid_frame(stream_mock):
    proto = ProtoFrame(stream_mock)
    proto.reset()
    proto.setTimestampProvider(lambda: time.time_ns() // 1_000_000)
    DATA = bytes([0x10, 0x20, 0x30])
    DATA_LEN = len(DATA)
    CRC = proto.crc8.calculate(DATA)
    # Simulate stream: available returns True for each byte, then False
    any_side_effects = [1, 1, 1, 1, 1, 1, 0]
    read_side_efffects = [proto.START_BYTE, DATA_LEN, *DATA, CRC]
    stream_mock.any.side_effect = any_side_effects
    stream_mock.read.side_effect = read_side_efffects
    assert proto.readFrame()
    assert stream_mock.any.call_count == len(any_side_effects) - 1
    assert stream_mock.read.call_count == len(read_side_efffects)
    last_msg = proto.getLastReceivedMessage()
    assert last_msg is not None
    assert last_msg["id"] == 0

def test_readFrame_invalid_crc(stream_mock):
    proto = ProtoFrame(stream_mock)
    START_BYTE = proto.START_BYTE
    DATA = bytes([0x01, 0x02])
    DATA_LEN = len(DATA)
    BAD_CRC = 0x00
    stream_mock.any.side_effect = [1, 1, 1, 1, 1, 0]
    stream_mock.read.side_effect = [START_BYTE, DATA_LEN, *DATA, BAD_CRC]
    assert not proto.readFrame()

def test_readFrame_length_too_large(stream_mock):
    proto = ProtoFrame(stream_mock)
    START_BYTE = proto.START_BYTE
    BAD_LEN = proto.MAX_SIZE + 1
    stream_mock.any.side_effect = [1, 1, 0]
    stream_mock.read.side_effect = [START_BYTE, BAD_LEN]
    assert not proto.readFrame()

def test_readFrame_wrong_start_byte(stream_mock):
    proto = ProtoFrame(stream_mock)
    WRONG_START_BYTE = proto.START_BYTE + 1
    DATA = bytes([0x01, 0x02])
    DATA_LEN = len(DATA)
    CRC = proto.crc8.calculate(DATA)
    stream_mock.any.side_effect = [1, 1, 1, 1, 1, 0]
    stream_mock.read.side_effect = [WRONG_START_BYTE, DATA_LEN, *DATA, CRC]
    assert not proto.readFrame()

def test_handleAck_acknowledges_message(stream_mock, C110PCommand):
    proto = ProtoFrame(stream_mock)
    sent_msg = C110PCommand(id=12345, cmd_type='ack')
    proto.m_sentMessageBuffer.add(sent_msg)
    proto.m_messageInfoMap[sent_msg["id"]] = {'lastProcessedTimestamp': 0, 'retryCount': 0}
    proto.handleAck(sent_msg["id"])
    assert proto.m_sentMessageBuffer.get(sent_msg["id"]) is not None
    assert sent_msg["id"] not in proto.m_messageInfoMap

def test_handleAck_message_not_found(stream_mock):
    proto = ProtoFrame(stream_mock)
    missing_id = 99999
    proto.m_messageInfoMap.pop(missing_id, None)
    proto.handleAck(missing_id)
    assert missing_id not in proto.m_messageInfoMap

def test_handleNack_message_not_found(stream_mock):
    proto = ProtoFrame(stream_mock)
    missing_id = 55555
    proto.handleNack(missing_id)
    assert proto.m_sentMessageBuffer.get(missing_id) is None

def test_handleNack_message_already_acknowledged(stream_mock, C110PCommand):
    proto = ProtoFrame(stream_mock)
    sent_msg = C110PCommand(id=22222, cmd_type='ack')
    proto.m_sentMessageBuffer.add(sent_msg)
    proto.m_messageInfoMap[sent_msg["id"]] = {'lastProcessedTimestamp': 0, 'retryCount': 0}
    proto.m_messageInfoMap.pop(sent_msg["id"])
    proto.handleNack(sent_msg["id"])
    assert proto.m_sentMessageBuffer.get(sent_msg["id"]) is not None

def test_handleNack_message_not_acknowledged_resends_and_sets_processedTimestamp(stream_mock, C110PCommand):
    proto = ProtoFrame(stream_mock)
    sent_msg = C110PCommand(id=33333, cmd_type='ack')
    proto.m_sentMessageBuffer.add(sent_msg)
    proto.m_messageInfoMap[sent_msg["id"]] = {'lastProcessedTimestamp': 0, 'retryCount': 0}
    proto.handleNack(sent_msg["id"])
    assert proto.m_sentMessageBuffer.get(sent_msg["id"]) is not None
    assert sent_msg["id"] in proto.m_messageInfoMap
    assert proto.m_messageInfoMap[sent_msg["id"]]['lastProcessedTimestamp'] != 0

def test_sendAck_calls_send_with_correct_message(stream_mock):
    proto = ProtoFrame(stream_mock)
    test_id = 123456
    proto.sendAck(test_id)
    msg = proto.m_sentMessageBuffer.get(test_id)
    assert test_id not in proto.m_messageInfoMap
    assert msg["id"] == test_id
    assert msg["ack"]["acknowledged"]

def test_sendNack_calls_send_with_correct_message_and_reason(stream_mock):
    proto = ProtoFrame(stream_mock)
    test_id = 123456
    reason = "TestReason"
    proto.sendNack(test_id, reason)
    msg = proto.m_sentMessageBuffer.get(test_id)
    assert test_id in proto.m_messageInfoMap
    assert msg["id"] == test_id
    assert msg["ack"]["reason"] == reason

def test_processCallback_calls_handleAck_when_acknowledged(stream_mock, C110PCommand):
    proto = ProtoFrame(stream_mock)
    proto.handleAck = MagicMock()
    msg = C110PCommand(id=1111, cmd_type='ack')
    msg["ack"]["acknowledged"] = True
    proto.processCallback(msg)
    proto.handleAck.assert_called_once_with(1111)

def test_processCallback_calls_handleNack_when_not_acknowledged(stream_mock, C110PCommand):
    proto = ProtoFrame(stream_mock)
    proto.handleNack = MagicMock()
    msg = C110PCommand(id=2222, cmd_type='ack')
    msg["ack"]["acknowledged"] = False
    proto.processCallback(msg)
    proto.handleNack.assert_called_once_with(2222)

def test_processCallback_calls_LedCallback_when_cmd_led(stream_mock, C110PCommand):
    proto = ProtoFrame(stream_mock)
    called = {}
    def cb(d):
        called['called'] = True
        called['duration'] = d["duration"]
        called['start'] = d["start"]
        called['end'] = d["end"]
    proto.setLedCallback(cb)
    msg = C110PCommand(cmd_type='led')
    msg["led"]["duration"] = 42
    msg["led"]["start"] = 0
    msg["led"]["end"] = 200
    proto.processCallback(msg)
    assert called['called']
    assert called['duration'] == 42
    assert called['start'] == 0
    assert called['end'] == 200

def test_processCallback_calls_MovementCallback_when_cmd_move(stream_mock, C110PCommand):
    proto = ProtoFrame(stream_mock)
    called = {
        'called': False,
        'x': None,
        'y': None,
        'z': None
    }
    def cb(d):
        called['called'] = True
        called['x'] = d["x"]
        called['y'] = d["y"]
        called['z'] = d["z"]
    proto.setMoveCallback(cb)
    msg = C110PCommand(cmd_type='move')
    msg["move"]["x"] = 7
    msg["move"]["y"] = 10
    msg["move"]["z"] = 255
    proto.processCallback(msg)
    assert called['called']
    assert called['x'] == 7
    assert called['y'] == 10
    assert called['z'] == 255

def test_processCallback_calls_SoundCallback_when_cmd_sound(stream_mock, C110PCommand):
    proto = ProtoFrame(stream_mock)
    called = {
        'called': False,
        'id': None,
        'play': None,
        'sync': None
    }
    def cb(d):
        called['called'] = True
        called['id'] = d["id"]
        called['play'] = d["play"]
        called['sync'] = d["syncToLeds"]
    proto.setSoundCallback(cb)
    msg = C110PCommand(cmd_type='sound')
    msg["sound"]["id"] = 7
    msg["sound"]["play"] = True
    msg["sound"]["syncToLeds"] = True
    proto.processCallback(msg)
    assert called['called']
    assert called['id'] == 7
    assert called['play'] is True
    assert called['sync'] is True

def test_processCallback_does_nothing_on_unknown_type(stream_mock):
    proto = ProtoFrame(stream_mock)
    proto.setLedCallback(lambda d: pytest.fail("LedCallback should not be called"))
    proto.setMoveCallback(lambda d: pytest.fail("MovementCallback should not be called"))
    proto.setSoundCallback(lambda d: pytest.fail("SoundCallback should not be called"))
    proto.processCallback({})  # Should not raise

def test_retryMessages_retries_unacknowledged_messages_after_timeout(stream_mock, C110PCommand):
    class TestProtoFrame(ProtoFrame):
        def __init__(self, *a, **kw):
            super().__init__(*a, **kw)
            self.resendCalled = 0
            self.lastResentTimestamp = 0
            self.fakeTime = 10000
        def resendMessage(self, message):
            self.resendCalled += 1
            self.lastResentTimestamp = message["id"]
            super().resendMessage(message)
        def getSafeTimestamp(self):
            return self.fakeTime
    proto = TestProtoFrame(stream_mock)
    proto.m_messageTimeout = 1000
    msg = C110PCommand(id=42, cmd_type='ack')
    proto.m_sentMessageBuffer.add(msg)
    proto.m_messageInfoMap[msg["id"]] = {'lastProcessedTimestamp': 8000, 'retryCount': 1}
    proto.retryMessages()
    assert proto.resendCalled == 1
    assert proto.lastResentTimestamp == 42
    assert proto.m_sentMessageBuffer.get(msg["id"]) is not None
    assert proto.m_messageInfoMap[msg["id"]]['lastProcessedTimestamp'] == proto.fakeTime

def test_retryMessages_does_not_retry_acknowledged_messages(stream_mock, C110PCommand):
    class TestProtoFrame(ProtoFrame):
        def __init__(self, *a, **kw):
            super().__init__(*a, **kw)
            self.resendCalled = 0
        def resendMessage(self, message):
            self.resendCalled += 1
        def getSafeTimestamp(self):
            return 10000
    proto = TestProtoFrame(stream_mock)
    proto.m_messageTimeout = 1000
    msg = C110PCommand(id=99, cmd_type='ack')
    proto.m_sentMessageBuffer.add(msg)
    proto.retryMessages()
    assert proto.resendCalled == 0

def test_retryMessages_does_not_retry_if_timeout_not_reached(stream_mock, C110PCommand):
    class TestProtoFrame(ProtoFrame):
        def __init__(self, *a, **kw):
            super().__init__(*a, **kw)
            self.resendCalled = 0
        def resendMessage(self, message):
            self.resendCalled += 1
        def getSafeTimestamp(self):
            return 10000
    proto = TestProtoFrame(stream_mock)
    proto.m_messageTimeout = 1000
    msg = C110PCommand(id=77, cmd_type='ack')
    proto.m_sentMessageBuffer.add(msg)
    proto.m_messageInfoMap[msg["id"]] = {'lastProcessedTimestamp': 9500, 'retryCount': 1}
    proto.retryMessages()
    assert proto.resendCalled == 0

def test_resendMessage_sets_processedTimestamp_and_calls_send(stream_mock, C110PCommand):
    class TestProtoFrame(ProtoFrame):
        def __init__(self, *a, **kw):
            super().__init__(*a, **kw)
            self.sendCalled = 0
            self.lastSent = None
            self.fakeTime = 12345
        def send(self, msg):
            self.sendCalled += 1
            self.lastSent = msg
            self.trackMessage(msg)
            return True
        def getSafeTimestamp(self):
            return self.fakeTime
    proto = TestProtoFrame(stream_mock)
    msg = C110PCommand(id=555, cmd_type='ack')
    msg["source"] = 2
    msg["target"] = 1
    msg["ack"]["acknowledged"] = True
    proto.m_messageInfoMap[msg["id"]] = {'lastProcessedTimestamp': 0, 'retryCount': 1}
    proto.resendMessage(msg)
    assert proto.sendCalled == 1
    assert proto.lastSent["id"] == 555
    assert proto.m_messageInfoMap[msg["id"]]['lastProcessedTimestamp'] == proto.fakeTime

def test_receiveMessage_decodes_and_processes_new_message(stream_mock, C110PCommand):
    class TestProtoFrame(ProtoFrame):
        def __init__(self, *a, **kw):
            super().__init__(*a, **kw)
            self.processCalled = 0
            self.lastMsg = None
            self.sendAckCalled = 0
            self.lastAckTimestamp = 0
        def processCallback(self, msg):
            self.processCalled += 1
            self.lastMsg = msg
        def sendAck(self, ts):
            self.sendAckCalled += 1
            self.lastAckTimestamp = ts
    proto = TestProtoFrame(stream_mock)
    msg = C110PCommand(id=1234, cmd_type='led')
    msg["led"]["duration"] = 10
    msg["led"]["start"] = 1
    msg["led"]["end"] = 2
    err, buffer = encode_command(msg)
    proto.receiveMessage(buffer)
    assert proto.processCalled == 1
    assert proto.lastMsg["id"] == 1234
    assert proto.sendAckCalled == 1
    assert proto.lastAckTimestamp == 1234
    assert proto.m_receivedMessageBuffer.contains(1234)

def test_receiveMessage_duplicate_message_only_acks(stream_mock, C110PCommand):
    class TestProtoFrame(ProtoFrame):
        def __init__(self, *a, **kw):
            super().__init__(*a, **kw)
            self.processCalled = 0
            self.sendAckCalled = 0
            self.lastAckTimestamp = 0
        def processCallback(self, msg): self.processCalled += 1
        def sendAck(self, ts):
            self.sendAckCalled += 1
            self.lastAckTimestamp = ts
    proto = TestProtoFrame(stream_mock)
    msg = C110PCommand(id=4321, cmd_type='move')
    proto.m_receivedMessageBuffer.add(msg)
    err, buffer = encode_command(msg)
    proto.receiveMessage(buffer)
    assert proto.processCalled == 0
    assert proto.sendAckCalled == 1
    assert proto.lastAckTimestamp == 4321

def test_receiveMessage_invalid_protobuf_does_nothing(stream_mock):
    class TestProtoFrame(ProtoFrame):
        def __init__(self, *a, **kw):
            super().__init__(*a, **kw)
            self.processCalled = 0
            self.sendAckCalled = 0
        def processCallback(self, msg): self.processCalled += 1
        def sendAck(self, ts): self.sendAckCalled += 1
    proto = TestProtoFrame(stream_mock)
    buffer = b'\xFF\xFF\xFF\xFF'
    proto.receiveMessage(buffer)
    assert proto.processCalled == 0
    assert proto.sendAckCalled == 0


if __name__ == "__main__":
    pytest.main([__file__])
