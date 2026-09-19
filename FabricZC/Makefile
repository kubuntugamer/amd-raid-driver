obj-m += fabriczc_mod.o
fabriczc_mod-objs := src/kernel/main.o

ccflags-y := -I$(src)/staging_includes -std=gnu11 -Wno-unused-variable -Wno-int-conversion

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
