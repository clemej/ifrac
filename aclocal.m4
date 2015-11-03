dnl ------------------------------
dnl SADP configuration macros
dnl ------------------------------


AC_DEFUN(SVGA_SIUDROOT,
[
AC_MSG_CHECKING([for suid root with svgalib])

AC_TRY_RUN([
#include <vga.h>

int main(void)
{
	return (vga_setmode(-1) >= 0x1900) ? 0 : 4;
}
],
with_suidroot=no, with_suidroot=yes, with_suidroot=no)

AC_MSG_RESULT($with_suidroot)
])
