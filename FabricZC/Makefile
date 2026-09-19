obj-m += fabriczc_mod.o
fabriczc_mod-objs := src/kernel/main.o

# Force include local staging paths while applying architecture-agnostic switches
ccflags-y := -I$(src)/staging_includes -std=gnu11 -Wno-unused-variable -Wno-int-conversion

# Dynamically resolve the active kernel release running in current system memory
KVERSION ?= $(shell uname -r)
KDIR     := /lib/modules/$(KVERSION)/build
PWD      := $(shell pwd)

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
