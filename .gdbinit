target extended-remote localhost:3333
monitor reset init
load
skip file **/tinyusb/**
#layout regs
#tbreak main
continue
