import pytest
from C110PSerial.CRC8 import CRC8

@pytest.fixture
def crc8():
    return CRC8()

def test_crc8_calculate_byte_known_values():
    assert CRC8._calculate_byte(0x00) == 0x00
    assert CRC8._calculate_byte(0xFF) == 0xF3
    assert CRC8._calculate_byte(0xA5) == 0x72
    assert CRC8._calculate_byte(0x01) == 0x07

def test_crc8_calculate_empty_data(crc8):
    assert crc8.calculate(b"") == CRC8.INIT ^ CRC8.XOROUT

def test_crc8_calculate_single_byte(crc8):
    assert crc8.calculate(bytes([0xAB])) == CRC8._calculate_byte(0xAB)

def test_crc8_calculate_multiple_bytes(crc8):
    data = bytes([0x01, 0x02, 0x03, 0x04])
    assert crc8.calculate(data) == 0xE3

def test_crc8_calculate_all_zeros(crc8):
    data = bytes([0x00] * 8)
    assert crc8.calculate(data) == 0x00

def test_crc8_calculate_all_ones(crc8):
    data = bytes([0xFF] * 8)
    assert crc8.calculate(data) == 0xD7

def test_crc8_calculate_incremental_data(crc8):
    data = bytes(range(16))
    assert crc8.calculate(data) == 0x41

def test_crc8_calculate_alternating_pattern(crc8):
    data = bytes([0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55, 0xAA, 0x55])
    assert crc8.calculate(data) == 0x3F

if __name__ == "__main__":
    pytest.main([__file__])
