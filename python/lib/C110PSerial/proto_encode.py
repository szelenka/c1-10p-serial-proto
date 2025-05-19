from time import time

# Wire format helper functions
def encode_varint(value):
    out = bytearray()
    while value > 0x7F:
        out.append((value & 0x7F) | 0x80)
        value >>= 7
    out.append(value & 0x7F)
    return out


def encode_key(field_number, wire_type):
    return encode_varint((field_number << 3) | wire_type)


def encode_length_delimited(field_number, payload_bytes):
    key = encode_key(field_number, 2)
    length = encode_varint(len(payload_bytes))
    return key + length + payload_bytes


# Command encoders
def encode_ack_command(acknowledged, reason = None):
    b = bytearray()
    if acknowledged:
        b += encode_key(1, 0) + encode_varint(1 if acknowledged else 0)
    if reason:
        b += encode_key(2, 2) + encode_varint(len(reason)) + reason.encode('utf-8')
    return b


def encode_led_command(start, end, duration):
    b = bytearray()
    if start != 0:
        b += encode_key(1, 0) + encode_varint(start)
    if end != 0:
        b += encode_key(2, 0) + encode_varint(end)
    if duration != 0:
        b += encode_key(3, 0) + encode_varint(duration)
    return b


def encode_movement_command(target, x, y=0, z=0):
    b = bytearray()
    if target != 0:
        b += encode_key(1, 0) + encode_varint(target)
    if x != 0:
        b += encode_key(2, 0) + encode_varint(x)
    if y != 0:
        b += encode_key(3, 0) + encode_varint(y)
    if z != 0:
        b += encode_key(4, 0) + encode_varint(z)
    return b


def encode_sound_command(id, play = False, sync_to_leds = False):
    b = bytearray()
    if id != 0:
        b += encode_key(1, 0) + encode_varint(id)
    if play:
        b += encode_key(2, 0) + encode_varint(1 if play else 0)
    if sync_to_leds:
        b += encode_key(3, 0) + encode_varint(1 if sync_to_leds else 0)
    return b


# Full message encoder
def encode_command(msg):
    is_error = False
    b = bytearray()
    if "id" not in msg or msg["id"] == 0:
        msg["id"] = int(time())
    b += encode_key(1, 0) + encode_varint(msg["id"])
    if msg.get("source"):
        b += encode_key(2, 0) + encode_varint(msg["source"])
    if msg.get("target"):
        b += encode_key(3, 0) + encode_varint(msg["target"])

    if "ack" in msg:
        payload = encode_ack_command(**msg["ack"])
        b += encode_length_delimited(4, payload)
    elif "led" in msg:
        payload = encode_led_command(**msg["led"])
        b += encode_length_delimited(5, payload)
    elif "move" in msg:
        payload = encode_movement_command(**msg["move"])
        b += encode_length_delimited(6, payload)
    elif "sound" in msg:
        payload = encode_sound_command(**msg["sound"])
        b += encode_length_delimited(7, payload)
    else:
        unknown_cmd_keys = [k for k in msg.keys() if k not in ("id", "source", "target")]
        is_error = True
        b = ("Unknown cmd_type: " + ", ".join(unknown_cmd_keys)).encode("utf-8")

    return is_error, bytes(b)
