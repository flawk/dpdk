/* SPDX-License-Identifier: BSD-3-Clause
 * Copyright 2021 Mellanox Technologies, Ltd
 * Copyright (C) 2022 Microsoft Corporation
 */

#include <errno.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>

#include <rte_errno.h>
#include <rte_log.h>
#include <rte_thread.h>

struct eal_tls_key {
	pthread_key_t thread_index;
};

rte_thread_t
rte_thread_self(void)
{
	RTE_BUILD_BUG_ON(sizeof(pthread_t) > sizeof(uintptr_t));

	rte_thread_t thread_id;

	thread_id.opaque_id = (uintptr_t)pthread_self();

	return thread_id;
}

int
rte_thread_key_create(rte_thread_key *key, void (*destructor)(void *))
{
	int err;

	*key = malloc(sizeof(**key));
	if ((*key) == NULL) {
		RTE_LOG(DEBUG, EAL, "Cannot allocate TLS key.\n");
		rte_errno = ENOMEM;
		return -1;
	}
	err = pthread_key_create(&((*key)->thread_index), destructor);
	if (err) {
		RTE_LOG(DEBUG, EAL, "pthread_key_create failed: %s\n",
			 strerror(err));
		free(*key);
		rte_errno = ENOEXEC;
		return -1;
	}
	return 0;
}

int
rte_thread_key_delete(rte_thread_key key)
{
	int err;

	if (!key) {
		RTE_LOG(DEBUG, EAL, "Invalid TLS key.\n");
		rte_errno = EINVAL;
		return -1;
	}
	err = pthread_key_delete(key->thread_index);
	if (err) {
		RTE_LOG(DEBUG, EAL, "pthread_key_delete failed: %s\n",
			 strerror(err));
		free(key);
		rte_errno = ENOEXEC;
		return -1;
	}
	free(key);
	return 0;
}

int
rte_thread_value_set(rte_thread_key key, const void *value)
{
	int err;

	if (!key) {
		RTE_LOG(DEBUG, EAL, "Invalid TLS key.\n");
		rte_errno = EINVAL;
		return -1;
	}
	err = pthread_setspecific(key->thread_index, value);
	if (err) {
		RTE_LOG(DEBUG, EAL, "pthread_setspecific failed: %s\n",
			strerror(err));
		rte_errno = ENOEXEC;
		return -1;
	}
	return 0;
}

void *
rte_thread_value_get(rte_thread_key key)
{
	if (!key) {
		RTE_LOG(DEBUG, EAL, "Invalid TLS key.\n");
		rte_errno = EINVAL;
		return NULL;
	}
	return pthread_getspecific(key->thread_index);
}

int
rte_thread_set_affinity_by_id(rte_thread_t thread_id,
		const rte_cpuset_t *cpuset)
{
	return pthread_setaffinity_np((pthread_t)thread_id.opaque_id,
		sizeof(*cpuset), cpuset);
}

int
rte_thread_get_affinity_by_id(rte_thread_t thread_id,
		rte_cpuset_t *cpuset)
{
	return pthread_getaffinity_np((pthread_t)thread_id.opaque_id,
		sizeof(*cpuset), cpuset);
}
