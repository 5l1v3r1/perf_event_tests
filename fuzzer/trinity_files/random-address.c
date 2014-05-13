#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/uio.h>
#include <sys/mman.h>	// mprotect

#include "trinity.h"	// page_size
#include "arch.h"	// KERNEL_ADDR etc
#include "log.h"
#include "random.h"
#include "sanitise.h"
#include "maps.h"
#include "shm.h"
#include "tables.h"
#include "utils.h"

void * get_writable_address(unsigned long size)
{
#if VMW
	struct map *map;
#endif
	void *addr = NULL;
	int i;

	/* Because we get called during startup when we create fd's, we need
	 * to special case this, as we can't use get_non_null_address at that point */
	if (getpid() == shm->mainpid)
		return page_rand;

	i = rand() % 7;

	if (size > page_size)
		i = rand_range(5, 6);

	switch (i) {
	case 0:	addr = page_zeros;
		break;
	case 1:	addr = page_0xff;
		break;
	case 2:	addr = page_rand;
		break;
	case 3: addr = page_allocs;
		break;
	case 4:	addr = page_maps;
		break;

//	case 5: map = get_map();
//		addr = map->ptr;
//		mprotect(addr, map->size, PROT_READ|PROT_WRITE);

//		if (rand_bool()) {
//			addr += map->size;
//			addr -= size;
//		}
//		break;

//	case 6: addr = zmalloc(size);	// FIXME: We leak this.
//		if (rand_bool()) {
//			/* place object at end of page. */
//			addr += page_size;
//			addr -= size;
//		}
	default:	addr=page_rand;
		break;
	}

	return addr;

}

static void * _get_address(unsigned char null_allowed)
{
	void *addr = NULL;
	int i;

	/* Because we get called during startup when we create fd's, we need
	 * to special case this, as we can't use get_non_null_address at that point */
	if (getpid() == shm->mainpid)
		return page_rand;

	if (null_allowed == TRUE)
		i = rand() % 4;
	else
		i = rand_range(1, 3);

	switch (i) {
	case 0: addr = NULL;
		break;
	case 1:	addr = (void *) KERNEL_ADDR;
		break;
	case 2:	addr = (void *)(unsigned long)rand64();
		break;

	case 3:	addr = get_writable_address(page_size);
		break;
	}
	return addr;
}

void * get_address(void)
{
	return _get_address(TRUE);
}

void * get_non_null_address(void)
{
	return _get_address(FALSE);
}
#if VMW
unsigned long find_previous_arg_address(unsigned int argnum, unsigned int call, int childno)
{
	struct syscallentry *entry;
	unsigned long addr = 0;

	entry = syscalls[call].entry;

	if (argnum > 1)
		if ((entry->arg1type == ARG_ADDRESS) ||
		    (entry->arg1type == ARG_NON_NULL_ADDRESS))
			addr = shm->syscall[childno].a1;

	if (argnum > 2)
		if ((entry->arg2type == ARG_ADDRESS) ||
		    (entry->arg2type == ARG_NON_NULL_ADDRESS))
			addr = shm->syscall[childno].a2;

	if (argnum > 3)
		if ((entry->arg3type == ARG_ADDRESS) ||
		    (entry->arg3type == ARG_NON_NULL_ADDRESS))
			addr = shm->syscall[childno].a3;

	if (argnum > 4)
		if ((entry->arg4type == ARG_ADDRESS) ||
		    (entry->arg4type == ARG_NON_NULL_ADDRESS))
			addr = shm->syscall[childno].a4;

	if (argnum > 5)
		if ((entry->arg5type == ARG_ADDRESS) ||
		    (entry->arg5type == ARG_NON_NULL_ADDRESS))
			addr = shm->syscall[childno].a5;

	return addr;
}


struct iovec * alloc_iovec(unsigned int num)
{
	struct iovec *iov;

	iov = malloc(num * sizeof(struct iovec));	/* freed by generic_free_arg */
	if (iov != NULL) {
		unsigned int i;

		for (i = 0; i < num; i++) {
			struct map *map;

			map = get_map();
			iov[i].iov_base = map->ptr;
			iov[i].iov_len = rand() % map->size;
		}
	}
	return iov;
}
#endif
