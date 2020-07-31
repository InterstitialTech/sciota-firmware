PROJECT = blinky
SRC_DIR = src
BUILD_DIR = bin

CFILES += $(SRC_DIR)/main.c

DEVICE = stm32f446re

# Open OCD stuff
OOCD_INTERFACE = stlink-v2
OOCD_TARGET = stm32f4x

OPENCM3_DIR = ${HOME}/software/libopencm3

include $(OPENCM3_DIR)/mk/genlink-config.mk
include rules.mk
include $(OPENCM3_DIR)/mk/genlink-rules.mk
