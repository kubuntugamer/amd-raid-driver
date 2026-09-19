# FabricZC Standalone Sandbox Module Construction Engine
obj-m += fabriczc_mod.o
fabriczc_mod-objs := src/kernel/main.o

# Identify live kernel environment tracking targets
KVERSION ?= $(shell uname -r)
KDIR ?= /lib/modules/$(KVERSION)/build
PWD ?= $(shell pwd)

# Inject explicit include parameters to bypass global Kconfig path checks entirely
EXTRA_CFLAGS += -I$(PWD)/include \
                -I$(KDIR)/include \
                -I$(KDIR)/arch/x86/include \
                -I$(KDIR)/include/generated \
                -I$(KDIR)/arch/x86/include/generated \
                -D__KERNEL__ -DMODULE

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
	rm -f src/kernel/*.o src/kernel/.*.cmd *.ko *.mod *.mod.c *.o .*.cmd
