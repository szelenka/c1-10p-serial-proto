try:
    import logging
except ImportError:
    import adafruit_logging as logging
import sys

def configure_logger(logger, stream, fmt, level):
    # Check if a handler with the same stream and formatter already exists
    for handler in logger.handlers if hasattr(logger, 'handlers') else logger._handlers:
        if isinstance(handler, logging.StreamHandler) and handler.stream == stream:
            if handler.formatter and handler.formatter._fmt == fmt and logger.level == level:
                return  # Already configured as desired

    # Remove all handlers and add the new one
    for handler in logger.handlers if hasattr(logger, 'handlers') else logger._handlers:
        logger.removeHandler(handler)
    handler = logging.StreamHandler(stream)
    handler.setFormatter(logging.Formatter(fmt))
    logger.addHandler(handler)
    logger.setLevel(level)


if not hasattr(logging, "basicConfig"):
    def basicConfig(**kwargs):
        stream = kwargs.get("stream", sys.stdout)
        level = kwargs.get("level", logging.INFO)
        fmt = kwargs.get("format", '[%(levelname)s] %(message)s')
        configure_logger(logging.getLogger(), stream, fmt, level)
    logging.basicConfig = basicConfig


logger = logging.getLogger(__name__)

basicConfig(
    stream=sys.stdout,
    level=logging.DEBUG,
    format='[%(levelname)s] %(message)s'
)
