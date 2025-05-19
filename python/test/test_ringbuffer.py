import pytest
from C110PSerial.RingBuffer import RingBuffer


def MockTestMessage(ts=0, pl=""):
    return {
        'id': ts,
        'data': {
            'payload': pl[:15]
        },
        'timestamp': ts,
        'payload': pl[:15]
    }

def test_add_and_contains():
    buf = RingBuffer()
    msg1 = MockTestMessage(1, "foo")
    msg2 = MockTestMessage(2, "bar")

    buf.add(msg1)
    buf.add(msg2)

    assert buf.contains(1)
    assert buf.contains(2)
    assert not buf.contains(3)

def test_no_duplicates():
    buf = RingBuffer()
    msg1 = MockTestMessage(1, "foo")
    buf.add(msg1)
    buf.add(msg1)  # duplicate

    assert buf.contains(1)
    assert buf.get(1)["id"] == 1

def test_get_returns_correct_message():
    buf = RingBuffer()
    buf.add(MockTestMessage(10, "abc"))
    buf.add(MockTestMessage(20, "def"))

    msg = buf.get(20)
    assert msg is not None
    assert msg["id"] == 20
    assert msg["data"]["payload"] == "def"

def test_get_returns_none_if_not_found():
    buf = RingBuffer()
    buf.add(MockTestMessage(10, "abc"))
    assert buf.get(99) is None

def test_buffer_wraps_and_evicts_oldest():
    OFFSET = 100
    buf = RingBuffer()
    size = getattr(buf, 'RING_BUFFER_SIZE', 8)  # fallback if not defined
    for i in range(size):
        buf.add(MockTestMessage(i, "x"))
    buf.add(MockTestMessage(size + OFFSET, "new"))

    assert not buf.contains(0)
    assert buf.contains(size + OFFSET)
    assert buf.get(0) is None
    assert buf.get(size + OFFSET) is not None

def test_get_current_value_returns_last_added():
    buf = RingBuffer()
    assert getattr(buf.getCurrentValue(), 'id', 0) == 0

    buf.add(MockTestMessage(5, "a"))
    assert buf.getCurrentValue()["id"] == 5

    buf.add(MockTestMessage(6, "b"))
    assert buf.getCurrentValue()["id"] == 6

def test_reset_clears_buffer():
    buf = RingBuffer()
    buf.add(MockTestMessage(1, "foo"))
    buf.add(MockTestMessage(2, "bar"))
    buf.reset()

    assert not buf.contains(1)
    assert not buf.contains(2)
    assert buf.get(1) is None
    assert getattr(buf.getCurrentValue(), 'id', 0) == 0

def test_put_behaves_like_add():
    buf = RingBuffer()
    buf.add(MockTestMessage(42, "hello"))
    assert buf.contains(42)
    assert buf.get(42)["id"] == 42

    buf.add(MockTestMessage(42, "world"))
    assert buf.get(42)["data"]["payload"] == "hello"

if __name__ == "__main__":
    pytest.main([__file__])
