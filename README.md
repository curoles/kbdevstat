# kbdevstat - Keyboard Input Events Statistics

## Build kernel module

```terminal
$ cd <SOURCE>/kmodule
$ make
$ modinfo ./kbdevstat.ko 
filename:       /home/igor/prj/github/kbdevstat/kbdevstat/kmodule/./kbdevstat.ko
license:        GPL
description:    Keyboard Events Statistics module
author:         Igor Lesik
version:        1.0.0
srcversion:     A9D604A20817F2100CC897A
depends:        
name:           kbdevstat
vermagic:       4.13.0-32-generic SMP mod_unload 686
```

## Load and unload kernel module

```terminal
$ sudo insmod ./kbdevstat.ko
$ dmesg
[29881.532026] kbdevstat: Initializing
$ sudo rmmod ./kbdevstat.ko
$dmesg
[29934.429326] kbdevstat: Exiting after 92 interrupts
```
