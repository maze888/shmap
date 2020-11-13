#include "shmap_common_inter.h"

int create_binary_semaphore(struct shmap *shmap, const char *name);
int create_shared_memory(struct shmap *shmap, const char *name, size_t size);

void binary_semaphore_lock(struct shmap *shmap);
void binary_semaphore_unlock(struct shmap *shmap);

void update_shmap(struct shmap *shmap);
void update_shared_memory(struct shmap *shmap);
