import pytest
from unittest.mock import MagicMock, patch, call


@pytest.fixture
def stream_mock():
    return MagicMock()


@pytest.fixture
def C110PCommand():
    def fn(id=0, cmd_type='ack'):
        if cmd_type == 'led':
            return {
                'id': id,
                'source': 0,
                'target': 0,
                'led': {
                    'start': 0,
                    'end': 0,
                    'duration': 0
                }
            }
        elif cmd_type == 'move':
            return {
                'id': id,
                'source': 0,
                'target': 0,
                'move': {
                    'target': 0,
                    'x': 0,
                    'y': 0,
                    'z': 0
                }
            }
        elif cmd_type == 'sound':
            return {
                'id': id,
                'source': 0,
                'target': 0,
                'sound': {
                    'id': 0,
                    'play': False,
                    'syncToLeds': False
                }
            }
        elif cmd_type == 'ack':
            return {
                'id': id,
                'source': 0,
                'target': 0,
                'ack': {
                    'acknowledged': False,
                    'reason': ''
                }
            }
        else:
            raise ValueError("Unknown command type")
    return fn
