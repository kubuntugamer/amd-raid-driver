# Top-Level Makefile for standalone FabricZC module development stack
# Halts cleanly on individual compilation faults for granular debugging.

obj-m += src/core/fabriczc_mod.o
obj-m += src/fs/fabriczc_fs_mod.o

fabriczc_mod-y := src/core/main.o src/core/block_dev.o src/core/power_mgmt.o
fabriczc_fs_mod-y := src/fs/fs_main.o src/fs/metadata_io.o

ccflags-y := -I$(src)/include -Werror

all:
	$(MAKE) -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	$(MAKE) -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
