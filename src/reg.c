#include <R_ext/Rdynload.h>

void fake_init_ulog(DllInfo *info) {
  /* really silly, but required */
  R_registerRoutines(info, 0, 0, 0, 0);
  R_useDynamicSymbols(info, FALSE);
}
