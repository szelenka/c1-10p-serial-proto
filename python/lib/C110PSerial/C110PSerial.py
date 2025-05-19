from .ProtoFrame import ProtoFrame
from typing import Callable, Optional
from .proto_encode import encode_command
from .Logger import logger

C110PRegion_REGION_UNSPECIFIED = 0
C110PRegion_REGION_BODY = 1
C110PRegion_REGION_LEG = 2
C110PRegion_REGION_NECK = 3
C110PRegion_REGION_DOME = 4

C110PActuator_UNSPECIFIED = 0
C110PActuator_BODY_NECK = 1

class C110PSerial(ProtoFrame):
    def __init__(self, stream, identifier=C110PRegion_REGION_UNSPECIFIED, timeout=1000):
        super().__init__(stream, identifier, timeout)

    def send(self, msg) -> bool:
        err, buffer = encode_command(msg)
        if len(buffer) > self.MAX_SIZE:
            logger.error(f"Message too large: {len(buffer)}")
            return False
        if err:
            logger.error(f"Error encoding message: {err}")
            return False

        crc = self.crc8.calculate(buffer)
        logger.debug("Sending data: [{}] LEN: {} CRC: {:02X}".format(
            ' '.join('{:02X}'.format(b) for b in buffer),
            len(buffer),
            crc
        ))

        if not any((self.m_stream.write(bytes([self.START_BYTE])),
            self.m_stream.write(bytes([len(buffer)])),
            self.m_stream.write(buffer),
            self.m_stream.write(bytes([crc])))
        ):
            logger.error("Write failed")
            return False
        
        self.trackMessage(msg)
        return True

    def processQueue(self):
        self.readFrame()
        self.retryMessages()

    def createLedCommand(self, target, start, end, duration=0):
        cmd = {
            "id": self.getSafeTimestamp(),
            "source": self.m_regionId,
            "target": target,
            "led": {
                "start": start,
                "end": end,
                "duration": duration
            }
        }
        return cmd

    def createSoundCommand(self, target, soundId, play=False, syncToLeds=False):
        cmd = {
            "id": self.getSafeTimestamp(),
            "source": self.m_regionId,
            "target": target,
            "sound": {
                "id": soundId,
                "play": play,
                "syncToLeds": syncToLeds
            }
        }
        return cmd

    def createMoveCommand(self, target, move_target, x=0, y=0, z=0):
        cmd = {
            "id": self.getSafeTimestamp(),
            "source": self.m_regionId,
            "target": target,
            "move": {
                "target": move_target,
                "x": x,
                "y": y,
                "z": z
            }
        }
        return cmd
