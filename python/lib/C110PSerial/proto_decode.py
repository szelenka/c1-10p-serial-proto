import io

# Field numbers
FIELD_ID = 1
FIELD_SOURCE = 2
FIELD_TARGET = 3
FIELD_ACK = 4
FIELD_LED = 5
FIELD_MOVE = 6
FIELD_SOUND = 7


def parse_varint(stream):
    result = 0
    shift = 0
    while True:
        b = stream.read(1)
        if not b:
            break
            # raise EOFError("Unexpected end of stream while parsing varint")
        b = int.from_bytes(b, 'little')
        result |= (b & 0x7F) << shift
        if not (b & 0x80):
            break
        shift += 7
    return result


def read_key(stream):
    key = parse_varint(stream)
    field_number = key >> 3
    wire_type = key & 0x07
    return field_number, wire_type


def read_length_delimited(stream):
    length = parse_varint(stream)
    return stream.read(length)


def parse_ack(data):
    s = io.BytesIO(data)
    ack = {
        "acknowledged": False,
        "reason": None
    }
    while s.tell() < len(data):
        field, wire = read_key(s)
        if field == 1:  # acknowledged
            ack['acknowledged'] = bool(parse_varint(s))
        elif field == 2:  # reason
            ack['reason'] = read_length_delimited(s).decode()
    return ack


def parse_led(data):
    s = io.BytesIO(data)
    led = {
        "start": 0,
        "end": 0,
        "duration": 0
    }
    while s.tell() < len(data):
        field, wire = read_key(s)
        if field == 1:
            led['start'] = parse_varint(s)
        elif field == 2:
            led['end'] = parse_varint(s)
        elif field == 3:
            led['duration'] = parse_varint(s)
    return led


def parse_move(data):
    s = io.BytesIO(data)
    move = {
        "target": 0,
        "x": 0,
        "y": 0,
        "z": 0
    }
    while s.tell() < len(data):
        field, wire = read_key(s)
        if field == 1:
            move['target'] = parse_varint(s)
        elif field == 2:
            move['x'] = parse_varint(s)
        elif field == 3:
            move['y'] = parse_varint(s)
        elif field == 4:
            move['z'] = parse_varint(s)
    return move


def parse_sound(data):
    s = io.BytesIO(data)
    sound = {
        "id": 0,
        "play": False,
        "syncToLeds": False
    }
    while s.tell() < len(data):
        field, wire = read_key(s)
        if field == 1:
            sound['id'] = parse_varint(s)
        elif field == 2:
            sound['play'] = bool(parse_varint(s))
        elif field == 3:
            sound['syncToLeds'] = bool(parse_varint(s))
    return sound


def decode_command(buf):
    is_error = False
    s = io.BytesIO(buf)
    result = {
        "id": 0,
        "source": 0,
        "target": 0
    }
    while s.tell() < len(buf):
        field, wire = read_key(s)
        if field == FIELD_ID:
            result['id'] = parse_varint(s)
        elif field == FIELD_SOURCE:
            result['source'] = parse_varint(s)
        elif field == FIELD_TARGET:
            result['target'] = parse_varint(s)
        elif field == FIELD_ACK:
            data = read_length_delimited(s)
            result['ack'] = parse_ack(data)
        elif field == FIELD_LED:
            data = read_length_delimited(s)
            result['led'] = parse_led(data)
        elif field == FIELD_MOVE:
            data = read_length_delimited(s)
            result['move'] = parse_move(data)
        elif field == FIELD_SOUND:
            data = read_length_delimited(s)
            result['sound'] = parse_sound(data)
        else:
            # Skip unknown field
            if wire == 2:
                _ = read_length_delimited(s)
            else:
                parse_varint(s)
                
    if not any(key in result for key in (
        "ack", "led", "sound", "move"
    )):
        is_error = True
        result["error_message"] = "Missing oneof field: ack, led, sound, move"
    
    return is_error, result

