obj-m += fabriczc_mod.o
fabriczc_mod-objs := src/kernel/main.o

# Assert clean standard out-of-tree module compilation parameters
ccflags-y := -I$(src)/staging_includes \
             -D__KERNEL__ \
             -DMODULE \
             -std=gnu11 \
             -Wno-unused-variable \
             -Wno-int-conversion \
             -Wno-missing-prototypes

KVERSION ?= $(shell uname -r)
KDIR     := /lib/modules/$(KVERSION)/build
PWD      := $(shell pwd)

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
