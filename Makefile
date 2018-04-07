ARDUINO_BASE           = /data1/projects/arduino
ARDUINO_DIR            = $(ARDUINO_BASE)/arduino-1.6.8
TARGET                 = lcdTerminal
ARDUINO_LIBS           = LiquidCrystal
MCU                    = atmega328p
F_CPU                  = 16000000
ARDUINO_PORT           = /dev/ttyUSB0
AVRDUDE_ARD_BAUDRATE   = 57600
AVRDUDE_ARD_PROGRAMMER = arduino
BOARD_TAG              = nano
ARDUINO_TOOLS_PATH     = $(ARDUINO_DIR)/hardware/tools
ARDUINO_ETC_PATH       = $(ARDUINO_TOOLS_PATH)/avr/etc
AVR_TOOLS_PATH         = $(ARDUINO_TOOLS_PATH)/avr/bin

include $(ARDUINO_BASE)/Arduino.make
