/*
 * Copyright (c) 2004, Bull S.A..  All rights reserved.
 * Created by: Sebastien Decugis

 * This program is free software; you can redistribute it and/or modify it
 * under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it would be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write the Free Software Foundation, Inc., 59
 * Temple Place - Suite 330, Boston MA 02111-1307, USA.

 
 * This sample test aims to check the following assertion:
 *
 * It is safe to destroy a condition variable when no thread is blocked on it.
 
 * The steps are:
 *  -> Some threads are waiting on a condition variable.
 *  -> A thread broadcasts and destroys immediatly the condvar, 
 *     then corrupts the memory of the condvar.
 *
 * The test fails if it hangs or if an error is returned, either
 * in the wait routines or in the destroy routine.
 *
 */
 
 
 /* We are testing conformance to IEEE Std 1003.1, 2003 Edition */
 #define _POSIX_C_SOURCE 200112L
 
 /* We need the XSI extention for the mutex attributes */
#ifndef WITHOUT_XOPEN
 #define _XOPEN_SOURCE	600
#endif
/********************************************************************************************/
/****************************** standard includes *****************************************/
/********************************************************************************************/
 #include <pthread.h>
 #include <stdarg.h>
 #include <stdio.h>
 #include <stdlib.h> 
 #include <unistd.h>

 #include <errno.h>
 #include <signal.h>
 #include <string.h>
 #include <time.h>
 #include <sys/mman.h>
 
/********************************************************************************************/
/******************************   Test framework   *****************************************/
/********************************************************************************************/
 #include "testfrmw.h"
 #include "testfrmw.c"
 /* This header is responsible for defining the following macros:
  * UNRESOLVED(ret, descr);  
  *    where descr is a description of the error and ret is an int (error code for example)
  * FAILED(descr);
  *    where descr is a short text saying why the test has failed.
  * PASSED();
  *    No parameter.
  * 
  * Both three macros shall terminate the calling process.
  * The testcase shall not terminate in any other maneer.
  * 
  * The other file defines the functions
  * void output_init()
  * void output(char * string, ...)
  * 
  * Those may be used to output information.
  */
#define UNRESOLVED_KILLALL(error, text, Tchild) { \
	UNRESOLVED(error, text); \
	}
#define FAILED_KILLALL(text, Tchild) { \
	FAILED(text); \
	}
/********************************************************************************************/
/********************************** Configuration ******************************************/
/********************************************************************************************/
#ifndef VERBOSE
#define VERBOSE 1
#endif

#define NTHREADS (5)

#define TIMEOUT  (120)

#ifndef WITHOUT_ALTCLK
#define USE_ALTCLK  /* test with MONOTONIC CLOCK if supported */
#endif

/********************************************************************************************/
/***********************************    Test case   *****************************************/
/********************************************************************************************/

#ifdef WITHOUT_XOPEN
/* We define those to avoid compilation errors, but they won't be used */
#define PTHREAD_MUTEX_DEFAULT 0
#define PTHREAD_MUTEX_NORMAL 0
#define PTHREAD_MUTEX_ERRORCHECK 0
#define PTHREAD_MUTEX_RECURSIVE 0

#endif


struct _scenar
{
	int m_type; /* Mutex type to use */
	int mc_pshared; /* 0: mutex and cond are process-private (default) ~ !0: Both are process-shared, if supported */
	int c_clock; /* 0: cond uses the default clock. ~ !0: Cond uses monotonic clock, if supported. */
	int fork; /* 0: Test between threads. ~ !0: Test across processes, if supported (mmap) */
	const char * descr; /* Case description */
}
scenarii[] =
{
	 {PTHREAD_MUTEX_DEFAULT,    0, 0, 0, "Default mutex"}
	,{PTHREAD_MUTEX_NORMAL,     0, 0, 0, "Normal mutex"}
	,{PTHREAD_MUTEX_ERRORCHECK, 0, 0, 0, "Errorcheck mutex"}
	,{PTHREAD_MUTEX_RECURSIVE,  0, 0, 0, "Recursive mutex"}

	,{PTHREAD_MUTEX_DEFAULT,    1, 0, 0, "PShared default mutex"}
	,{PTHREAD_MUTEX_NORMAL,     1, 0, 0, "Pshared normal mutex"}
	,{PTHREAD_MUTEX_ERRORCHECK, 1, 0, 0, "Pshared errorcheck mutex"}
	,{PTHREAD_MUTEX_RECURSIVE,  1, 0, 0, "Pshared recursive mutex"}

	,{PTHREAD_MUTEX_DEFAULT,    1, 0, 1, "Pshared default mutex across processes"}
	,{PTHREAD_MUTEX_NORMAL,     1, 0, 1, "Pshared normal mutex across processes"}
	,{PTHREAD_MUTEX_ERRORCHECK, 1, 0, 1, "Pshared errorcheck mutex across processes"}
	,{PTHREAD_MUTEX_RECURSIVE,  1, 0, 1, "Pshared recursive mutex across processes"}

#ifdef USE_ALTCLK
	,{PTHREAD_MUTEX_DEFAULT,    1, 1, 1, "Pshared default mutex and alt clock condvar across processes"}
	,{PTHREAD_MUTEX_NORMAL,     1, 1, 1, "Pshared normal mutex and alt clock condvar across processes"}
	,{PTHREAD_MUTEX_ERRORCHECK, 1, 1, 1, "Pshared errorcheck mutex and alt clock condvar across processes"}
	,{PTHREAD_MUTEX_RECURSIVE,  1, 1, 1, "Pshared recursive mutex and alt clock condvar across processes"}

	,{PTHREAD_MUTEX_DEFAULT,    0, 1, 0, "Default mutex and alt clock condvar"}
	,{PTHREAD_MUTEX_NORMAL,     0, 1, 0, "Normal mutex and alt clock condvar"}
	,{PTHREAD_MUTEX_ERRORCHECK, 0, 1, 0, "Errorcheck mutex and alt clock condvar"}
	,{PTHREAD_MUTEX_RECURSIVE,  0, 1, 0, "Recursive mutex and alt clock condvar"}

	,{PTHREAD_MUTEX_DEFAULT,    1, 1, 0, "PShared default mutex and alt clock condvar"}
	,{PTHREAD_MUTEX_NORMAL,     1, 1, 0, "Pshared normal mutex and alt clock condvar"}
	,{PTHREAD_MUTEX_ERRORCHECK, 1, 1, 0, "Pshared errorcheck mutex and alt clock condvar"}
	,{PTHREAD_MUTEX_RECURSIVE,  1, 1, 0, "Pshared recursive mutex and alt clock condvar"}
#endif
};
#define NSCENAR (sizeof(scenarii)/sizeof(scenarii[0]))

/* The shared data */
typedef struct 
{
	int 		count1;     /* number of children currently waiting (1st pass)*/
	int 		count2;     /* number of children currently waiting (2nd pass)*/
	pthread_cond_t 	cnd;
	pthread_mutex_t mtx1;
	pthread_mutex_t mtx2;
	int 		predicate1, predicate2; /* Boolean associated to the condvar */
} testdata_t;
testdata_t * td;


/* Child function (either in a thread or in a process) */
void * child(void * arg)
{
	int ret=0;
	struct timespec ts;
	char timed;
	
	/* lock the 1st mutex */
	ret = pthread_mutex_lock(&td->mtx1);
	if (ret != 0)  {  UNRESOLVED(ret, "Failed to lock mutex in child");  }
	
	/* increment count */
	td->count1++;
	
	timed=td->count1 & 1;
	
	if (timed)
	{
	/* get current time if we are a timedwait */
		struct timeval now;
		ret = gettimeofday(&now, NULL);
		if (ret != 0)  {  UNRESOLVED(errno, "Unable to read clock");  }
		ts.tv_sec = now.tv_sec + TIMEOUT;
		ts.tv_nsec = now.tv_usec * 1000;
	}
	
	do {
	/* Wait while the predicate is false */
		if (timed)
			ret = pthread_cond_timedwait(&td->cnd, &td->mtx1, &ts);
		else
			ret = pthread_cond_wait(&td->cnd, &td->mtx1);
	} while ((ret == 0) && (td->predicate1==0));
	if ((ret != 0) && (td->predicate1 != 0))
	{
		output("Wakening the cond failed with error %i (%s)\n", ret, strerror(ret));
		FAILED("Destroying the cond var while threads were awaken but inside wait routine failed.");
	}
	if (ret != 0)  {  UNRESOLVED(ret, "Failed to wait for the cond");  }
	
	td->count1--;
	
	/* unlock the mutex */
	ret = pthread_mutex_unlock(&td->mtx1);
	if (ret != 0)  {  UNRESOLVED(ret, "Failed to unlock the mutex.");  }
	
  /* Second pass */
	
	/* lock the mutex */
	ret = pthread_mutex_lock(&td->mtx2);
	if (ret != 0)  {  UNRESOLVED(ret, "Failed to lock mutex in child");  }
	
	/* increment count */
	td->count2++;
	
	timed=td->count2 & 1;
	
	if (timed)
	{
	/* get current time if we are a timedwait */
		struct timeval now;
		ret = gettimeofday(&now, NULL);
		if (ret != 0)  {  UNRESOLVED(errno, "Unable to read clock");  }
		ts.tv_sec = now.tv_sec + TIMEOUT;
		ts.tv_nsec = now.tv_usec * 1000;
	}
	
	do {
	/* Wait while the predicate is false */
		if (timed)
			ret = pthread_cond_timedwait(&td->cnd, &td->mtx2, &ts);
		else
			ret = pthread_cond_wait(&td->cnd, &td->mtx2);
	} while ((ret == 0) && (td->predicate2==0));
	if ((ret != 0) && (td->predicate2 != 0))
	{
		output("Wakening the cond failed with error %i (%s)\n", ret, strerror(ret));
		FAILED("Destroying the cond var while threads were awaken but inside wait routine failed.");
	}
	if (ret != 0)  {  UNRESOLVED(ret, "Failed to wait for the cond");  }
	
	/* unlock the mutex */
	ret = pthread_mutex_unlock(&td->mtx2);
	if (ret != 0)  {  UNRESOLVED(ret, "Failed to unlock the mutex.");  }
	
	
	return NULL;
}

/* main function */

int main (int argc, char * argv[])
{
	int ret;
	
	pthread_mutexattr_t ma;
	pthread_condattr_t ca;
	
	int scenar;
	
	pid_t p_child[NTHREADS];
	pthread_t t_child[NTHREADS];
	int ch;
	pid_t pid;
	int status;
	
	testdata_t alternativ;
	
	output_init();
	
/**********
 * Allocate space for the testdata structure
 */
	/* Cannot mmap a file, we use an alternative method */
	td = &alternativ;
	#if VERBOSE > 0
	output("Testdata allocated in the process memory.\n");
	#endif
	
	/* Do the test for each test scenario */
	for (scenar=0; scenar < NSCENAR; scenar++)
	{
		/* set / reset everything */
		ret = pthread_mutexattr_init(&ma);
		if (ret != 0)  {  UNRESOLVED(ret, "[parent] Unable to initialize the mutex attribute object");  }
		ret = pthread_condattr_init(&ca);
		if (ret != 0)  {  UNRESOLVED(ret, "[parent] Unable to initialize the cond attribute object");  }
		
		#ifndef WITHOUT_XOPEN
		/* Set the mutex type */
		ret = pthread_mutexattr_settype(&ma, scenarii[scenar].m_type);
		if (ret != 0)  {  UNRESOLVED(ret, "[parent] Unable to set mutex type");  }
		#endif
		
	/* Proceed to testing */
		/* initialize the mutex */
		ret = pthread_mutex_init(&td->mtx1, &ma);
		if (ret != 0)  {  UNRESOLVED(ret, "Mutex init failed");  }
		
		ret = pthread_mutex_init(&td->mtx2, &ma);
		if (ret != 0)  {  UNRESOLVED(ret, "Mutex init failed");  }
		
		ret = pthread_mutex_lock(&td->mtx2);
		if (ret != 0)  {  UNRESOLVED(ret, "Mutex lock failed");  }
		
		/* initialize the condvar */
		ret = pthread_cond_init(&td->cnd, &ca);
		if (ret != 0)  {  UNRESOLVED(ret, "Cond init failed");  }
		
		#if VERBOSE > 2
		output("[parent] Starting 1st pass of test %s\n", scenarii[scenar].descr);
		#endif
		
		td->count1=0;
		td->count2=0;
		td->predicate1=0;
		td->predicate2=0;
		
		/* Create all the children */
		for (ch=0; ch < NTHREADS; ch++)
		{
			ret = pthread_create(&t_child[ch], NULL, child, NULL);
			if (ret != 0)  {  UNRESOLVED(ret, "Failed to create a child thread");  }
		}
		#if VERBOSE > 4
		output("[parent] All children are running\n");
		#endif
		
		/* Make sure all children are waiting */
		ret = pthread_mutex_lock(&td->mtx1);
		if (ret != 0) {  UNRESOLVED_KILLALL(ret, "Failed to lock mutex", p_child);  }
		ch = td->count1;
		while (ch < NTHREADS)
		{
			ret = pthread_mutex_unlock(&td->mtx1);
			if (ret != 0)  {  UNRESOLVED_KILLALL(ret, "Failed to unlock mutex",p_child);  }
			sched_yield();
			ret = pthread_mutex_lock(&td->mtx1);
			if (ret != 0) {  UNRESOLVED_KILLALL(ret, "Failed to lock mutex",p_child);  }
			ch = td->count1;
		}
		
		#if VERBOSE > 4
		output("[parent] All children are waiting\n");
		#endif
		
		/* Wakeup the children */
		td->predicate1=1;
		ret = pthread_cond_broadcast(&td->cnd);
		if (ret != 0)  {  UNRESOLVED_KILLALL(ret, "Failed to signal the condition.", p_child);  }
		
		ret = pthread_mutex_unlock(&td->mtx1);
		if (ret != 0)  {  UNRESOLVED_KILLALL(ret, "Failed to unlock mutex",p_child);  }

		/* Destroy the condvar (this must be safe) */
		do {
			ret = pthread_cond_destroy(&td->cnd);
			usleep(10 * 1000);
		} while (ret == EBUSY);

		if (ret != 0)  {  FAILED_KILLALL("Unable to destroy the cond while no thread is blocked inside", p_child);  }
		
		/* Reuse the cond memory */
		memset(&td->cnd, 0xFF, sizeof(pthread_cond_t));
		
		#if VERBOSE > 4
		output("[parent] Condition was broadcasted, and condvar destroyed.\n");
		#endif
		
		/* Make sure all children have exited the first wait */
		ret = pthread_mutex_lock(&td->mtx1);
		if (ret != 0) {  UNRESOLVED_KILLALL(ret, "Failed to lock mutex",p_child);  }
		ch = td->count1;
		while (ch > 0)
		{
			ret = pthread_mutex_unlock(&td->mtx1);
			if (ret != 0)  {  UNRESOLVED_KILLALL(ret, "Failed to unlock mutex",p_child);  }
			sched_yield();
			ret = pthread_mutex_lock(&td->mtx1);
			if (ret != 0) {  UNRESOLVED_KILLALL(ret, "Failed to lock mutex",p_child);  }
			ch = td->count1;
		}

		ret = pthread_mutex_unlock(&td->mtx1);
		if (ret != 0)  {  UNRESOLVED_KILLALL(ret, "Failed to unlock mutex",p_child);  }
		
	/* Go toward the 2nd pass */
		/* Now, all children are waiting to lock the 2nd mutex, which we own here. */
		/* reinitialize the condvar */
		ret = pthread_cond_init(&td->cnd, &ca);
		if (ret != 0)  {  UNRESOLVED(ret, "Cond init failed");  }
		
		#if VERBOSE > 2
		output("[parent] Starting 2nd pass of test %s\n", scenarii[scenar].descr);
		#endif
		
		/* Make sure all children are waiting */
		ch = td->count2;
		while (ch < NTHREADS)
		{
			ret = pthread_mutex_unlock(&td->mtx2);
			if (ret != 0)  {  UNRESOLVED_KILLALL(ret, "Failed to unlock mutex",p_child);  }
			sched_yield();
			ret = pthread_mutex_lock(&td->mtx2);
			if (ret != 0) {  UNRESOLVED_KILLALL(ret, "Failed to lock mutex",p_child);  }
			ch = td->count2;
		}
		
		#if VERBOSE > 4
		output("[parent] All children are waiting\n");
		#endif
		
		/* Wakeup the children */
		td->predicate2=1;
		ret = pthread_cond_broadcast(&td->cnd);
		if (ret != 0)  {  UNRESOLVED_KILLALL(ret, "Failed to signal the condition.", p_child);  }
		
		/* Allow the children to terminate */
		ret = pthread_mutex_unlock(&td->mtx2);
		if (ret != 0)  {  UNRESOLVED_KILLALL(ret, "Failed to unlock mutex",p_child);  }

		/* Destroy the condvar (this must be safe) */
		ret = pthread_cond_destroy(&td->cnd);
		if (ret != 0)  {  FAILED_KILLALL("Unable to destroy the cond while no thread is blocked inside", p_child);  }
		
		/* Reuse the cond memory */
		memset(&td->cnd, 0x00, sizeof(pthread_cond_t));
		
		#if VERBOSE > 4
		output("[parent] Condition was broadcasted, and condvar destroyed.\n");
		#endif
		
		#if VERBOSE > 4
		output("[parent] Joining the children\n");
		#endif

		/* join the children */
		for (ch=(NTHREADS - 1); ch >= 0 ; ch--)
		{
			ret = pthread_join(t_child[ch], NULL);
			if (ret != 0)  {  UNRESOLVED(ret, "Failed to join a child thread");  }
		}
		if (ret != 0)
		{
			output_fini();
			exit(ret);
		}
		#if VERBOSE > 4
		output("[parent] All children terminated\n");
		#endif
		
		/* Destroy the datas */
#if 0
// This seems wrong, as it was just destroyed and zeroed out above.
		ret = pthread_cond_destroy(&td->cnd);
		if (ret != 0)  {  UNRESOLVED(ret, "Failed to destroy the condvar");  }
#endif
		
		ret = pthread_mutex_destroy(&td->mtx1);
		if (ret != 0)  {  UNRESOLVED(ret, "Failed to destroy the mutex");  }

		ret = pthread_mutex_destroy(&td->mtx2);
		if (ret != 0)  {  UNRESOLVED(ret, "Failed to destroy the mutex");  }

		/* Destroy the attributes */
		ret = pthread_condattr_destroy(&ca);
		if (ret != 0)  {  UNRESOLVED(ret, "Failed to destroy the cond var attribute object");  }
		
		ret = pthread_mutexattr_destroy(&ma);
		if (ret != 0)  {  UNRESOLVED(ret, "Failed to destroy the mutex attribute object");  }
		
		
	}
	
	/* exit */
	PASSED;
	return 0;
}

