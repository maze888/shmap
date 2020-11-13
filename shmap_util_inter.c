#include "shmap.h"
#include "siphash_inter.h"
#include "shmap_util_inter.h"

/* this apis used in only library */

static uint8_t shmap_halfsiphash_key[8] = {
	0x8b, 0x04, 0xe0, 0x12, 0xbb, 0x21, 0xae, 0x57
};

static uint8_t shmap_siphash_key[16] = {
	0xd7, 0xf3, 0xfe, 0xe1, 0x0c, 0xe1, 0xe2, 0xca,
	0xed, 0xe6, 0xb1, 0x37, 0xfd, 0xf0, 0x15, 0x4b
};

int generate_key(const void *data, size_t data_len)
{
	int key = 0;

	(void)halfsiphash(data, data_len, shmap_halfsiphash_key, (uint8_t *)&key, sizeof(key));

	return key;
}

long long generate_long_key(const void *data, size_t data_len)
{
	long long key = 0;

	(void)siphash(data, data_len, shmap_siphash_key, (uint8_t *)&key, sizeof(key));

	return key;
}

int find_key_data(struct shmap *shmap, const void *key, size_t key_len, long long *hash_key, uint8_t **data, size_t *data_len)
{
	size_t data_len_tmp;
	uint8_t *p = shmap->head;
	long long data_key = generate_long_key(key, key_len);

	for ( size_t i = 0; i < shmap->total_data_count; i++ ) {
		if ( memcmp(&data_key, p, sizeof(long long)) == 0 ) { // found
			// copy key
			if ( hash_key ) *hash_key = data_key;
			p += sizeof(long long);

			// copy data length
			if ( data_len ) memcpy(data_len, p, sizeof(size_t));
			p += sizeof(size_t);
			
			// point data (no copy)
			if ( data ) *data = p;

			return i + 1;
		}
		else { // next
			// key
			p += sizeof(long long);
			// data_len
			memcpy(&data_len_tmp, p, sizeof(size_t));
			p += sizeof(size_t);  
			// data
			p += data_len_tmp;
		}
	}

	return 0; // not found
}

void hexdump(const uint8_t *data, size_t len, FILE *fp)
{
	int i, j, k;
	size_t seq = 0;
	unsigned char c, c1;

	while ( len >= 16 ) {
		fprintf(fp, "%07x ", (unsigned int)seq);
		
		for ( i = 1; i <= 16; i++ ) {
			c = data[seq + i - 1];

			if ( c > 0xf ) fprintf(fp, "%x", c);
			else fprintf(fp, "0%x", c);

			if ( i % 2 == 0 ) fprintf(fp, " ");
			
			if ( i == 16 ) {
				fprintf(fp, "   ");

				for ( j = 0; j < 16; j++ ) {
					c1 = data[seq + j];

					if ( isprint(c1) ) fprintf(fp, "%c", c1);
					else fprintf(fp, ".");
				}
			}
		}
		fprintf(fp, "\r\n");

		seq += 16;
		len -= 16;
	} 

	if ( len > 0 ) {
		fprintf(fp, "%07x ", (unsigned int)seq);
		
		for ( i = 1; i <= len; i++ ) {
			c = data[seq + i - 1];
			
			if ( c > 0xf ) fprintf(fp, "%x", c);
			else fprintf(fp, "0%x", c);
			
			if ( i % 2 == 0 ) fprintf(fp, " ");
		}
			
		fprintf(fp, "   ");
		if ( len % 2 ) fprintf(fp, " ");

		for ( j = 1; j <= 16 - len; j++ ) {
			fprintf(fp, "  ");

			if ( j % 2 == 0 ) fprintf(fp, " ");
		}
		
		for ( k = 0; k < len; k++ ) {
			c1 = data[seq + k];

			if ( isprint(c1) ) fprintf(fp, "%c", c1);
			else fprintf(fp, ".");
		}
		
		fprintf(fp, "\r\n");
	}
}
