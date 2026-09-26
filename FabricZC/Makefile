obj-m += fabriczc_mod.o

fabriczc_mod-y := src/core/main.o src/core/block_dev.o src/core/power_mgmt.o \
                  src/fs/fs_main.o src/fs/metadata_io.o

ccflags-y := -I$(src)/include -Werror

all:
	$(MAKE) -C /lib/modules/$(shell uname -r)/build M=$(PWD) src=$(PWD) modules

clean:
	$(MAKE) -C /lib/modules/$(shell uname -r)/build M=$(PWD) src=$(PWD) clean
