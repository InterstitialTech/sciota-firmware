#ifndef THERMOMETER_H
#define THERMOMETER_H

#define MCP9808_ADDR7 0x18
#define MCP9808_REG_TEMP 0x05

void thermometer_setup(void);
void thermometer_init(void);
float thermometer_read(void);

#endif
