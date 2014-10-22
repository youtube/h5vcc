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
 * When the abstime parameter is invalid, 
 * the function must return EINVAL and 
 * the mutex state must not have changed during the call.

 * The steps are:
 *  -> parent (for each mutex type and each condvar options, across threads or processes)
 *     -> locks the mutex m
 *     -> sets ctrl = 0
 *     -> creates a bunch of children, which:
 *        -> lock the mutex m
 *        -> if ctrl == 0, test has failed
 *        -> unlock the mutex then exit
 *     -> calls pthread_cond_timedwait with invalid values (nsec > 999999999)
 *     -> sets ctrl = non-zero value
 *     -> unlocks the mutex m
 */
 
 /* We are testing conformance to IEEE Std 1003.1, 2003 Edition */
 #define _POSIX_C_SOURCE 200112L
 
 /* We need the XSI extention for the mutex attributes
   and the mkstemp() routine */
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
 #include <sys/mman.h>
 #include <string.h>
 #include <time.h>
 
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

/********************************************************************************************/
/********************************** Configuration ******************************************/
/********************************************************************************************/
#ifndef VERBOSE
#define VERBOSE 1
#endif

#define NCHILDREN (20)

#ifndef WITHOUT_ALTCLK
#define USE_ALTCLK  /* make tests with MONOTONIC CLOCK if supported */
#endif

/********************************************************************************************/
/***********************************    Test case   *****************************************/
/********************************************************************************************/
#ifndef WITHOUT_XOPEN

typedef struct 
{
	pthread_mutex_t mtx;
	int ctrl;   /* Control value */
	int gotit;  /* Thread locked the mutex while ctrl == 0 */
	int status; /* error code */
} testdata_t;

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

struct {
	long sec_val;          /* Value for seconds */
	short sec_is_offset;   /* Seconds value is added to current time or is absolute */
	long nsec_val;         /* Value for nanoseconds */
	short nsec_is_offset;  /* Nanoseconds value is added to current time or is absolute */
}
junks_ts[]={
	 {          -2 , 1,  1000000000, 1 }
	,{          -2 , 1,          -1, 0 }
	,{          -3 , 1,  2000000000, 0 }
};


void * tf(void * arg)
{
	int ret=0;
	
	testdata_t * td = (testdata_t *)arg;
	
	/* Lock the mutex */
	ret = pthread_mutex_lock(&(td->mtx));
	if (ret != 0)
	{
		td->status = ret;
		UNRESOLVED(ret, "[child] Unable to lock the mutex");
	}
	
	/* Checks whether the parent release the lock inside the timedwait function */
	if (td->ctrl == 0)
		td->gotit += 1;
	
	/* Unlock and exit */
	ret = pthread_mutex_unlock(&(td->mtx));
	if (ret != 0)
	{
		td->status=ret;
		UNRESOLVED(ret, "[child] Failed to unlock the mutex.");
	}
	return NULL;
}


int main(int argc, char * argv[])
{
	int ret, i, j, k;
	pthread_mutexattr_t ma;
	pthread_condattr_t ca;
	pthread_cond_t cnd;
	struct timespec ts, ts_junk;
	
	testdata_t * td;
	testdata_t alternativ;
	
	pid_t child_pr[NCHILDREN], chkpid;
	int status;
	pthread_t child_th[NCHILDREN];
	
	output_init();
	
/**********
 * Allocate space for the testdata structure
 */
	/* Cannot mmap a file, we use an alternative method */
	td = &alternativ;
	#if VERBOSE > 0
	output("Testdata allocated in the process memory.\n");
	#endif
	
/**********
 * For each test scenario, initialize the attributes and other variables.
 * Do the whole thing for each time to test.
 */
	for ( i=0; i< (sizeof(scenarii) / sizeof(scenarii[0])); i++)
	{
		for ( j=0; j< (sizeof(junks_ts) / sizeof(junks_ts[0])); j++)
		{
			#if VERBOSE > 1
			output("[parent] Preparing attributes for: %s\n", scenarii[i].descr);
			#endif
			/* set / reset everything */
			ret = pthread_mutexattr_init(&ma);
			if (ret != 0)  {  UNRESOLVED(ret, "[parent] Unable to initialize the mutex attribute object");  }
			ret = pthread_condattr_init(&ca);
			if (ret != 0)  {  UNRESOLVED(ret, "[parent] Unable to initialize the cond attribute object");  }
			
			/* Set the mutex type */
			ret = pthread_mutexattr_settype(&ma, scenarii[i].m_type);
			if (ret != 0)  {  UNRESOLVED(ret, "[parent] Unable to set mutex type");  }
			#if VERBOSE > 1
			output("[parent] Mutex type : %i\n", scenarii[i].m_type);
			#endif
			
			/* initialize the condvar */
			ret = pthread_cond_init(&cnd, &ca);
			if (ret != 0)
			{  UNRESOLVED(ret, "[parent] Cond init failed");  }
			
			
/**********
 * Initialize the testdata_t structure with the previously defined attributes 
 */
			/* Initialize the mutex */
			ret = pthread_mutex_init(&(td->mtx), &ma);
			if (ret != 0)
			{  UNRESOLVED(ret, "[parent] Mutex init failed");  }
			
			/* Initialize the other datas from the test structure */
			td->ctrl=0;
			td->gotit=0;
			td->status=0;
			
/**********
 * Proceed to the actual testing 
 */
			/* Lock the mutex before creating children */
			ret = pthread_mutex_lock(&(td->mtx));
			if (ret != 0)  {  UNRESOLVED(ret, "[parent] Unable to lock the mutex");  }
			
			/* Create the children */
			/* We are testing across two threads */
			for (k=0; k<NCHILDREN; k++)
			{
				ret = pthread_create(&child_th[k], NULL, tf, td);
				if (ret != 0)  {  UNRESOLVED(ret, "[parent] Unable to create the child thread.");  }
			}
			
			/* Children are now running and trying to lock the mutex.*/
			struct timeval now;
			ret = gettimeofday(&now, NULL);
			if (ret != 0)  {  UNRESOLVED(errno, "Unable to read clock");  }
			ts.tv_sec = now.tv_sec;
			ts.tv_nsec = now.tv_usec * 1000;
			
			/* Do the junk timedwaits */
			ts_junk.tv_sec = junks_ts[j].sec_val + (junks_ts[j].sec_is_offset?ts.tv_sec:0) ;
			ts_junk.tv_nsec = junks_ts[j].nsec_val + (junks_ts[j].nsec_is_offset?ts.tv_nsec:0) ;
			
			#if VERBOSE > 2
			output("TS: s = %s%li ; ns = %s%li\n", 
				junks_ts[j].sec_is_offset?"n + ":" ", 
				junks_ts[j].sec_val, 
				junks_ts[j].nsec_is_offset?"n + ":" ", 
				junks_ts[j].nsec_val);
			output("Now is: %i.%09li\n", ts.tv_sec, ts.tv_nsec);	
			output("Junk is: %i.%09li\n", ts_junk.tv_sec, ts_junk.tv_nsec);
			#endif
			
			do {
				ret = pthread_cond_timedwait(&cnd, &(td->mtx), &ts_junk);
			} while (ret == 0);
			#if VERBOSE > 2
			output("timedwait returns %d (%s) - gotit = %d\n", ret, strerror(ret), td->gotit);
			#endif
			
			/* check that when EINVAL is returned, the mutex has not been released */
			if (ret == EINVAL)
			{
				if (td->gotit != 0)
				{
					FAILED("The mutex was released when an invalid timestamp was detected in the function");
				}
			#if VERBOSE > 0
			} else {
				output("Warning, struct timespec with tv_sec = %i and tv_nsec = %li was not invalid\n", ts_junk.tv_sec, ts_junk.tv_nsec);
			#endif
			}
			
			/* Finally unlock the mutex */
			td->ctrl = 1;
			ret = pthread_mutex_unlock(&(td->mtx));
			if (ret != 0)  {  UNRESOLVED(ret, "[parent] Unable to unlock the mutex");  }
			
			/* Wait for the child to terminate */
			for (k=0; k<NCHILDREN; k++)
			{
				ret = pthread_join(child_th[k], NULL);
				if (ret != 0)  {  UNRESOLVED(ret, "[parent] Unable to join the thread");  }
			}
	
/**********
 * Destroy the data 
 */
			ret = pthread_cond_destroy(&cnd);
			if (ret != 0)  {  UNRESOLVED(ret, "Failed to destroy the cond var");  }
			
			ret = pthread_mutex_destroy(&(td->mtx));
			if (ret != 0)  {  UNRESOLVED(ret, "Failed to destroy the mutex");  }
			
			ret = pthread_condattr_destroy(&ca);
			if (ret != 0)  {  UNRESOLVED(ret, "Failed to destroy the cond var attribute object");  }
			
			ret = pthread_mutexattr_destroy(&ma);
			if (ret != 0)  {  UNRESOLVED(ret, "Failed to destroy the mutex attribute object");  }
			
		} /* Proceed to the next junk timedwait value */
	}  /* Proceed to the next scenario */
	
	#if VERBOSE > 0
	output("Test passed\n");
	#endif

	PASSED;
	return 0;
}

#else /* WITHOUT_XOPEN */
int main(int argc, char * argv[])
{
	output_init();
	UNTESTED("This test requires XSI features");
	return 0;
}
#endif


