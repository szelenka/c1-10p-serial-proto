import pytest
from C110PSerial.proto_decode import decode_command
from C110PSerial.Logger import logger

def read_bytes(filename):
    with open(f"test/encoded_msg/{filename}", "rb") as f:
        return f.read()


def test_decode_command_ack():
    data = read_bytes("cmd_ack.bin")
    err, result = decode_command(data)
    logger.debug(result)
    assert result['id'] == 43
    assert 'ack' in result
    ack = result['ack']
    assert ack['acknowledged'] == 1
    assert ack['reason'] == "Test reason"


def test_decode_command_led():
    data = read_bytes("cmd_led.bin")
    err, result = decode_command(data)
    logger.debug(result)
    assert result['id'] == 42
    assert 'led' in result
    led = result['led']
    assert led['start'] == 1
    assert led['end'] == 2
    assert led['duration'] == 10


def test_decode_command_move():
    data = read_bytes("cmd_move.bin")
    err, result = decode_command(data)
    logger.debug(result)
    assert result['id'] == 44
    assert 'move' in result
    move = result['move']
    assert move['x'] == 100
    assert move['y'] == 200
    assert move['z'] == 300


def test_decode_command_sound():
    data = read_bytes("cmd_sound.bin")
    err, result = decode_command(data)
    logger.debug(result)
    assert result['id'] == 45
    assert 'sound' in result
    sound = result['sound']
    assert sound['id'] == 7
    assert sound['play'] == 1
    assert sound['syncToLeds'] == 0


def test_decode_command_ack_reason_none():
    data = read_bytes("cmd_ack_reason_none.bin")
    err, result = decode_command(data)
    logger.debug(result)
    assert result['id'] == 46
    assert 'ack' in result
    ack = result['ack']
    assert ack['acknowledged'] is False
    assert ack['reason'] is None


if __name__ == "__main__":
    pytest.main([__file__])
