#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "shm.h"

struct SHM {
	unsigned int id;
	size_t length;
	void *data;
};

/*
 * Adding getter for id, setter not required since we want it read-only.
 */
shm_id_t shm_get_id(shm *shared)
{
	if (shared == NULL) {
#ifndef TEST
		fprintf(stderr,
			"shm.c - shm_getid() : SHM instance is NULL.\n");
#endif
		return 0;
	}

	return shared->id;
}

/*
 * Create a shm instance with autogenerated id.
 */
shm *shm_create(key_t key, size_t length)
{
	shm *shared;

	if (length == 0) {
#ifndef TEST
		fprintf(stderr,
			"shm.c - shm_create() : Cannot allocate 0 bytes.\n");
#endif
		return NULL;
	}

	/* Creating new shm instance */
	shared = (shm *)malloc(sizeof(shm));
	if (shared == NULL) {
		fprintf(stderr,
			"shm.c - shm_create() : Computer refuses to give more memory.\n");
		return NULL;
	}

	/* Generating new ID */
	shared->id = shmget(key, length,
			    IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR);
	if (shared->id == -1) {
		fprintf(stderr,
			"shm.c - shm_create() : invalid shared memory reference\n");
		return NULL;
	}

	shared->length = length;

	shared->data = shmat((int)shared->id, 0, 0);
	if (shared->data == NULL) {
		fprintf(stderr,
			"shm.c - shm_create() : Computer refuses to give more shared memory.\n");
		return NULL;
	}

	return shared;
}

/*
 * Delete given shm instance and frees the memory.
 */
void shm_delete(shm *shared)
{
	if (shared == NULL) {
#ifndef TEST
		fprintf(stderr,
			"shm.c - shm_delete() : SHM instance is NULL.\n");
#endif
		return;
	}

	shmctl((int)shared->id, IPC_RMID, 0);
	shmdt(shared->data);
	free(shared);
}

/*
 * Create a shm instance that is attachable to a pre-allocated shm segment,
 * given proper id
 */
shm *shm_attach(shm_id_t id, size_t length)
{
	shm *shared;

	if (length == 0) {
#ifndef TEST
		fprintf(stderr,
			"shm.c - shm_open() : Cannot allocate 0 bytes.\n");
#endif
		return NULL;
	}

	/* Creating new shm instance */
	shared = (shm *)malloc(sizeof(shm));
	if (shared == NULL) {
		fprintf(stderr,
			"shm.c - shm_open() : Computer refuses to give more memory.\n");
		return NULL;
	}

	shared->id = id;

	shared->length = length;

	/* Attach to existent shared memory instance */
	shared->data = shmat((int)shared->id, 0, 0);
	if (shared->data == NULL) {
		fprintf(stderr,
			"shm.c - shm_open() : Computer refuses to give more shared memory.\n");
		return NULL;
	}

	return shared;
}

/*
 * Remove shm instance without detaching it.
 */
void shm_detach(shm *shared)
{
	if (shared == NULL) {
#ifndef TEST
		fprintf(stderr,
			"shm.c - shm_close() : SHM instance is NULL.\n");
#endif
		return;
	}

	shmdt(shared->data);
	free(shared);
}

void *shm_read(shm *shared)
{
	if (shared == NULL) {
#ifndef TEST
		fprintf(stderr, "shm.c - shm_read() : SHM instance is NULL.\n");
#endif
		return NULL;
	}

	return shared->data;
}

void shm_write(shm *shared, const void *data, size_t length)
{
	unsigned int u;

	if (shared == NULL) {
#ifndef TEST
		fprintf(stderr,
			"shm.c - shm_write() : SHM instance is NULL.\n");
#endif
		return;
	}
	if (data == NULL) {
#ifndef TEST
		fprintf(stderr, "shm.c - shm_write() : Given data is NULL.\n");
#endif
		return;
	}
	if (length > shared->length) {
#ifndef TEST
		fprintf(stderr,
			"shm.c - shm_write() : Data given are greater than reserved memory.\n");
#endif
		return;
	}

	memcpy(shared->data, data, length);
}