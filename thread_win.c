#if defined(_WIN32) && !defined(CORE_SDL)

#include "quakedef.h"
#include "thread.h"
#include <process.h>

int Thread_Init(void)
{
#ifdef THREADDISABLE
	Con_Printf("Threading disabled in this build\n");
#endif
	return 0;
}

void Thread_Shutdown(void)
{
}

qbool Thread_HasThreads(void)
{
#ifdef THREADDISABLE
	return false;
#else
	return true;
#endif
}

void *_Thread_CreateMutex(const char *filename, int fileline)
{
	void *mutex = (void *)CreateMutex(NULL, FALSE, NULL);
#ifdef THREADDEBUG
	Sys_PrintfToTerminal("%p mutex create %s:%d\n" , mutex, filename, fileline);
#endif
	return mutex;
}

void _Thread_DestroyMutex(void *mutex, const char *filename, int fileline)
{
#ifdef THREADDEBUG
	Sys_PrintfToTerminal("%p mutex destroy %s:%d\n", mutex, filename, fileline);
#endif
	CloseHandle(mutex);
}

int _Thread_LockMutex(void *mutex, const char *filename, int fileline)
{
#ifdef THREADDEBUG
	Sys_PrintfToTerminal("%p mutex lock %s:%d\n"   , mutex, filename, fileline);
#endif
	return (WaitForSingleObject(mutex, INFINITE) == WAIT_FAILED) ? -1 : 0;
}

int _Thread_UnlockMutex(void *mutex, const char *filename, int fileline)
{
#ifdef THREADDEBUG
	Sys_PrintfToTerminal("%p mutex unlock %s:%d\n" , mutex, filename, fileline);
#endif
	return (ReleaseMutex(mutex) == false) ? -1 : 0;
}

typedef struct thread_semaphore_s
{
	HANDLE semaphore;
	volatile LONG value;
}
thread_semaphore_t;

static thread_semaphore_t *Thread_CreateSemaphore(unsigned int v)
{
	thread_semaphore_t *s = (thread_semaphore_t *)calloc(sizeof(*s), 1);
	s->semaphore = CreateSemaphore(NULL, v, 32768, NULL);
	s->value = v;
	return s;
}

static void Thread_DestroySemaphore(thread_semaphore_t *s)
{
	CloseHandle(s->semaphore);
	free(s);
}

static int Thread_WaitSemaphore(thread_semaphore_t *s, unsigned int msec)
{
	int r = WaitForSingleObject(s->semaphore, msec);
	if (r == WAIT_OBJECT_0)
	{
		InterlockedDecrement(&s->value);
		return 0;
	}
	if (r == WAIT_TIMEOUT)
		return 1;
	return -1;
}

static int Thread_PostSemaphore(thread_semaphore_t *s)
{
	InterlockedIncrement(&s->value);
	if (ReleaseSemaphore(s->semaphore, 1, NULL))
		return 0;
	InterlockedDecrement(&s->value);
	return -1;
}

typedef struct thread_cond_s
{
	HANDLE mutex;
	int waiting;
	int signals;
	thread_semaphore_t *sem;
	thread_semaphore_t *done;
}
thread_cond_t;

void *_Thread_CreateCond(const char *filename, int fileline)
{
	thread_cond_t *c = (thread_cond_t *)calloc(sizeof(*c), 1);
	c->mutex = CreateMutex(NULL, FALSE, NULL);
	c->sem = Thread_CreateSemaphore(0);
	c->done = Thread_CreateSemaphore(0);
	c->waiting = 0;
	c->signals = 0;
#ifdef THREADDEBUG
	Sys_PrintfToTerminal("%p cond create %s:%d\n"   , c, filename, fileline);
#endif
	return c;
}

void _Thread_DestroyCond(void *cond, const char *filename, int fileline)
{
	thread_cond_t *c = (thread_cond_t *)cond;
#ifdef THREADDEBUG
	Sys_PrintfToTerminal("%p cond destroy %s:%d\n"   , cond, filename, fileline);
#endif
	Thread_DestroySemaphore(c->sem);
	Thread_DestroySemaphore(c->done);
	CloseHandle(c->mutex);
}

int _Thread_CondSignal(void *cond, const char *filename, int fileline)
{
	thread_cond_t *c = (thread_cond_t *)cond;
	int n;
#ifdef THREADDEBUG
	Sys_PrintfToTerminal("%p cond signal %s:%d\n"   , cond, filename, fileline);
#endif
	WaitForSingleObject(c->mutex, INFINITE);
	n = c->waiting - c->signals;
	if (n > 0)
	{
		c->signals++;
		Thread_PostSemaphore(c->sem);
	}
	ReleaseMutex(c->mutex);
	if (n > 0)
		Thread_WaitSemaphore(c->done, INFINITE);
	return 0;
}

int _Thread_CondBroadcast(void *cond, const char *filename, int fileline)
{
	thread_cond_t *c = (thread_cond_t *)cond;
	int i = 0;
	int n = 0;
#ifdef THREADDEBUG
	Sys_PrintfToTerminal("%p cond broadcast %s:%d\n"   , cond, filename, fileline);
#endif
	WaitForSingleObject(c->mutex, INFINITE);
	n = c->waiting - c->signals;
	if (n > 0)
	{
		c->signals += n;
		for (i = 0;i < n;i++)
			Thread_PostSemaphore(c->sem);
	}
	ReleaseMutex(c->mutex);
	for (i = 0;i < n;i++)
		Thread_WaitSemaphore(c->done, INFINITE);
	return 0;
}

int _Thread_CondWait(void *cond, void *mutex, const char *filename, int fileline)
{
	thread_cond_t *c = (thread_cond_t *)cond;
	int waitresult;
#ifdef THREADDEBUG
	Sys_PrintfToTerminal("%p cond wait %s:%d\n"   , cond, filename, fileline);
#endif

	WaitForSingleObject(c->mutex, INFINITE);
	c->waiting++;
	ReleaseMutex(c->mutex);

	ReleaseMutex(mutex);

	waitresult = Thread_WaitSemaphore(c->sem, INFINITE);
	WaitForSingleObject(c->mutex, INFINITE);
	if (c->signals > 0)
	{
		if (waitresult > 0)
			Thread_WaitSemaphore(c->sem, INFINITE);
		Thread_PostSemaphore(c->done);
		c->signals--;
	}
	c->waiting--;
	ReleaseMutex(c->mutex);

	WaitForSingleObject(mutex, INFINITE);
	return waitresult;
}

typedef struct threadwrapper_s
{
	HANDLE handle;
	unsigned int threadid;
	int result;
	int (*fn)(void *);
	void *data;
}
threadwrapper_t;

unsigned int __stdcall Thread_WrapperFunc(void *d)
{
	threadwrapper_t *w = (threadwrapper_t *)d;
	w->result = w->fn(w->data);
	_endthreadex(w->result);
	return w->result;
}

void *_Thread_CreateThread(int (*fn)(void *), void *data, const char *filename, int fileline)
{
	threadwrapper_t *w = (threadwrapper_t *)calloc(sizeof(*w), 1);
#ifdef THREADDEBUG
	Sys_PrintfToTerminal("%p thread create %s:%d\n"   , w, filename, fileline);
#endif
	w->fn = fn;
	w->data = data;
	w->threadid = 0;
	w->result = 0;
	w->handle = (HANDLE)_beginthreadex(NULL, 0, Thread_WrapperFunc, (void *)w, 0, &w->threadid);
	return (void *)w;
}

int _Thread_WaitThread(void *d, int retval, const char *filename, int fileline)
{
	threadwrapper_t *w = (threadwrapper_t *)d;
#ifdef THREADDEBUG
	Sys_PrintfToTerminal("%p thread wait %s:%d\n"   , w, filename, fileline);
#endif
	WaitForSingleObject(w->handle, INFINITE);
	CloseHandle(w->handle);
	retval = w->result;
	free(w);
	return retval;
}

// standard barrier implementation using conds and mutexes
// see: http://www.howforge.com/implementing-barrier-in-pthreads
typedef struct {
	unsigned int needed;
	unsigned int called;
	void *mutex;
	void *cond;
} barrier_t;

void *_Thread_CreateBarrier(unsigned int count, const char *filename, int fileline)
{
	volatile barrier_t *b = (volatile barrier_t *) Z_Malloc(sizeof(barrier_t));
#ifdef THREADDEBUG
	Sys_PrintfToTerminal("%p barrier create(%d) %s:%d\n", b, count, filename, fileline);
#endif
	b->needed = count;
	b->called = 0;
	b->mutex = Thread_CreateMutex();
	b->cond = Thread_CreateCond();
	return (void *) b;
}

void _Thread_DestroyBarrier(void *barrier, const char *filename, int fileline)
{
	volatile barrier_t *b = (volatile barrier_t *) barrier;
#ifdef THREADDEBUG
	Sys_PrintfToTerminal("%p barrier destroy %s:%d\n", b, filename, fileline);
#endif
	Thread_DestroyMutex(b->mutex);
	Thread_DestroyCond(b->cond);
}

void _Thread_WaitBarrier(void *barrier, const char *filename, int fileline)
{
	volatile barrier_t *b = (volatile barrier_t *) barrier;
#ifdef THREADDEBUG
	Sys_PrintfToTerminal("%p barrier wait %s:%d\n", b, filename, fileline);
#endif
	Thread_LockMutex(b->mutex);
	b->called++;
	if (b->called == b->needed) {
		b->called = 0;
		Thread_CondBroadcast(b->cond);
	} else {
		do {
			Thread_CondWait(b->cond, b->mutex);
		} while(b->called);
	}
	Thread_UnlockMutex(b->mutex);
}

#if 0 // Baker: See FTE ?

// NOVAS

/*
  Simple DirectMedia Layer
  Copyright (C) 1997-2018 Sam Lantinga <slouken@libsdl.org>

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/

#if defined(_MSC_VER) && (_MSC_VER >= 1500)
#include <intrin.h>
#define HAVE_MSC_ATOMICS 1
#endif

#if defined(__MACOSX__)  /* !!! FIXME: should we favor gcc atomics? */
#include <libkern/OSAtomic.h>
#endif

#if !defined(HAVE_GCC_ATOMICS) && defined(__SOLARIS__)
#include <atomic.h>
#endif

/* The __atomic_load_n() intrinsic showed up in different times for different compilers. */
#if defined(HAVE_GCC_ATOMICS)
# if defined(__clang__)
#   if __has_builtin(__atomic_load_n)
      /* !!! FIXME: this advertises as available in the NDK but uses an external symbol we don't have.
         It might be in a later NDK or we might need an extra library? --ryan. */
#     if !defined(__ANDROID__)
#       define HAVE_ATOMIC_LOAD_N 1
#     endif
#   endif
# elif defined(__GNUC__)
#   if (__GNUC__ >= 5)
#     define HAVE_ATOMIC_LOAD_N 1
#   endif
# endif
#endif

#if defined(__WATCOMC__) && defined(__386__)
#define HAVE_WATCOM_ATOMICS
extern _inline int _SDL_xchg_watcom(volatile int *a, int v);
#pragma aux _SDL_xchg_watcom = \
  "xchg [ecx], eax" \
  parm [ecx] [eax] \
  value [eax] \
  modify exact [eax];

extern _inline unsigned char _SDL_cmpxchg_watcom(volatile int *a, int newval, int oldval);
#pragma aux _SDL_cmpxchg_watcom = \
  "lock cmpxchg [edx], ecx" \
  "setz al" \
  parm [edx] [ecx] [eax] \
  value [al] \
  modify exact [eax];

extern _inline int _SDL_xadd_watcom(volatile int *a, int v);
#pragma aux _SDL_xadd_watcom = \
  "lock xadd [ecx], eax" \
  parm [ecx] [eax] \
  value [eax] \
  modify exact [eax];
#endif /* __WATCOMC__ && __386__ */

/*
  If any of the operations are not provided then we must emulate some
  of them. That means we need a nice implementation of spin locks
  that avoids the "one big lock" problem. We use a vector of spin
  locks and pick which one to use based on the address of the operand
  of the function.

  To generate the index of the lock we first shift by 3 bits to get
  rid on the zero bits that result from 32 and 64 bit allignment of
  data. We then mask off all but 5 bits and use those 5 bits as an
  index into the table.

  Picking the lock this way insures that accesses to the same data at
  the same time will go to the same lock. OTOH, accesses to different
  data have only a 1/32 chance of hitting the same lock. That should
  pretty much eliminate the chances of several atomic operations on
  different data from waiting on the same "big lock". If it isn't
  then the table of locks can be expanded to a new size so long as
  the new size is a power of two.

  Contributed by Bob Pendleton, bob@pendleton.com
*/

#if 1 // BAKER
//#define SDL_atomic_t void
typedef struct { int value; } SDL_atomic_t;
#define SDL_bool int
#endif

#if !defined(HAVE_MSC_ATOMICS) && !defined(HAVE_GCC_ATOMICS) && !defined(__MACOSX__) && !defined(__SOLARIS__) && !defined(HAVE_WATCOM_ATOMICS)
#define EMULATE_CAS 1
#endif

#if EMULATE_CAS
static SDL_SpinLock locks[32];

static SDL_INLINE void
enterLock(void *a)
{
    uintptr_t index = ((((uintptr_t)a) >> 3) & 0x1f);

    SDL_AtomicLock(&locks[index]);
}

static SDL_INLINE void
leaveLock(void *a)
{
    uintptr_t index = ((((uintptr_t)a) >> 3) & 0x1f);

    SDL_AtomicUnlock(&locks[index]);
}
#endif


SDL_bool
SDL_AtomicCAS(SDL_atomic_t *a, int oldval, int newval)
{
#ifdef HAVE_MSC_ATOMICS
    return (_InterlockedCompareExchange((long*)&a->value, (long)newval, (long)oldval) == (long)oldval);
#elif defined(HAVE_WATCOM_ATOMICS)
    return (SDL_bool) _SDL_cmpxchg_watcom(&a->value, newval, oldval);
#elif defined(HAVE_GCC_ATOMICS)
    return (SDL_bool) __sync_bool_compare_and_swap(&a->value, oldval, newval);
#elif defined(__MACOSX__)  /* this is deprecated in 10.12 sdk; favor gcc atomics. */
    return (SDL_bool) OSAtomicCompareAndSwap32Barrier(oldval, newval, &a->value);
#elif defined(__SOLARIS__) && defined(_LP64)
    return (SDL_bool) ((int) atomic_cas_64((volatile uint64_t*)&a->value, (uint64_t)oldval, (uint64_t)newval) == oldval);
#elif defined(__SOLARIS__) && !defined(_LP64)
    return (SDL_bool) ((int) atomic_cas_32((volatile uint32_t*)&a->value, (uint32_t)oldval, (uint32_t)newval) == oldval);
#elif EMULATE_CAS
    SDL_bool retval = SDL_FALSE;

    enterLock(a);
    if (a->value == oldval) {
        a->value = newval;
        retval = SDL_TRUE;
    }
    leaveLock(a);

    return retval;
#else
    #error Please define your platform.
#endif
}

SDL_bool
SDL_AtomicCASPtr(void **a, void *oldval, void *newval)
{
#if defined(HAVE_MSC_ATOMICS) && (_M_IX86)
    return (_InterlockedCompareExchange((long*)a, (long)newval, (long)oldval) == (long)oldval);
#elif defined(HAVE_MSC_ATOMICS) && (!_M_IX86)
    return (_InterlockedCompareExchangePointer(a, newval, oldval) == oldval);
#elif defined(HAVE_WATCOM_ATOMICS)
    return (SDL_bool) _SDL_cmpxchg_watcom((int *)a, (long)newval, (long)oldval);
#elif defined(HAVE_GCC_ATOMICS)
    return __sync_bool_compare_and_swap(a, oldval, newval);
#elif defined(__MACOSX__) && defined(__LP64__)  /* this is deprecated in 10.12 sdk; favor gcc atomics. */
    return (SDL_bool) OSAtomicCompareAndSwap64Barrier((int64_t)oldval, (int64_t)newval, (int64_t*) a);
#elif defined(__MACOSX__) && !defined(__LP64__)  /* this is deprecated in 10.12 sdk; favor gcc atomics. */
    return (SDL_bool) OSAtomicCompareAndSwap32Barrier((int32_t)oldval, (int32_t)newval, (int32_t*) a);
#elif defined(__SOLARIS__)
    return (SDL_bool) (atomic_cas_ptr(a, oldval, newval) == oldval);
#elif EMULATE_CAS
    SDL_bool retval = SDL_FALSE;

    enterLock(a);
    if (*a == oldval) {
        *a = newval;
        retval = SDL_TRUE;
    }
    leaveLock(a);

    return retval;
#else
    #error Please define your platform.
#endif
}

int
SDL_AtomicSet(SDL_atomic_t *a, int v)
{
#ifdef HAVE_MSC_ATOMICS
    return _InterlockedExchange((long*)&a->value, v);
#elif defined(HAVE_WATCOM_ATOMICS)
    return _SDL_xchg_watcom(&a->value, v);
#elif defined(HAVE_GCC_ATOMICS)
    return __sync_lock_test_and_set(&a->value, v);
#elif defined(__SOLARIS__) && defined(_LP64)
    return (int) atomic_swap_64((volatile uint64_t*)&a->value, (uint64_t)v);
#elif defined(__SOLARIS__) && !defined(_LP64)
    return (int) atomic_swap_32((volatile uint32_t*)&a->value, (uint32_t)v);
#else
    int value;
    do {
        value = a->value;
    } while (!SDL_AtomicCAS(a, value, v));
    return value;
#endif
}

void*
SDL_AtomicSetPtr(void **a, void *v)
{
#if defined(HAVE_MSC_ATOMICS) && (_M_IX86)
    return (void *) _InterlockedExchange((long *)a, (long) v);
#elif defined(HAVE_MSC_ATOMICS) && (!_M_IX86)
    return _InterlockedExchangePointer(a, v);
#elif defined(HAVE_WATCOM_ATOMICS)
    return (void *) _SDL_xchg_watcom((int *)a, (long)v);
#elif defined(HAVE_GCC_ATOMICS)
    return __sync_lock_test_and_set(a, v);
#elif defined(__SOLARIS__)
    return atomic_swap_ptr(a, v);
#else
    void *value;
    do {
        value = *a;
    } while (!SDL_AtomicCASPtr(a, value, v));
    return value;
#endif
}

int
SDL_AtomicAdd(SDL_atomic_t *a, int v)
{
#ifdef HAVE_MSC_ATOMICS
    return _InterlockedExchangeAdd((long*)&a->value, v);
#elif defined(HAVE_WATCOM_ATOMICS)
    return _SDL_xadd_watcom(&a->value, v);
#elif defined(HAVE_GCC_ATOMICS)
    return __sync_fetch_and_add(&a->value, v);
#elif defined(__SOLARIS__)
    int pv = a->value;
    membar_consumer();
#if defined(_LP64)
    atomic_add_64((volatile uint64_t*)&a->value, v);
#elif !defined(_LP64)
    atomic_add_32((volatile uint32_t*)&a->value, v);
#endif
    return pv;
#else
    int value;
    do {
        value = a->value;
    } while (!SDL_AtomicCAS(a, value, (value + v)));
    return value;
#endif
}

int
SDL_AtomicGet(SDL_atomic_t *a)
{
#ifdef HAVE_ATOMIC_LOAD_N
    return __atomic_load_n(&a->value, __ATOMIC_SEQ_CST);
#else
    int value;
    do {
        value = a->value;
    } while (!SDL_AtomicCAS(a, value, value));
    return value;
#endif
}

void *
SDL_AtomicGetPtr(void **a)
{
#ifdef HAVE_ATOMIC_LOAD_N
    return __atomic_load_n(a, __ATOMIC_SEQ_CST);
#else
    void *value;
    do {
        value = *a;
    } while (!SDL_AtomicCASPtr(a, value, value));
    return value;
#endif
}

//void
//SDL_MemoryBarrierReleaseFunction(void)
//{
//    SDL_MemoryBarrierRelease();
//}
//
//void
//SDL_MemoryBarrierAcquireFunction(void)
//{
//    SDL_MemoryBarrierAcquire();
//}
//
///* vi: set ts=4 sw=4 expandtab: */

int _Thread_AtomicGet(Thread_Atomic *a, const char *filename, int fileline)
{
#ifdef THREADDEBUG
	Sys_PrintfToTerminal("%p atomic get at %s:%d\n", a, filename, fileline);
#endif
//	return SDL_AtomicGet((SDL_atomic_t *)a);
    int value;
    do {
        value = a->value;
    } while (!SDL_AtomicCAS(a, value, value));
    return value;

	return __atomic_load_n(&a->value, __ATOMIC_SEQ_CST);
}

int _Thread_AtomicSet(Thread_Atomic *a, int v, const char *filename, int fileline)
{
#ifdef THREADDEBUG
	Sys_PrintfToTerminal("%p atomic set %v at %s:%d\n", a, v, filename, fileline);
#endif
	return SDL_AtomicSet((SDL_atomic_t *)a, v);
}

int _Thread_AtomicAdd(Thread_Atomic *a, int v, const char *filename, int fileline)
{
#ifdef THREADDEBUG
	Sys_PrintfToTerminal("%p atomic add %v at %s:%d\n", a, v, filename, fileline);
#endif
	return SDL_AtomicAdd((SDL_atomic_t *)a, v);
}

void _Thread_AtomicIncRef(Thread_Atomic *a, const char *filename, int fileline)
{
#ifdef THREADDEBUG
	Sys_PrintfToTerminal("%p atomic incref %s:%d\n", a, filename, fileline);
#endif
	SDL_AtomicIncRef((SDL_atomic_t *)a);
}

qbool _Thread_AtomicDecRef(Thread_Atomic *a, const char *filename, int fileline)
{
#ifdef THREADDEBUG
	Sys_PrintfToTerminal("%p atomic decref %s:%d\n", a, filename, fileline);
#endif
	return SDL_AtomicDecRef((SDL_atomic_t *)a) != SDL_FALSE;
}

qbool _Thread_AtomicTryLock(Thread_SpinLock *lock, const char *filename, int fileline)
{
#ifdef THREADDEBUG
	Sys_PrintfToTerminal("%p atomic try lock %s:%d\n", lock, filename, fileline);
#endif
	return SDL_AtomicTryLock(lock) != SDL_FALSE;
}

void _Thread_AtomicLock(Thread_SpinLock *lock, const char *filename, int fileline)
{
#ifdef THREADDEBUG
	Sys_PrintfToTerminal("%p atomic lock %s:%d\n", lock, filename, fileline);
#endif
	SDL_AtomicLock(lock);
}

void _Thread_AtomicUnlock(Thread_SpinLock *lock, const char *filename, int fileline)
{
#ifdef THREADDEBUG
	Sys_PrintfToTerminal("%p atomic unlock %s:%d\n", lock, filename, fileline);
#endif
	SDL_AtomicUnlock(lock);
}
#endif // 0

#endif // CORE_SDL