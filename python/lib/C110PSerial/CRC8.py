class CRC8:
    # CRC-8 parameters (example, can be changed for different CRCs)
    POLY = 0x07
    INIT = 0x00
    XOROUT = 0x00

    def __init__(self):
        self._table = self._build_table()

    @staticmethod
    def _calculate_byte(byte):
        crc = byte
        for _ in range(8):
            if crc & 0x80:
                crc = ((crc << 1) ^ CRC8.POLY) & 0xFF
            else:
                crc = (crc << 1) & 0xFF
        return crc

    def _build_table(self):
        table = []
        for i in range(256):
            table.append(self._calculate_byte(i))
        return table

    def calculate(self, data):
        crc = self.INIT
        for b in data:
            crc = self._table[crc ^ b]
        return crc ^ self.XOROUT
