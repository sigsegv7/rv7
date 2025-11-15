AC_INIT([rv7], [0.0.1], [ian@osmora.org])
AC_PROG_CC

CC=$(pwd)/cc/gcc/bin/x86_64-pc-osmora-gcc
LD=$(pwd)/cc/toolchain/build-binutils/bin/x86_64-pc-osmora-ld
ARCH="amd64"

MD_CFLAGS="-mno-sse -mno-sse2 -mno-sse3 -mno-mmx -mno-3dnow -mno-avx -mno-avx2"
CFLAGS="-nostdlib -nostdinc -ffreestanding  -mcmodel=kernel -fno-stack-protector $MD_CFLAGS"

AC_SUBST(SYS_CFLAGS, [$CFLAGS])
AC_SUBST(CC, [$CC])
AC_SUBST(LD, [$LD])
AC_SUBST(ARCH, [$ARCH])
AC_CONFIG_FILES([Makefile])
AC_OUTPUT
