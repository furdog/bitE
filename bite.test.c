
#include <stdio.h>

void print_binary(unsigned int value, int bits) {
	int i;

	for (i = bits - 1; i >= 0; i--) {
		putchar((value & (1U << i)) ? '1' : '0');
	}
	putchar('\n');
}

#include "bite.h"

struct bite bite;
uint8_t buf[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
void clearbuf(uint8_t val)
{
	int i;

	for (i = 0; i < 8; i++) { buf[i] = val; }
}

void bite_test_print_result()
{
	int i;

	for (i = 0; i < 8; i++) {
		printf("0x%02X ", buf[i]);
	}

	printf("\n%s\n", bite.flags & BITE_FLAG_OVERFLOW ? "overflow" : "ok");
	fflush(0);
}

void bite_test_print_result_binary()
{
	int i;
	int j;

	for (j = 0; j < 8; j++) {
		for (i = 8 - 1; i >= 0; i--) {
			putchar((buf[j] & (1U << i)) ? '1' : '0');
		}
		putchar(' ');
	}
	putchar('\n');
}

void bite_test()
{
	/* Trigger UBsan
	 * volatile uint8_t x = 0xFF;
	   volatile uint8_t y = 32;
	   x >> y; */

	clearbuf(0xAA);
	bite_init(&bite, buf/*, 8*/);
	
	bite_config(&bite, 16, 16);

	/* TEST NORMAL */
	bite_write(&bite, 0x12);
	bite_write(&bite, 0x34);
	bite_test_print_result();
	assert(bite.flags == 0);
	
	bite_reset(&bite);
	assert(bite_read(&bite) == 0x12);
	assert(bite_read(&bite) == 0x34);

	/* TEST OVERFLOW */
	bite_write(&bite, 0x56);
	bite_test_print_result();
	assert(bite.flags != 0);

	/* TEST MISALIGNED (one byte) */
	clearbuf(0xAA);
	bite_config(&bite, 5, 2);
	bite_write(&bite, 0xFF);
	bite_write(&bite, 0xFF);
	/* bite_write(&bite, 0x56); */
	bite_test_print_result();

	bite_reset(&bite);
	assert(bite_read(&bite) == 0x03);

	/* print_binary(bite_mix_u8(0xAA, 4, 0xFF), 8); */

	/* TEST MISALIGNED (multibyte) */
	clearbuf(0xAA);
	bite_config(&bite, 2, 16);
	bite_write(&bite, 0xF2);
	bite_write(&bite, 0xAF);
	/* bite_write(&bite, 0x56); */
	bite_test_print_result();

	bite_reset(&bite);
	assert(bite_read(&bite) == 0xF2);
	assert(bite_read(&bite) == 0xAF);

	/* print_binary(bite_mix_u8(0xAA, 4, 0xFF), 8); */

	/* TEST MISALIGNED (Ending) */
	clearbuf(0xAA);
	bite_config(&bite, 8, 9);
	bite_write(&bite, 0xF2);
	bite_write(&bite, 0xAF);
	/* bite_write(&bite, 0x56); */
	bite_test_print_result();

	bite_reset(&bite);
	assert(bite_read(&bite) == 0xF2);
	assert(bite_read(&bite) != 0xAF);

	/* print_binary(bite_mix_u8(0xAA, 4, 0xFF), 8); */

	/* TEST UNDERFLOW */
	bite_config(&bite, 16, 16);
	bite_write(&bite, 0x12);
	bite_test_print_result();
	bite_reset(&bite);
	assert(bite.flags == BITE_FLAG_UNDERFLOW);
}


int main()
{
	uint8_t res = 0;

	/* uint8_t d[2] = {0x00, 0x00};
	
	uint8_t data = 0xFF;
	uint8_t ofs  = 7;

	BITE_COPY_U8(data >> ofs, d[0], ofs, 8 - ofs);
	BITE_COPY_U8(data,        d[1],   0,     ofs);

	print_binary(d[0], 8);
	print_binary(d[1], 8); */

	BITE_COPY_U8_REV(0xF0, res, 0, 4);
	print_binary(res, 8);

	clearbuf(0x00);
	bite_init(&bite, buf/*, 8*/);
	/* bite_config(&bite, 7, 6); */
	bite_config(&bite, 1, 8);
	bite_write(&bite, 0xFF);
	bite_test_print_result();
	bite_test_print_result_binary();

	bite_reset(&bite);
	res = bite_read(&bite);
	print_binary(res, 8);
	assert(res == 0xFF >> 2);

	/* bite_test(); */

	return 0;
}
