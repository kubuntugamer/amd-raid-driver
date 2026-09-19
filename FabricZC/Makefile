obj-m += fabriczc_mod.o
fabriczc_mod-objs := src/kernel/main.o

ccflags-y := -I$(src)/staging_includes \
             -I$(src)/staging_includes/generated \
             -I/usr/src/linux-headers-$(shell uname -r)/include \
             -I/usr/src/linux-headers-$(shell uname -r)/include/uapi \
             -I/usr/src/linux-headers-$(shell uname -r)/arch/x86/include \
             -I/usr/src/linux-headers-$(shell uname -r)/arch/x86/include/uapi \
             -I/usr/src/linux-headers-$(shell uname -r)/arch/x86/include/generated \
             -I/usr/src/linux-headers-$(shell uname -r)/arch/x86/include/generated/uapi \
             -D__KERNEL__ -DMODULE \
             -DCONFIG_PAGE_SHIFT=12 \
             -DCONFIG_HZ=250 \
             -DCONFIG_X86_L1_CACHE_SHIFT=6 \
             -DCONFIG_X86_INTERNODE_CACHE_SHIFT=6 \
             -std=gnu11 -Wno-unused-variable -Wno-int-conversion -Wno-missing-prototypes \
             -include $(src)/staging_includes/generated/autoconf.h

all:
	gcc -nostdinc -std=gnu11 \
	    -D__KERNEL__ -DMODULE \
	    -include $(PWD)/staging_includes/generated/autoconf.h \
	    -I$(PWD)/staging_includes \
	    -I$(PWD)/staging_includes/generated \
	    -I/usr/src/linux-headers-$(shell uname -r)/include \
	    -I/usr/src/linux-headers-$(shell uname -r)/include/uapi \
	    -I/usr/src/linux-headers-$(shell uname -r)/arch/x86/include \
	    -I/usr/src/linux-headers-$(shell uname -r)/arch/x86/include/uapi \
	    -I/usr/src/linux-headers-$(shell uname -r)/arch/x86/include/generated \
	    -I/usr/src/linux-headers-$(shell uname -r)/arch/x86/include/generated/uapi \
	    -O2 -Wall -m64 -mcmodel=kernel -mno-red-zone -fno-pie \
	    -c src/kernel/main.c -o src/kernel/main.o
	gcc -nostdinc -std=gnu11 \
	    -D__KERNEL__ -DMODULE \
	    -I$(PWD)/staging_includes \
	    -I/usr/src/linux-headers-$(shell uname -r)/include \
	    -I/usr/src/linux-headers-$(shell uname -r)/include/uapi \
	    -I/usr/src/linux-headers-$(shell uname -r)/arch/x86/include \
	    -I/usr/src/linux-headers-$(shell uname -r)/arch/x86/include/uapi \
	    -I/usr/src/linux-headers-$(shell uname -r)/arch/x86/include/generated \
	    -I/usr/src/linux-headers-$(shell uname -r)/arch/x86/include/generated/uapi \
	    -O2 -fno-pie \
	    -c src/kernel/fabriczc_mod.mod.c -o src/kernel/fabriczc_mod.mod.o
	ld -r -m elf_x86_64 -z max-page-size=0x1000 -o fabriczc_mod.ko src/kernel/main.o src/kernel/fabriczc_mod.mod.o

clean:
	rm -f fabriczc_mod.ko src/kernel/*.o src/kernel/.*.cmd 2>/dev/null || true
