import logging
import sys

# Configure basic logging to stdout
logging.basicConfig(
    level=logging.INFO,
    format='[%(levelname)s][%(filename)s:%(lineno)d] %(message)s',
    stream=sys.stdout
)

logger = logging.getLogger(__name__)
