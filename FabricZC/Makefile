# FabricZC Unified Out-Of-Tree Kbuild Configuration
obj-m += fabriczc_mod.o
obj-m += fabriczc_fs_mod.o

# Force the engine to evaluate local directories relative to the execution root
fabriczc_mod-y := src/core/main.o src/core/block_dev.o src/core/power_mgmt.o
fabriczc_fs_mod-y := src/fs/fs_main.o src/fs/metadata_io.o

ccflags-y := -I$(src)/include -Werror

all:
	$(MAKE) -C /lib/modules/$(shell uname -r)/build M=$(PWD) src=$(PWD) modules

clean:
	$(MAKE) -C /lib/modules/$(shell uname -r)/build M=$(PWD) src=$(PWD) clean
