obj-m += fabriczc_mod.o
fabriczc_mod-objs := src/kernel/main.o

# Force clean preprocessor parameters to completely clear out internal struct redefinitions
ccflags-y := -I$(src)/staging_includes \
             -D__KERNEL__ \
             -DMODULE \
             -DCONFIG_AS_ISYNC=1 \
             -std=gnu11 \
             -Wno-unused-variable \
             -Wno-int-conversion \
             -Wno-missing-prototypes \
             -Wno-implicit-function-declaration

KVERSION ?= $(shell uname -r)
KDIR     := /lib/modules/$(KVERSION)/build
PWD      := $(shell pwd)

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
