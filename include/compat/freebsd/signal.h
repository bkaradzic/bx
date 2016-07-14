#ifdef BX_CRT_GLIBC
#include_next <signal.h>
#else
#include <sys/signal.h>
#endif
