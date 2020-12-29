#include "shmap_common_inter.h"

char shmap_last_error[1024] = {0};
size_t shmap_magic_number = 0x12131415;
size_t shmap_version      = 0x10000000;

void shmap_set_last_error(const char *func, char *fmt, ...)
{
	va_list ap;
	char buf[512] = {0};

	memset(shmap_last_error, 0x00, sizeof(shmap_last_error));

	va_start(ap, fmt);
	vsnprintf(buf, sizeof(buf), fmt, ap);
	va_end(ap);

	snprintf(shmap_last_error, sizeof(shmap_last_error), "%s(): %s", func, buf);
}
