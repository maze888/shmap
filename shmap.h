#pragma once

#include <stdio.h>
#include <stdint.h>
#include <sys/sem.h>

#define SHMAP_MAX_NAME 128

// create options
#define SHMAP_AUTO_EXTEND          0x01
#define SHMAP_ALLOW_REDUNDANCY_KEY 0x02

// dump options
#define SHMAP_DUMP_HEADER_ONLY 0x01
#define SHMAP_DUMP_RAW         0x02

/* data structure in shared memory
 *
 * magic number   | version        | shm size       | shm used size  | total data count | key   | data length    | data     | ... | key   | data length    | data
   sizeof(size_t) | sizeof(size_t) | sizeof(size_t) | sizeof(size_t) | sizeof(size_t)   | 8byte | sizeof(size_t) | variable | ... | 8byte | sizeof(size_t) | variable 
*/

struct shmap {
	char name[SHMAP_MAX_NAME];
	int option;
	
	// semaphore control
	key_t sem_key;
	int sem_id;

	// shared memory control
	key_t shm_key;
	int shm_id;

	// meta data in shared memory
	size_t magic_number;
	size_t version;
	size_t shm_size;
	size_t shm_used_size;
	size_t total_data_count;
	
	// data in shared memory
	uint8_t *shm;  // attached memory pointer
	uint8_t *head; // data start pointer
};

struct shmap * alloc_shmap(const char *name, size_t size, int option);
int free_shmap(struct shmap *shmap);

int shmap_insert(struct shmap *shmap, const void *key, size_t key_len, const void *data, size_t data_len);
int shmap_copy(struct shmap *shmap, const void *key, size_t key_len, void **data, size_t *data_len);
int shmap_erase(struct shmap *shmap, const void *key, size_t key_len);
void shmap_clean(struct shmap *shmap);

int dump_shmap(struct shmap *shmap, FILE *fp, int option);

const char * shmap_get_last_error();
