from .Logger import logger

class RingBuffer:
    RING_BUFFER_SIZE = 25

    def __init__(self):
        self._buffer = [None] * self.RING_BUFFER_SIZE
        self._head = 0
        self._tail = 0
        self._size = 0
        self._message_map = {}

    def __contains__(self, timestamp):
        return self.contains(timestamp)

    def __getitem__(self, timestamp):
        return self.get(timestamp)

    def __len__(self):
        return self.size()

    def reset(self):
        self._head = 0
        self._tail = 0
        self._size = 0
        self._message_map.clear()

    def add(self, message):
        if message is None or not isinstance(message, dict):
            logger.error(f"Invalid message type, expected dict but got {type(message).__name__}")
            return
        
        timestamp = message['id']  # Assumes message is an object with getter/setter for 'id'
        if self.contains(timestamp):
            return

        if self._size < self.RING_BUFFER_SIZE:
            self._size += 1
        else:
            # Remove the oldest message from the map
            old_id = self._buffer[self._tail]['id']
            self._message_map.pop(old_id, None)
            self._tail = (self._tail + 1) % self.RING_BUFFER_SIZE

        self._buffer[self._head] = message
        self._message_map[timestamp] = True
        self._head = (self._head + 1) % self.RING_BUFFER_SIZE

    def contains(self, timestamp):
        return timestamp in self._message_map

    def get(self, timestamp):
        for i in range(self._size):
            idx = (self._tail + i) % self.RING_BUFFER_SIZE
            if self._buffer[idx] and self._buffer[idx]['id'] == timestamp:
                return self._buffer[idx]
        return None

    def getCurrentValue(self):
        if self._size == 0:
            return None
        idx = self._head - 1 if self._head > 0 else self.RING_BUFFER_SIZE - 1
        return self._buffer[idx]

    def getMessageMap(self):
        return self._message_map.copy()

    def size(self):
        return self._size
