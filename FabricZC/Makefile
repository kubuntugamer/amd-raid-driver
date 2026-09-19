obj-m += fabriczc_mod.o
fabriczc_mod-objs := src/kernel/main.o

# Manually assert core missing kernel layout symbols straight through ccflags
ccflags-y := -I$(src)/staging_includes \
             -DCONFIG_SMP=1 \
             -DCONFIG_X86_64=1 \
             -DCONFIG_PAGE_SHIFT=12 \
             -DCONFIG_HZ=250 \
             -DCONFIG_X86_L1_CACHE_SHIFT=6 \
             -DCONFIG_X86_INTERNODE_CACHE_SHIFT=6 \
             -DCONFIG_INLINE_SPIN_UNLOCK=1 \
             -DCONFIG_UNINLINE_SPIN_UNLOCK=1 \
             -DCONFIG_PREEMPT_RCU=1 \
             -std=gnu11 -Wno-unused-variable -Wno-int-conversion -Wno-missing-prototypes

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
