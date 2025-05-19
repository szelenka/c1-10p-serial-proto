import glob
import os
import pytest
from C110PSerial.proto_encode import encode_command
from C110PSerial.proto_decode import decode_command
from C110PSerial.Logger import logger

ENCODED_MSG_DIR = os.path.join(os.path.dirname(__file__), "encoded_msg")
os.makedirs(ENCODED_MSG_DIR, exist_ok=True)


def save_encoded_msg(name, data):
    path = os.path.join(ENCODED_MSG_DIR, f"cmd_{name}.bin")
    with open(path, "wb") as f:
        f.write(data)


def test_encode_command_ack():
    msg = {
        "id": 43,
        "ack": {
            "acknowledged": 1,
            "reason": "Test reason"
        }
    }
    err, encoded = encode_command(msg)
    save_encoded_msg("ack", encoded)
    err, decoded = decode_command(encoded)
    assert decoded['id'] == 43
    assert 'ack' in decoded
    ack = decoded['ack']
    assert ack['acknowledged'] == 1
    assert ack['reason'] == "Test reason"


def test_encode_command_led():
    msg = {
        "id": 42,
        "led": {
            "start": 1,
            "end": 2,
            "duration": 10
        }
    }
    err, encoded = encode_command(msg)
    save_encoded_msg("led", encoded)
    err, decoded = decode_command(encoded)
    assert decoded['id'] == 42
    assert 'led' in decoded
    led = decoded['led']
    assert led['start'] == 1
    assert led['end'] == 2
    assert led['duration'] == 10


def test_encode_command_move():
    msg = {
        "id": 44,
        "move": {
            "target": 1,
            "x": 100,
            "y": 200,
            "z": 300
        }
    }
    err, encoded = encode_command(msg)
    save_encoded_msg("move", encoded)
    err, decoded = decode_command(encoded)
    assert decoded['id'] == 44
    assert 'move' in decoded
    move = decoded['move']
    assert move['target'] == 1
    assert move['x'] == 100
    assert move['y'] == 200
    assert move['z'] == 300


def test_encode_command_sound():
    msg = {
        "id": 45,
        "sound": {
            "id": 7,
            "play": 1
        }
    }
    err, encoded = encode_command(msg)
    save_encoded_msg("sound", encoded)
    err, decoded = decode_command(encoded)
    assert decoded['id'] == 45
    assert 'sound' in decoded
    sound = decoded['sound']
    assert sound['id'] == 7
    assert sound['play'] == 1
    assert sound['syncToLeds'] is False


def test_encode_command_ack_reason_none():
    msg = {
        "id": 46,
        "ack": {
            "acknowledged": 0,
            "reason": ""
        }
    }
    err, encoded = encode_command(msg)
    save_encoded_msg("ack_reason_none", encoded)
    err, decoded = decode_command(encoded)
    assert decoded['id'] == 46
    assert 'ack' in decoded
    ack = decoded['ack']
    assert ack["acknowledged"] is False
    assert ack["reason"] is None


def test_encode_command_empty():
    msg = {}
    err, encoded = encode_command(msg)
    assert err is True, "Encoding should fail for empty message"


def test_compare_encoded_msg_files():
    """
    Compare binary files in python/test/encoded_msg to files with the same name in test/encoded_msg.
    """

    python_encoded_dir = os.path.join(os.path.dirname(__file__), "encoded_msg")
    root_dir = os.path.dirname(os.path.dirname(os.path.dirname(__file__)))
    reference_encoded_dir = os.path.join(root_dir, "test", "encoded_msg")

    python_files = glob.glob(os.path.join(python_encoded_dir, "*.bin"))
    for py_file in python_files:
        logger.debug(f"Comparing {os.path.basename(py_file)}")
        filename = os.path.basename(py_file)
        ref_file = os.path.join(reference_encoded_dir, filename)
        assert os.path.exists(ref_file), f"Reference file missing: {ref_file}"
        with open(py_file, "rb") as f1, open(ref_file, "rb") as f2:
            data1 = f1.read()
            data2 = f2.read()
            assert data1 == data2, f"File mismatch: {filename}"


if __name__ == "__main__":
    pytest.main([__file__])
