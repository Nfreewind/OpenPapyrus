/*
 * pthread_rwlockattr_setpshared.c
 *
 * Description:
 * This translation unit implements read/write lock primitives.
 *
 * --------------------------------------------------------------------------
 *
 *      Pthreads4w - POSIX Threads for Windows
 *      Copyright 1998 John E. Bossom
 *      Copyright 1999-2018, Pthreads4w contributors
 *
 *      Homepage: https://sourceforge.net/projects/pthreads4w/
 *
 *      The current list of contributors is contained
 *      in the file CONTRIBUTORS included with the source
 *      code distribution. The list can also be seen at the
 *      following World Wide Web location:
 *
 *      https://sourceforge.net/p/pthreads4w/wiki/Contributors/
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <sl_pthreads4w.h>
#pragma hdrstop
/*
 * ------------------------------------------------------
 * DOCPUBLIC
 *      Rwlocks created with 'attr' can be shared between
 *      processes if pthread_rwlock_t variable is allocated
 *      in memory shared by these processes.
 *
 * PARAMETERS
 *      attr
 *              pointer to an instance of pthread_rwlockattr_t
 *
 *      pshared
 *              must be one of:
 *
 *                      PTHREAD_PROCESS_SHARED
 *                              May be shared if in shared memory
 *
 *                      PTHREAD_PROCESS_PRIVATE
 *                              Cannot be shared.
 *
 * DESCRIPTION
 *      Rwlocks creatd with 'attr' can be shared between
 *      processes if pthread_rwlock_t variable is allocated
 *      in memory shared by these processes.
 *
 *      NOTES:
 *              1)      pshared rwlocks MUST be allocated in shared
 *                      memory.
 *
 *              2)      The following macro is defined if shared rwlocks
 *                      are supported:
 *                              _POSIX_THREAD_PROCESS_SHARED
 *
 * RESULTS
 *              0               successfully set attribute,
 *              EINVAL          'attr' or pshared is invalid,
 *              ENOSYS          PTHREAD_PROCESS_SHARED not supported,
 *
 * ------------------------------------------------------
 */
int pthread_rwlockattr_setpshared(pthread_rwlockattr_t * attr, int pshared)
{
	int result;
	if((attr != NULL && *attr != NULL) && ((pshared == PTHREAD_PROCESS_SHARED) || (pshared == PTHREAD_PROCESS_PRIVATE))) {
		if(pshared == PTHREAD_PROCESS_SHARED) {
#if !defined( _POSIX_THREAD_PROCESS_SHARED )
			result = ENOSYS;
			pshared = PTHREAD_PROCESS_PRIVATE;
#else
			result = 0;
#endif
		}
		else 
			result = 0;
		(*attr)->pshared = pshared;
	}
	else
		result = EINVAL;
	return (result);
}