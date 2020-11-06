#ifndef _assets_h_
#define _assets_h_

#include <stdint.h>

typedef struct
{
	uint8_t count;
	uint8_t width;
	int8_t points[62]; // up to 31 xy points
} hershey_char_t;


extern const hershey_char_t hershey_simplex[];

void assets_test_pattern(void);

#endif
