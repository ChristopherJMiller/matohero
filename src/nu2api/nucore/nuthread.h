#ifndef NUTHREAD_H
#define NUTHREAD_H

typedef void nuthreaddisableswapfn(void);

extern unsigned int nu_current_thread_id;
extern nuthreaddisableswapfn *NuThreadDisableThreadSwap;
extern nuthreaddisableswapfn *NuThreadEnableThreadSwap;

int NuThreadCreateCriticalSection(void);
void NuThreadCriticalSectionBegin(int SectionID);
void NuThreadCriticalSectionEnd(int SectionID);

#endif
