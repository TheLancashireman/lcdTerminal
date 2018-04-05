ARDUINO_DIR            = /data/projects/arduino/arduino-1.5.5
TARGET                 = lcdTerminal
ARDUINO_LIBS           = LiquidCrystal
MCU                    = atmega328p
F_CPU                  = 16000000
ARDUINO_PORT           = /dev/ttyUSB0
AVRDUDE_ARD_BAUDRATE   = 115200
AVRDUDE_ARD_PROGRAMMER = arduino
BOARD_TAG              = nano
ARDUINO_TOOLS_PATH     = $(ARDUINO_DIR)/hardware/tools
ARDUINO_ETC_PATH       = $(ARDUINO_TOOLS_PATH)
AVR_TOOLS_PATH         = $(ARDUINO_TOOLS_PATH)/avr/bin
AVRDUDE_ARD_BAUDRATE   = 57600

include ./Arduino.mk
