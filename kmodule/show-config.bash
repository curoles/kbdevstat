#!/bin/bash

KERNEL_CONFIG=/boot/config-`uname -r`
echo
echo "Grepping ${KERNEL_CONFIG} for \"KEYB\""
echo "======================================"
cat $KERNEL_CONFIG | grep -i --color KEYB

echo
echo "Grepping ${KERNEL_CONFIG} for \"KBD\""
echo "======================================"
cat $KERNEL_CONFIG | grep -i --color KBD

echo
echo "xinput --list"
echo "============="
xinput --list

echo
echo "sudo dmidecode | grep keyb"
echo "=========================="
sudo dmidecode | grep -i --color keyb

echo
echo "cat /proc/interrupts | grep i8042"
echo "================================="
cat /proc/interrupts | grep --color i8042

echo
echo "dmidecode"
echo "========="
sudo dmidecode --type 17 | grep -i -e "memory\|size\|speed"

#echo "sudo lshw"
#echo "lspci"
#echo "lsusb"
