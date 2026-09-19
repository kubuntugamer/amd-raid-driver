#ifndef __FABRICZC_AUTOCONF_H__
#define __FABRICZC_AUTOCONF_H__
#define CONFIG_SMP 1
#define CONFIG_64BIT 1
#define CONFIG_MODULES 1
#define MODULE 1
#define __KERNEL__ 1
#define CONFIG_X86_64 1
#define CONFIG_X86 1
#define CONFIG_PGTABLE_LEVELS 4
#define CONFIG_PAGE_SHIFT 12
#define CONFIG_X86_L1_CACHE_SHIFT 6
#define CONFIG_X86_INTERNODE_CACHE_SHIFT 6
#define CONFIG_AMD_MEM_ENCRYPT 1
#define CONFIG_TREE_RCU 1
#define CONFIG_PREEMPT_RCU 1
#define CONFIG_NR_CPUS 64
#define CONFIG_X86_MINIMUM_CPU_FAMILY 64
#define CONFIG_X86_CMPXCHG64 1
#define CONFIG_AS_IS_VERSION 1
#define __OPTIMIZE__ 1

/* Core Scheduler Timing Configurations */
#define CONFIG_HZ 250
#define HZ 250

/* Low-level x86 register encryption parameters mapping */
extern unsigned long sme_me_mask;
static inline unsigned long __sme_pa(void *vaddr) {
    return ((unsigned long)(vaddr) - 0xffff888000000000UL);
}

#define IS_ENABLED(option) __is_defined(option)
#define __is_defined(x) ___is_defined(x)
#define ___is_defined(val) ____is_defined(__ARG_PLACEHOLDER_##val)
#define ____is_defined(arg1_or_junk) __take_second(arg1_or_junk 1, 0)
#define __take_second(arg1, arg2, ...) arg2
#define __ARG_PLACEHOLDER_1 ,
#endif
