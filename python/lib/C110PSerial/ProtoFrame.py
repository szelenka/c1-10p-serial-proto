import time
from .CRC8 import CRC8
from .RingBuffer import RingBuffer
from .proto_encode import encode_command
from .proto_decode import decode_command
from .Logger import logger

class ProtoFrame:
    START_BYTE = 0xAA
    MAX_SIZE = 128
    BUFFER_DATA_MAX_SIZE = 128
    BUFFER_MESSAGE_MAX_SIZE = 256

    def __init__(self, stream, identifier=0, timeout=1000, maxRetries=3):
        self.m_regionId = identifier
        self.m_stream = stream
        self.m_sentMessageBuffer = RingBuffer()
        self.m_receivedMessageBuffer = RingBuffer()
        self.m_messageTimeout = timeout
        self.m_maxRetries = maxRetries
        self.m_messageInfoMap = {}
        self.m_inputBuffer = bytearray(self.BUFFER_MESSAGE_MAX_SIZE)
        self.m_inputIndex = 0
        self.m_inputLength = 0
        self.m_inputCrc = 0
        self.m_timestampProvider = lambda: int(time.time())
        self.m_LedCallback = lambda msg: None
        self.m_SoundCallback = lambda msg: None
        self.m_MoveCallback = lambda msg: None
        self.crc8 = CRC8()

        if hasattr(self.m_stream, 'any'):
            # MicroPython
            self.bytesAvailable = lambda: self.m_stream.any()
        elif hasattr(self.m_stream, 'in_waiting'):
            # CircuitPython
            self.bytesAvailable = lambda: self.m_stream.in_waiting
        elif hasattr(self.m_stream, 'available'):
            # Python
            self.bytesAvailable = lambda: self.m_stream.available()
        else:
            raise ValueError("Stream does not support bytesAvailable method")

    def reset(self):
        self.m_sentMessageBuffer.reset()
        self.m_receivedMessageBuffer.reset()
        self.m_messageInfoMap = {}
        self.m_inputIndex = 0
        self.m_inputLength = 0
        self.m_inputCrc = 0

    def setTimestampProvider(self, provider):
        self.m_timestampProvider = provider

    def getSafeTimestamp(self):
        return self.m_timestampProvider() & 0xFFFFFFFF

    def setLedCallback(self, cb):
        self.m_LedCallback = cb

    def setSoundCallback(self, cb):
        self.m_SoundCallback = cb

    def setMoveCallback(self, cb):
        self.m_MoveCallback = cb

    def send(self, msg):
        self.trackMessage(msg)
        return False
    
    def trackMessage(self, msg):
        self.m_sentMessageBuffer.add(msg)
        # TODO: m_messageInfoMap should be a RingBuffer; if nerver ack, it'll grow unbounded
        if msg["id"] not in self.m_messageInfoMap:
            self.m_messageInfoMap[msg["id"]] = {
                "lastProcessedTimestamp": self.getSafeTimestamp(),
                "retryCount": 0
            }
        else:
            self.m_messageInfoMap[msg["id"]]["lastProcessedTimestamp"] = self.getSafeTimestamp()
            self.m_messageInfoMap[msg["id"]]["retryCount"] += 1

    def receive(self, msg):
        self.m_receivedMessageBuffer.add(msg)
        return True

    def readFrame(self):
        timestamp = self.getSafeTimestamp()
        while self.bytesAvailable():
            now = self.getSafeTimestamp()
            if now - timestamp > self.m_messageTimeout:
                break

            c = self.m_stream.read(1)
            if not c:
                break
            logger.debug(f"Received byte: {c}")

            if self.m_inputIndex == 0 and c == self.START_BYTE:
                self.m_inputIndex = 1
                continue
            elif self.m_inputIndex == 1:
                self.m_inputLength = c
                if self.m_inputLength > self.MAX_SIZE - 1:
                    self.m_inputIndex = 0
                    self.m_inputLength = 0
                    self.m_inputCrc = 0
                    logger.error(f"Invalid length: {self.m_inputLength}")
                    return False
                else:
                    self.m_inputIndex += 1
                continue
            elif self.m_inputIndex == self.m_inputLength + 2:
                self.m_inputCrc = c
                if self.crc8.calculate(self.m_inputBuffer[:self.m_inputLength]) == self.m_inputCrc:
                    self.receiveMessage(self.m_inputBuffer[:self.m_inputLength])
                    self.m_inputIndex = 0
                    self.m_inputLength = 0
                    self.m_inputCrc = 0
                    return True
                else:
                    self.m_inputIndex = 0
                    self.m_inputLength = 0
                    self.m_inputCrc = 0
                    logger.error("Invalid CRC: {m_inputCrc}")
                    return False
            elif self.m_inputIndex > 1:
                if self.m_inputIndex < self.BUFFER_MESSAGE_MAX_SIZE - 1:
                    self.m_inputBuffer[self.m_inputIndex - 2] = c
                    self.m_inputIndex += 1
                else:
                    self.m_inputIndex = 0
                continue
        logger.debug("No data available")
        return False

    def getSentMessageBufferSize(self):
        return len(self.m_sentMessageBuffer)

    def getReceivedMessageBufferSize(self):
        return len(self.m_receivedMessageBuffer)

    def getUnacknowledgedMessagesSize(self):
        return len(self.m_messageInfoMap)

    def getUnacknowledgedMessage(self, timestamp):
        return timestamp if timestamp in self.m_messageInfoMap else 0

    def getLastSentMessage(self):
        return self.m_sentMessageBuffer.getCurrentValue()

    def getLastReceivedMessage(self):
        return self.m_receivedMessageBuffer.getCurrentValue()

    def handleAck(self, timestamp):
        self.m_messageInfoMap.pop(timestamp, None)

    def sendAck(self, timestamp):
        msg = {
            "id": timestamp,
            "ack": {
                "acknowledged": True,
                "reason": None
            }
        }
        self.send(msg)
        self.m_messageInfoMap.pop(timestamp, None)

    def sendNack(self, timestamp, reason="Unknown"):
        msg = {
            "id": timestamp,
            "ack": {
                "acknowledged": False,
                "reason": reason
            }
        }
        self.send(msg)

    def handleNack(self, timestamp):
        if timestamp in self.m_sentMessageBuffer and timestamp in self.m_messageInfoMap:
            self.resendMessage(self.m_sentMessageBuffer[timestamp])

    def retryMessages(self):
        currentTime = self.getSafeTimestamp()
        for timestamp, info in list(self.m_messageInfoMap.items()):
            if (currentTime - info['lastProcessedTimestamp'] >= self.m_messageTimeout and
                info['retryCount'] < self.m_maxRetries):
                self.resendMessage(self.m_sentMessageBuffer[timestamp])
            elif info['retryCount'] >= self.m_maxRetries:
                del self.m_messageInfoMap[timestamp]

    def resendMessage(self, msg):
        self.send(msg)

    def receiveMessage(self, rawMessage):
        # Replace with your own message decoding logic
        err, msg = decode_command(rawMessage)
        logger.debug(f"Decoded message: {msg}")
        if err:
            if msg["id"] != 0:
                self.sendNack(msg["id"], "Invalid message")
            return
        if self.m_receivedMessageBuffer.contains(msg["id"]):
            self.sendAck(msg['id'])
        else:
            self.receive(msg)
            self.sendAck(msg['id'])
            self.processCallback(msg)

    def processCallback(self, message):
        if 'ack' in message:
            if message['ack']['acknowledged']:
                self.handleAck(message['id'])
            else:
                self.handleNack(message['id'])
        elif 'led' in message and self.m_LedCallback:
            self.m_LedCallback(message['led'])
        elif 'move' in message and self.m_MoveCallback:
            self.m_MoveCallback(message['move'])
        elif 'sound' in message and self.m_SoundCallback:
            self.m_SoundCallback(message['sound'])
