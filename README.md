# sciota-firmware

microcontroller firmware for the Sciota IoT device

## Dependencies
* gcc-arm-none-eabi
* binutils-arm-none-eabi
* libnewlib-arm-none-eabi
* openocd
* libopencm3


## Build + Flash

Make sure that your libopencm3 directory is pointed to by $(OPENCM_DIR) in
Makefile. Then,

```
make
make flash
```

## Pinout
```
PA13 <--> SWDIO
PA14 <--> SWCLK
PA2  <--> Modem TX
PA3  <--> Modem RX
PC10 <--> Debug TX
PC11 <--> Debug RX

```
(with TX/RX defined from the perspective of the MCU)

