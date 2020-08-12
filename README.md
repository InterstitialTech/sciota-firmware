# sciota-firmware

microcontroller firmware for the Sciota IoT device

## Build + Flash
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

