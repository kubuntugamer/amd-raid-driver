obj-m += fabriczc_mod.o
fabriczc_mod-y := src/core/main.o src/core/block_dev.o src/core/power_mgmt.o src/fs/fs_main.o src/fs/metadata_io.o

# Enforce explicit header search visibility for the Kbuild compiler subsystem
ccflags-y := -I$(src)/include

KDIR ?= /lib/modules/$(shell uname -r)/build
PWD  := $(shell pwd)
OUT  := $(PWD)/build/modules

all:
	@mkdir -p $(OUT)
	$(MAKE) -C $(KDIR) M=$(PWD) modules
	@cp -f fabriczc_mod.ko $(OUT)/
	@mv -f src/core/*.o src/fs/*.o .*.cmd *.mod.c *.mod *.o modules.order Module.symvers $(OUT)/ 2>/dev/null || true

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
	@rm -rf build/modules/* .*.cmd *.mod.c *.mod *.o *.ko modules.order Module.symvers .tmp_versions
