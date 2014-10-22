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
 * pthread_cond_signal() unblocks at least one thread which is blocked
 * on the conditional variable, if any.
 
 * The steps are:
 *  -> Create a lot of threads/process which will wait on a condition variable
 *  -> Cascade-signal the condition and check that all threads/processes are awaken.
 *
 *  The test will fail when the threads are not terminated within a certain duration.
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
/********************************************************************************************/
/********************************** Configuration ******************************************/
/********************************************************************************************/
#ifndef VERBOSE
#define VERBOSE 1
#endif

#define NTHREADS (20)

#define TIMEOUT  (120)

#ifndef WITHOUT_ALTCLK
#define USE_ALTCLK  /* make tests with MONOTONIC CLOCK if supported */
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
	int 		count;     /* number of children currently waiting */
	pthread_cond_t 	cnd;
	pthread_mutex_t mtx;
	int 		predicate; /* Boolean associated to the condvar */
} testdata_t;
testdata_t * td;


/* Child function (either in a thread or in a process) */
void * child(void * arg)
{
	int ret=0;
	struct timespec ts;
	char timed;
	
	/* lock the mutex */
	ret = pthread_mutex_lock(&td->mtx);
	if (ret != 0)  {  UNRESOLVED(ret, "Failed to lock mutex in child");  }
	
	/* increment count */
	td->count++;
	
	timed=td->count & 1;
	
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
			ret = pthread_cond_timedwait(&td->cnd, &td->mtx, &ts);
		else
			ret = pthread_cond_wait(&td->cnd, &td->mtx);
		#if VERBOSE > 5
		output("[child] Wokenup timed=%i, Predicate=%i, ret=%i\n", timed, td->predicate, ret);
		#endif
	} while ((ret == 0) && (td->predicate==0));
	if (ret == ETIMEDOUT)
	{
		FAILED("Timeout occured. This means a cond signal was lost -- or parent died");
	}
	if (ret != 0)  {  UNRESOLVED(ret, "Failed to wait for the cond");  }
	
	/* Signal the condition to cascade */
	ret = pthread_cond_signal(&td->cnd);
	if (ret != 0)  {  UNRESOLVED(ret, "Failed to cascade signal the cond");  }
	
	/* unlock the mutex */
	ret = pthread_mutex_unlock(&td->mtx);
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
		
		/* initialize the condvar */
		ret = pthread_cond_init(&td->cnd, &ca);
		if (ret != 0)  {  UNRESOLVED(ret, "Cond init failed");  }
		
		/* initialize the mutex */
		ret = pthread_mutex_init(&td->mtx, &ma);
		if (ret != 0)  {  UNRESOLVED(ret, "Mutex init failed");  }
		
		/* Destroy the attributes */
		ret = pthread_condattr_destroy(&ca);
		if (ret != 0)  {  UNRESOLVED(ret, "Failed to destroy the cond var attribute object");  }
		
		ret = pthread_mutexattr_destroy(&ma);
		if (ret != 0)  {  UNRESOLVED(ret, "Failed to destroy the mutex attribute object");  }
		
		#if VERBOSE > 2
		output("[parent] Starting test %s\n", scenarii[scenar].descr);
		#endif
		
		td->count=0;
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
		ret = pthread_mutex_lock(&td->mtx);
		if (ret != 0) {  UNRESOLVED_KILLALL(ret, "Failed to lock mutex", p_child);  }
		ch = td->count;
		while (ch < NTHREADS)
		{
			ret = pthread_mutex_unlock(&td->mtx);
			if (ret != 0)  {  UNRESOLVED_KILLALL(ret, "Failed to unlock mutex",p_child);  }
			sched_yield();
			ret = pthread_mutex_lock(&td->mtx);
			if (ret != 0) {  UNRESOLVED_KILLALL(ret, "Failed to lock mutex",p_child);  }
			ch = td->count;
		}
		
		#if VERBOSE > 4
		output("[parent] All children are waiting\n");
		#endif
		
		/* Wakeup the children */
		td->predicate=1;
		ret = pthread_cond_signal(&td->cnd);
		if (ret != 0)  {  UNRESOLVED_KILLALL(ret, "Failed to signal the condition.", p_child);  }

		#if VERBOSE > 4
		output("[parent] Condition was signaled\n");
		#endif
		
		ret = pthread_mutex_unlock(&td->mtx);
		if (ret != 0)  {  UNRESOLVED_KILLALL(ret, "Failed to unlock mutex",p_child);  }
		
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
		ret = pthread_cond_destroy(&td->cnd);
		if (ret != 0)  {  UNRESOLVED(ret, "Failed to destroy the condvar");  }
		
		ret = pthread_mutex_destroy(&td->mtx);
		if (ret != 0)  {  UNRESOLVED(ret, "Failed to destroy the mutex");  }
		
	}
	
	/* exit */
	PASSED;
	return 0;
}

