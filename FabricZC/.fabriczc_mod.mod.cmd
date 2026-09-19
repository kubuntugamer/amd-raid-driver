savedcmd_fabriczc_mod.mod := printf '%s\n'   src/kernel/main.o | awk '!x[$$0]++ { print("./"$$0) }' > fabriczc_mod.mod
