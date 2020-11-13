#include "shmap.h"
#include "shmap_common_inter.h"
#include "shmap_resource_inter.h"
#include "shmap_util_inter.h"

struct shmap * alloc_shmap(const char *name, size_t size, int option)
{
	struct shmap *shmap = NULL;

	if ( !name ) {
		SHMAP_SET_ERROR("invalid argument: name = NULL");
		goto out;
	}

	if ( strlen(name) >= SHMAP_MAX_NAME - 1) {
		SHMAP_SET_ERROR("invalid argument: (must name length < %d)", SHMAP_MAX_NAME);
		goto out;
	}

	if ( size < SHMAP_MIN_SIZE ) {
		SHMAP_SET_ERROR("invalid argument: (must size > %d)", SHMAP_MIN_SIZE);
		goto out;
	}

	assert(shmap = calloc(1, sizeof(*shmap)));

	snprintf(shmap->name, sizeof(shmap->name), "%s", name);
	shmap->option = option;

	if ( create_binary_semaphore(shmap, name) < 0 ) goto out;

	if ( create_shared_memory(shmap, name, size) < 0 ) goto out;

	return shmap;

out:
	(void)free_shmap(shmap);

	return NULL;
}

int free_shmap(struct shmap *shmap)
{
	if ( shmap ) {
		if ( shmdt(shmap->shm) < 0 ) {
			SHMAP_SET_ERROR("shmdt() is failed: (error: %s, errno: %d)", strerror(errno), errno);
			return -1;
		}
		
		safe_free(shmap);
	}

	return 0;
}

int shmap_insert(struct shmap *shmap, const void *key, size_t key_len, const void *data, size_t data_len)
{
	uint8_t *p;
	size_t allow_size;

	if ( !shmap || !key || !key_len || !data || !data_len ) {
		SHMAP_SET_ERROR("invalid argument: (shmap = %s, key = %s, key_len = %lu, data = %s, data_len = %lu)", CKNUL(shmap), CKNUL(key), key_len, CKNUL(data), data_len);
		return -1;
	}
	
	binary_semaphore_lock(shmap);

	update_shmap(shmap);

	// checking size
	allow_size = shmap->shm_size - shmap->shm_used_size;
	if ( allow_size >= shmap->shm_size ) { // should not occur ..
		SHMAP_SET_ERROR("invalid allow size: (allow_size: %lu, shm_size: %lu, shm_used_size: %lu)", allow_size, shmap->shm_size, shmap->shm_used_size);
		goto out;
	}

	if ( allow_size < data_len ) {
		SHMAP_SET_ERROR("not enough shared memory: (allow_size: %lu, data_len: %lu)", allow_size, data_len);
		goto out;
	}

	// prevent redunduncy data
	if ( find_key_data(shmap, key, key_len, NULL, NULL, NULL) ) {
		SHMAP_SET_ERROR("already exist key");
		goto out;
	}

	// tail memory pointer
	p = shmap->shm + shmap->shm_used_size;
	
	long long hash_key = generate_long_key(key, key_len);

	// insert hash key
	memcpy(p, &hash_key, sizeof(long long));
	p += sizeof(long long);
	
	// insert data length
	memcpy(p, &data_len, sizeof(size_t));
	p += sizeof(size_t);
	
	// insert data
	memcpy(p, data, data_len);
	
	// update shared memory
	shmap->shm_used_size += SHMAP_NODE_METADATA_SIZE + data_len;
	shmap->total_data_count++;
	
	update_shared_memory(shmap);
	
	binary_semaphore_unlock(shmap);

	return 0;

out:
	binary_semaphore_unlock(shmap);

	return -1;
}

int shmap_copy(struct shmap *shmap, const void *key, size_t key_len, void **data, size_t *data_len)
{
	uint8_t *data_in_shm;

	if ( !shmap || !key || !key_len || !data || !data_len ) {
		SHMAP_SET_ERROR("invalid argument: (shmap = %s, key = %s, key_len = %lu, data = %s, data_len = %lu)", CKNUL(shmap), CKNUL(key), key_len, CKNUL(data), data_len);
		return -1;
	}

	binary_semaphore_lock(shmap);
	
	update_shmap(shmap);
	
	if ( !find_key_data(shmap, key, key_len, NULL, &data_in_shm, data_len) ) {
		SHMAP_SET_ERROR("not exist data");
		goto out;
	}

	assert(*data = calloc(1, *data_len + 1 /* null terminator */));

	memcpy(*data, data_in_shm, *data_len);

	binary_semaphore_unlock(shmap);

	return 0;

out:
	binary_semaphore_unlock(shmap);

	return -1;
}

static void erase_data(struct shmap *shmap, int index, uint8_t *data, size_t data_len)
{
	uint8_t *p;
	uint8_t *copy_start;
	size_t copy_len = 0, data_len_tmp;

	p = data - SHMAP_NODE_METADATA_SIZE;
	
	// erase data
	memset(p, 0x00, SHMAP_NODE_METADATA_SIZE + data_len);

	if ( index != shmap->total_data_count ) {
		copy_start = p = data + data_len;

		// calculate copy length & copy
		for ( size_t i = 1; i <= shmap->total_data_count - index; i++ ) {
			// key
			p += sizeof(long long);
			// data len
			memcpy(&data_len_tmp, p, sizeof(size_t));
			p += sizeof(size_t);
			// data
			p += data_len_tmp;

			copy_len += SHMAP_NODE_METADATA_SIZE + data_len_tmp;
		}
		memmove(data - SHMAP_NODE_METADATA_SIZE, copy_start, copy_len);

		// erase garbage data
		p -= data_len + SHMAP_NODE_METADATA_SIZE;
		memset(p, 0x00, SHMAP_NODE_METADATA_SIZE + data_len);
	}
}

int shmap_erase(struct shmap *shmap, const void *key, size_t key_len)
{
	int index;
	uint8_t *data;
	size_t data_len;

	if ( !shmap || !key || !key_len ) {
		SHMAP_SET_ERROR("invalid argument: (shmap = %s, key = %s, key_len = %lu)", CKNUL(shmap), CKNUL(key), key_len);
		return -1;
	}

	binary_semaphore_lock(shmap);
	
	update_shmap(shmap);
	
	index = find_key_data(shmap, key, key_len, NULL, &data, &data_len);
	if ( !index ) {
		SHMAP_SET_ERROR("not exist data");
		goto out;
	}

	switch (shmap->total_data_count) {
		case 1:
			memset(shmap->head, 0x00, SHMAP_NODE_METADATA_SIZE + data_len);
			break;
		default:
			erase_data(shmap, index, data, data_len);
			break;
	}

	shmap->total_data_count--;
	shmap->shm_used_size -= SHMAP_NODE_METADATA_SIZE + data_len;
	
	update_shared_memory(shmap);
	
	binary_semaphore_unlock(shmap);

	return 0;

out:
	binary_semaphore_unlock(shmap);

	return -1;
}

void shmap_clean(struct shmap *shmap)
{
	if ( shmap ) {
		binary_semaphore_lock(shmap);

		update_shmap(shmap);

		memset(shmap->shm, 0x00, shmap->shm_used_size);
		shmap->shm_used_size = 0;
		shmap->total_data_count = 0;

		update_shared_memory(shmap);

		binary_semaphore_unlock(shmap);
	}
}

int dump_shmap(struct shmap *shmap, FILE *fp, int option)
{
	uint8_t *p;
	size_t data_len;

	if ( !shmap || !fp ) {
		SHMAP_SET_ERROR("invalid argument: (shmap = %s, fp = %s)", CKNUL(shmap), CKNUL(fp));
		goto out;
	}

	binary_semaphore_lock(shmap);

	update_shmap(shmap);

	if ( option & SHMAP_DUMP_RAW ) {
		fprintf(fp, "---------- shmap raw dump --------\n");
		hexdump(shmap->shm, shmap->shm_used_size, fp);
	}
	else {
		fprintf(fp, "---------- shmap header ----------\n");
		fprintf(fp, "%-18s: 0x%lx\n", "magic number", shmap->magic_number);
		fprintf(fp, "%-18s: 0x%lx\n", "version", shmap->version);
		fprintf(fp, "%-18s: %lu\n", "shm size", shmap->shm_size);
		fprintf(fp, "%-18s: %lu\n", "shm used size", shmap->shm_used_size);
		fprintf(fp, "%-18s: %lu\n\n", "total data count", shmap->total_data_count);

		if ( option & SHMAP_DUMP_HEADER_ONLY ) return 0;

		fprintf(fp, "---------- shmap data ------------\n");
		p = shmap->head;
		for ( size_t i = 0; i < shmap->total_data_count; i++ ) {
			fprintf(fp, "[index]: %lu\n", i + 1);

			// print key
			fprintf(fp, "[key]\n");
			hexdump(p, sizeof(long long), fp);
			p += sizeof(long long);

			// print data length
			memcpy(&data_len, p, sizeof(size_t));
			fprintf(fp, "[data length]: %lu\n", data_len);
			p += sizeof(size_t);

			// print data
			fprintf(fp, "[data]\n");
			hexdump(p, data_len, fp);
			p += data_len;

			fprintf(fp, "\n");
		}
	}

	fflush(fp);

	binary_semaphore_unlock(shmap);

	return 0;

out:
	fprintf(fp, "%s\n", shmap_get_last_error());

	return -1;
}

const char * shmap_get_last_error()
{
	return shmap_last_error;
}
