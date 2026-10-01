#ifndef NUTHREAD_H
#define NUTHREAD_H

#include "nutypes.h"

typedef void nuthreaddisableswapfn(void);
typedef void nuthreadenableswapfn(void);

extern u32 nu_current_thread_id;
extern nuthreaddisableswapfn *NuThreadDisableThreadSwap;
extern nuthreadenableswapfn *NuThreadEnableThreadSwap;

#endif
