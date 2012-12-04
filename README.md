fmcomms1-eeprom-cal
===================

Build example:
dave@hal9000:./fmcomms1-eeprom-cal$ CC=microblazeel-unknown-linux-gnu-gcc make

Run example:
/bin/xcomm_cal /sys/bus/i2c/devices/0-0055/eeprom -f 2400 -s
