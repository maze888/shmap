#include <string.h>
#include <stdlib.h>

#include "shmap.h"

static int insert_test(struct shmap *shmap)
{
	char key[] = "test_key";
	char data[] = "abcd1234";
	char key2[] = "test_key2";
	char data2[] = "zzzzz";
	char key3[] = "test_key3";
	char data3[] = "xxxxx123456789";

	if ( shmap_insert(shmap, key, strlen(key), data, strlen(data)) < 0 ) {
		fprintf(stderr, "%s\n", shmap_get_last_error());
	}
	
	if ( shmap_insert(shmap, key2, strlen(key2), data2, strlen(data2)) < 0 ) {
		fprintf(stderr, "%s\n", shmap_get_last_error());
	}
	
	if ( shmap_insert(shmap, key3, strlen(key3), data3, strlen(data3)) < 0 ) {
		fprintf(stderr, "%s\n", shmap_get_last_error());
	}

	//char *copied_data;
	//size_t copied_data_len;

	/*if ( shmap_copy(shmap, key2, strlen(key2), (void **)&copied_data, &copied_data_len) < 0 ) {
		goto out;
	}
	printf("copy man? : %s %lu\n", copied_data, copied_data_len);
	free(copied_data);
	
	if ( shmap_copy(shmap, key, strlen(key), (void **)&copied_data, &copied_data_len) < 0 ) {
		goto out;
	}
	printf("copy man? : %s %lu\n", copied_data, copied_data_len);
	free(copied_data);*/
	
	/*if ( shmap_copy(shmap, fake_key, strlen(fake_key), (void **)&copied_data, &copied_data_len) < 0 ) {
		goto out;
	}
	printf("copy man? : %s %lu\n", copied_data, copied_data_len);
	free(copied_data);*/

	if ( shmap_erase(shmap, key2, strlen(key2)) < 0 ) {
		goto out;
	}
	if ( shmap_erase(shmap, key, strlen(key)) < 0 ) {
		goto out;
	}

	return 0;

out:
	fprintf(stderr, "%s\n", shmap_get_last_error());

	return -1;
}

int main(int argc, char **argv)
{
	struct shmap *shmap;

	if ( !(shmap = alloc_shmap("test_map", 1024 * 1024, 0)) ) {
		fprintf(stderr, "%s\n", shmap_get_last_error());
		goto out;
	}
	
	if ( insert_test(shmap) < 0 ) goto out;
	
	dump_shmap(shmap, stdout, 0);
	dump_shmap(shmap, stdout, SHMAP_DUMP_RAW);
	
	free_shmap(shmap);

	return 0;

out:
	free_shmap(shmap);

	return 1;
}
