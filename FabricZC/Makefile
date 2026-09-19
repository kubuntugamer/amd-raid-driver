obj-m += fabriczc_mod.o
fabriczc_mod-y := src/kernel/main.o

# Force include our custom staging headers path during the kbuild execution pass
ccflags-y := -I$(src)/staging_includes -std=gnu11 -Wno-unused-variable

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
