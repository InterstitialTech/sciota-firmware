PROJECT = blinky
SRC_DIR = src
BUILD_DIR = bin

CFILES += $(SRC_DIR)/main.c
CFILES += $(SRC_DIR)/leds.c
CFILES += $(SRC_DIR)/serial.c
CFILES += $(SRC_DIR)/thermometer.c
CFILES += $(SRC_DIR)/modem.c

INCLUDES += -I include

DEVICE = stm32l152re

# Open OCD stuff
OOCD_INTERFACE = stlink-v2
OOCD_TARGET = stm32l1

OPENCM3_DIR = ${HOME}/software/libopencm3

include $(OPENCM3_DIR)/mk/genlink-config.mk
include rules.mk
include $(OPENCM3_DIR)/mk/genlink-rules.mk
