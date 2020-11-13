#include "shmap_common_inter.h"

int generate_key(const void *data, size_t data_len);
long long generate_long_key(const void *data, size_t data_len);
int find_key_data(struct shmap *shmap, const void *key, size_t key_len, long long *hash_key, uint8_t **data, size_t *data_len);
void hexdump(const uint8_t *data, size_t len, FILE *fp);
