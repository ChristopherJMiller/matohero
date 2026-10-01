#include <windows.h>
#include <string.h>

#include "bgproc.h"
#include "listman.h"

/* Background processing on PC (PS2: nucore/ps2/bgproc_ps2.c): a list of requests guarded
 * by a critical section, worked through by a loader thread woken with an event, or at the
 * end of each frame when not multithreaded. Names from the PS2 prototype; the file name
 * and the PC statics' names are ours. */

// GLOBAL: BIONICLE 0x617244
int multithreaded = 1;

// GLOBAL: BIONICLE 0x34c4284
static HANDLE bg_event;

// GLOBAL: BIONICLE 0x34c4288
static NULSTHDR *bg_list;

// GLOBAL: BIONICLE 0x34c428c
static volatile BGPROCINFO *bg_active;

// GLOBAL: BIONICLE 0x34c4290
static CRITICAL_SECTION bg_cs;

// GLOBAL: BIONICLE 0x34c42ac
static HANDLE bg_thread;

// GLOBAL: BIONICLE 0x34c42b0
static DWORD bg_threadid;

// GLOBAL: BIONICLE 0x34c42b4
static int bg_init;

// FUNCTION: BIONICLE 0x56b170
BGPROCINFO *bgPostRequest(bgprocdofn *dofn, bgprocackfn *ackfn, void *data, int datasize) {
    BGPROCINFO *i;

    EnterCriticalSection(&bg_cs);
    i = (BGPROCINFO *)NuLstAllocTail(bg_list);
    if (i != NULL) {
        i->DoFn = dofn;
        i->AckFn = ackfn;
        i->posted = 0;
        i->abort = 0;
        i->vars = NULL;
        i->var_cnt = 0;
        if (data != NULL && datasize != 0) {
            memcpy(i->wkspace, data, datasize);
        }
    }
    LeaveCriticalSection(&bg_cs);
    SetEvent(bg_event);
    return i;
}

// FUNCTION: BIONICLE 0x56b200
static DWORD WINAPI bgThreadMain(void *arg) {
    WaitForSingleObject(bg_event, INFINITE);
    EnterCriticalSection(&bg_cs);
    bg_active = (volatile BGPROCINFO *)NuLstGetNext(bg_list, NULL);
    LeaveCriticalSection(&bg_cs);
    for (;;) {
        do {
            bg_active->posted = 1;
            if (bg_active->DoFn != NULL) {
                (*bg_active->DoFn)((BGPROCINFO *)bg_active);
            }
            if (bg_active->AckFn != NULL) {
                (*bg_active->AckFn)((BGPROCINFO *)bg_active);
            }
            EnterCriticalSection(&bg_cs);
            NuLstFree((NULNKHDR *)bg_active);
            bg_active = (volatile BGPROCINFO *)NuLstGetNext(bg_list, NULL);
            LeaveCriticalSection(&bg_cs);
        } while (bg_active != NULL);
        do {
            WaitForSingleObject(bg_event, INFINITE);
            EnterCriticalSection(&bg_cs);
            bg_active = (volatile BGPROCINFO *)NuLstGetNext(bg_list, NULL);
            LeaveCriticalSection(&bg_cs);
        } while (bg_active == NULL);
    }
}

// FUNCTION: BIONICLE 0x56b2f0
void bgProcInit(void) {
    if (!bg_init) {
        bg_init = 1;
        bg_list = NuLstCreate(16, sizeof(BGPROCINFO));
        InitializeCriticalSection(&bg_cs);
        // STRING: BIONICLE 0x5e0914
        bg_event = CreateEventA(NULL, FALSE, FALSE, "BGWorkToDo");
        if (multithreaded) {
            bg_thread = CreateThread(NULL, 0x10000, bgThreadMain, NULL, CREATE_SUSPENDED, &bg_threadid);
            SetThreadPriority(bg_thread, THREAD_PRIORITY_NORMAL);
            ResumeThread(bg_thread);
        }
    }
}

// FUNCTION: BIONICLE 0x56b380
void bgProcAbortAll(void) {
    BGPROCINFO *i;
    BGPROCINFO *nxt;

    for (i = (BGPROCINFO *)NuLstGetNext(bg_list, (NULNKHDR *)bg_active); i != NULL; i = nxt) {
        nxt = (BGPROCINFO *)NuLstGetNext(bg_list, (NULNKHDR *)i);
        i->abort = 1;
        if (i->AckFn != NULL) {
            (*i->AckFn)(i);
        }
        NuLstFree((NULNKHDR *)i);
    }
    if (bg_active != NULL) {
        bg_active->abort = 1;
    }
}

// FUNCTION: BIONICLE 0x56b3f0
void NuFrameEndBgLoadPS(int min_delay) {
    if (!multithreaded) {
        for (bg_active = (volatile BGPROCINFO *)NuLstGetNext(bg_list, NULL); bg_active != NULL;
             bg_active = (volatile BGPROCINFO *)NuLstGetNext(bg_list, NULL)) {
            bg_active->posted = 1;
            if (bg_active->DoFn != NULL) {
                (*bg_active->DoFn)((BGPROCINFO *)bg_active);
            }
            if (bg_active->AckFn != NULL) {
                (*bg_active->AckFn)((BGPROCINFO *)bg_active);
            }
            NuLstFree((NULNKHDR *)bg_active);
        }
    } else if (bg_list != NULL) {
        if (bg_active != NULL || NuLstGetNext(bg_list, NULL) != NULL) {
            Sleep(1);
        }
    }
}
