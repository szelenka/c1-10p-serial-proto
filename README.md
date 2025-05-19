# C1-10P Serial Proto

This is a simple lightweight protocol to communicate between microcontrollers inside C1-10P.

## Features

### Serial Frame

The serial frame format is:

```
| start_byte | length | data | crc8 |
```

- **start_byte**: A fixed value (e.g., `0xAA`) indicating the beginning of a frame.
- **length**: The number of bytes in the `data` field.
- **data**: The serialized protobuf message.
- **crc8**: A CRC-8 checksum calculated over the `data` field for error detection.

Each data object is expected to include an `id` field, which is typically a `uint32_t` timestamp. This helps uniquely identify messages and can be used for deduplication or ordering.

### "data" is a Protobuf

Protobuf uses a compact binary encoding for data. For integer fields, it uses a format called **varint**, which encodes numbers using one or more bytes, where smaller numbers use fewer bytes. This makes the encoding efficient for small values.

Additionally, Protobuf does **not send fields that have default values** (such as `0` for numbers, `false` for booleans, or empty strings/arrays). This further reduces the size of the serialized message, as only fields with non-default values are included in the output.

### ACK/NACK and Retries

Each message sent includes an `id` field, which is used to uniquely identify the message. After sending a message, the sender expects an acknowledgment (ACK) from the receiver, which references the same `id`. If the receiver cannot process the message, it responds with a negative acknowledgment (NACK) including the `id`.

If no ACK or NACK is received within a specified timeout, the sender will retry sending the message. This retry process continues up to a configurable maximum number of attempts or until a total timeout is reached. If all retries fail, the sender aborts the operation and may report an error.

This mechanism ensures reliable delivery and helps detect lost or unprocessed messages.

### Asynchronous

The protocol is designed to be asynchronous and non-blocking. Bytes are read from the serial interface as they become available, without waiting for a complete message in a single read. If a message is split across multiple reads, the implementation buffers incoming bytes and automatically combines them. The defined callback for a message type is only triggered when a full, valid message has been received and successfully decoded. This ensures that partial or corrupted messages do not invoke callbacks, and processing remains responsive even with fragmented or delayed data.

## ESP-IDF Arduino / C++

### Protobuf

To generate the protobuf files and code:

```bash
make gen-cpp
```

### lib/C110PSerial

This contains the logic to send/receive messages from [c110p_serial.proto](c110p_serial.proto)

Copy the `lib/C110PSerial` folder into your `components/C110PSerial` folder, and add to your CMakeLists.txt.

TODO: make this simplier with ESP-IDF projects

#### Send

```c++
#include <Arduino.h>
#include "C110PSerial.h"

// setup the Serial port to use
Serial2.begin(9600, SERIAL_8N1, GPIO_NUM_3, GPIO_NUM_1);

// init the protobuf with the specified pins
C110PSerial c110p_serial(&Serial2);

// use helper to format the message
C110PCommand msg = createMoveCommand(
  C110PRegion_REGION_BODY,
  C110PActuator_BODY_NECK,
  100,
  150,
  200);

// send over UART
bool result = c110p_serial.send(msg);
```

#### Receive / Process

```c++
#include <Arduino.h>
#include "C110PSerial.h"

// setup the Serial port to use
Serial2.begin(9600, SERIAL_8N1, GPIO_NUM_3, GPIO_NUM_1);

// init the protobuf with the specified pins
C110PSerial c110p_serial(&Serial2);

// define a void function to process messages by tupe
auto cb = [](const C110PCommand_data_move_MSGTYPE& d)
{
  // echo out what the message was
  // TODO: perform logic on message receive here
  std::cout << "target: " << d.target << ", x: " << d.x << ", y: " << d.y << ", z: " << d.z << std::endl;
};
c110p_serial.setMoveCallback(cb);
// repeat for all commands you want to act on

// listen to RX line in non-blocking way for messages
// upon full message received, it'll execute defined callback
// for that message type
c110p_serial.processQueue();
```

### Tests

This project relies on PlatformIO, nanopb, and unity testing framework via VSCode.

```bash
make test-cpp
```

NOTE: As of May 2025, the ArduinoFake library has a "bug" with it's copy/paste of FakeIt. The Makefile does a crude patch of this in the `clean` target. While `clean` will appear to print an error, it's because we run `pio run` to download ArduinoFake first, then use `sed` to patch it.

## MicroPython / CircuitPython

### Protobuf

To generate the protobuf files and code:

```bash
make gen-py
```

NOTE: This is only for reference, because native gRPC requires Google's protobuf package, and this is too large for microcontrollers, we implement our own encode/decode logic in these files:
- python/lib/C110PSerial/proto_encode.py
- python/lib/C110PSerial/proto_decode.py

### lib/C110PSerial

This contains a stripped down logic to send/receive messages from [python/c110p_serial.proto](python/c110p_serial.proto)

Otherwise, it mirrors the native C++ approach.

Copy the [python/lib/C110PSerial] folder to your microcontroller in a folder `C110PSerial` or equivalent.

TODO: make this simplier for MicroPython projects

#### Send

Specifying pins in MicroPython are defined here:
- https://docs.micropython.org/en/latest/library/machine.UART.html

```python
from machine import UART
from C110PSerial import C110PSerial, C110PRegion_REGION_BODY, C110PActuator_BODY_NECK

# setup the Serial port to use
uart = UART(1, 9600)
uart.init(9600, bits=8, parity=None, stop=1, rx=3, tx=1)

# init the protobuf with the specified pins
c110p_serial = C110PSerial(uart)

# use helper to format the message
msg = c110p_serial.createMoveCommand(
  C110PRegion_REGION_BODY,
  C110PActuator_BODY_NECK,
  100,
  150,
  200
)

# send over UART
result = c110p_serial.send(msg)
```

#### Receive / Process

Specifying pins in MicroPython are defined here:
- https://docs.micropython.org/en/latest/library/machine.UART.html

NOTE: the decode method makes it a Dict object.

```python
from machine import UART
from C110PSerial import C110PSerial

# setup the Serial port to use
uart = UART(1, 9600)
uart.init(9600, bits=8, parity=None, stop=1, rx=3, tx=1)

# init the protobuf with the specified pins
c110p_serial = C110PSerial(uart)

# define a void function to process messages by tupe
cb = lambda (msg): (
  # echo out what the message was
  # TODO: perform logic on message receive here
  print(f"target: {msg["target"]}, x: {msg["x"]}, y: {msg["y"]}, z: {msg["z"]}")
)
c110p_serial.setMoveCallback(cb)
# repeat for all commands you want to act on

# listen to RX line in non-blocking way for messages
# upon full message received, it'll execute defined callback
# for that message type
c110p_serial.processQueue()
```

### Tests

This project relies on Python and PyTest testing framework via VSCode.

```bash
make test-py
```