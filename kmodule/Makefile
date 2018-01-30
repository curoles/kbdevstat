# http://tldp.org/LDP/lkmpg/2.6/html/lkmpg.html#AEN181
# https://github.com/torvalds/linux/blob/master/Documentation/kbuild/modules.txt
#
#
mkfile_path := $(abspath $(lastword $(MAKEFILE_LIST)))
src_path := $(dir $(mkfile_path))

TARGET_MODULE  := kbdevstat
WARN    := -Wall
CC      := gcc

CFLAGS_${TARGET_MODULE}.o := -w -Wall -Werror -std=gnu99

BUILDSYSTEM_DIR:=/lib/modules/$(shell uname -r)/build
MSRC:=$(src_path)
# is it even possible to build somewhere else? O= did not work
MOUT:=$(src_path)

$(TARGET_MODULE)-objs := #igor_kmodule.o
obj-m := $(TARGET_MODULE).o

#${TARGET}.o: ${TARGET}.c


all : 
	# run kernel build system to make module
	echo $(mkfile_path)
	$(MAKE) -C $(BUILDSYSTEM_DIR) M=$(MSRC) modules

.PHONY: clean
clean:
	# run kernel build system to cleanup in current directory
	$(MAKE) -C $(BUILDSYSTEM_DIR) M=$(MSRC) clean

load:
	sudo insmod ./$(TARGET_MODULE).ko
	cat /proc/modules | grep $(TARGET_MODULE)
	cat /var/log/syslog | grep kbdevstat

unload:
	sudo rmmod ./$(TARGET_MODULE).ko
	cat /var/log/syslog | grep kbdevstat

modinfo:
	sudo modinfo $(MOUT)/$(TARGET_MODULE).ko

