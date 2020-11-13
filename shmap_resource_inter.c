#include "shmap.h"
#include "shmap_common_inter.h"
#include "shmap_util_inter.h"
#include "shmap_resource_inter.h"

static pthread_mutex_t shmap_resource_lock = PTHREAD_MUTEX_INITIALIZER;

int create_binary_semaphore(struct shmap *shmap, const char *name)
{
	pthread_mutex_lock(&shmap_resource_lock);

	// create or get(exist) semaphore
	shmap->sem_key = generate_key(name, strlen(name));

	shmap->sem_id = semget(shmap->sem_key, 1, IPC_CREAT | IPC_EXCL | 0660);
	if ( shmap->sem_id < 0 ) {
		if ( errno == EEXIST ) {
			shmap->sem_id = semget(shmap->sem_key, 0, IPC_CREAT | 0660);
			if ( shmap->sem_id < 0 ) {
				SHMAP_SET_ERROR("semget() is failed: (error: %s, errno: %d)", strerror(errno), errno);
				goto out;
			}
	
			pthread_mutex_unlock(&shmap_resource_lock);

			return 0;
		}
		else {
			SHMAP_SET_ERROR("semget() is failed: (error: %s, errno: %d)", strerror(errno), errno);
			goto out;
		}
	}

	// semaphore argument
	union semun {
		int                  val;
		struct   semid_ds   *buf;
		unsigned short int  *arrary;
	} arg;

	// binary semaphore
	arg.val = 1;
	if ( semctl(shmap->sem_id, 0, SETVAL, arg) < 0 ) {
		SHMAP_SET_ERROR("semctl() is failed: (error: %s, errno: %d)", strerror(errno), errno);
		goto out;
	}
	
	pthread_mutex_unlock(&shmap_resource_lock);
	
	return 0;

out:
	pthread_mutex_unlock(&shmap_resource_lock);

	return -1;
}

int create_shared_memory(struct shmap *shmap, const char *name, size_t size)
{
	int setup_metadata = 0;
	
	shmap->shm_key = generate_key(name, strlen(name));

	if ( size == 0 ) { // already exist
		shmap->shm_id = shmget(shmap->shm_key, size, IPC_CREAT | 0660);
		if ( shmap->shm_id < 0 ) {
			SHMAP_SET_ERROR("shmget() is failed: (error: %s, errno: %d)", strerror(errno), errno);
			goto out;
		}
	}
	else {
		shmap->shm_id = shmget(shmap->shm_key, size, IPC_CREAT | IPC_EXCL | 0660);
		if ( shmap->shm_id < 0 ) {
			if ( errno == EEXIST ) {
				shmap->shm_id = shmget(shmap->shm_key, 0, IPC_CREAT | 0660);
				if ( shmap->shm_id < 0 ) {
					SHMAP_SET_ERROR("shmget() is failed: (error: %s, errno: %d)", strerror(errno), errno);
					goto out;
				}
			}
			else {
				SHMAP_SET_ERROR("shmget() is failed: (error: %s, errno: %d)", strerror(errno), errno);
				goto out;
			}
		}
		else {
			// created shared memory at first time. must setup metadata.
			setup_metadata = 1;
		}
	}
	
	binary_semaphore_lock(shmap);
	
	shmap->shm = shmat(shmap->shm_id, NULL, 0);
	if ( shmap->shm == (void *)-1 ) {
		SHMAP_SET_ERROR("shmat() is failed: (error: %s, errno: %d)", strerror(errno), errno);
		goto out;
	}

	shmap->head = shmap->shm + SHMAP_METADATA_SIZE;

	if ( setup_metadata ) {
		// created shared memory at first time. must setup metadata.
		shmap->magic_number = shmap_magic_number;
		shmap->version = shmap_version;
		shmap->shm_size = size;
		shmap->shm_used_size = SHMAP_METADATA_SIZE;
		shmap->total_data_count = 0;

		update_shared_memory(shmap);
	}
	else {
		update_shmap(shmap);
	}
	
	binary_semaphore_unlock(shmap);
	
	return 0;

out:
	return -1;
}

void binary_semaphore_lock(struct shmap *shmap)
{
	struct sembuf pbuf;
	union semun{
		int                  val;
		struct   semid_ds   *buf;
		unsigned short int  *arrary;
	}arg;

	pbuf.sem_num   = 0;
	pbuf.sem_op    = -1;
	pbuf.sem_flg   = SEM_UNDO;
	
	memset(&arg, 0x00, sizeof(arg));

	assert(semop(shmap->sem_id, &pbuf, 1) == 0);
	assert(semctl(shmap->sem_id, 0, GETVAL, arg) == 0);
}

void binary_semaphore_unlock(struct shmap *shmap)
{
	struct sembuf pbuf;
	union semun{
		int                  val;
		struct   semid_ds   *buf;
		unsigned short int  *arrary;
	}arg;

	pbuf.sem_num   = 0;
	pbuf.sem_op    = 1;
	pbuf.sem_flg   = SEM_UNDO;
	
	memset(&arg, 0x00, sizeof(arg));
	
	assert(semop(shmap->sem_id, &pbuf, 1) == 0);
	assert(semctl(shmap->sem_id, 0, GETVAL, arg) == 1);
}

void update_shmap(struct shmap *shmap)
{
	uint8_t *p = shmap->shm;

	// copy magic number
	memcpy(&shmap->magic_number, p, sizeof(shmap->magic_number));
	p += sizeof(shmap->magic_number);
	
	// copy version
	memcpy(&shmap->version, p, sizeof(shmap->version));
	p += sizeof(shmap->version);
	
	// copy shm size 
	memcpy(&shmap->shm_size, p, sizeof(shmap->shm_size));
	p += sizeof(shmap->shm_size);
	
	// copy shm used size 
	memcpy(&shmap->shm_used_size, p, sizeof(shmap->shm_used_size));
	p += sizeof(shmap->shm_used_size);
	
	// copy total data count
	memcpy(&shmap->total_data_count, p, sizeof(shmap->total_data_count));
}

void update_shared_memory(struct shmap *shmap)
{
	uint8_t *p = shmap->shm;
	
	// copy magic number
	memcpy(p, &shmap->magic_number, sizeof(shmap->magic_number));
	p += sizeof(shmap->magic_number);

	// copy version
	memcpy(p, &shmap->version, sizeof(shmap->version));
	p += sizeof(shmap->version);

	// copy shm size
	memcpy(p, &shmap->shm_size, sizeof(shmap->shm_size));
	p += sizeof(shmap->shm_size);

	// copy shm used size
	memcpy(p, &shmap->shm_used_size, sizeof(shmap->shm_used_size));
	p += sizeof(shmap->shm_used_size);

	// copy total data count
	memcpy(p, &shmap->total_data_count, sizeof(shmap->total_data_count));
}
