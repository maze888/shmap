#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdint.h>
#include <ctype.h>
#include <errno.h>
#include <assert.h>
#include <pthread.h>
#include <sys/shm.h>

#define safe_free(p) if ( p ) { free(p); p = NULL; }

#define CKNUL(p) p ? "valid" : "null"

#define SHMAP_MIN_SIZE 1024
#define SHMAP_METADATA_SIZE (sizeof(size_t) /* magic_number */ + sizeof(size_t) /* version */ + sizeof(size_t) /* shm_size */ + sizeof(size_t) /* shm_used_size */ + sizeof(size_t) /* total_data_count */)
#define SHMAP_NODE_METADATA_SIZE (sizeof(long long) /* hash key */ + sizeof(size_t) /* data length */)

#define SHMAP_SET_ERROR(...) shmap_set_last_error(__func__, __VA_ARGS__);

extern char shmap_last_error[1024];
extern size_t shmap_magic_number;
extern size_t shmap_version;

void shmap_set_last_error(const char *func, char *fmt, ...);
