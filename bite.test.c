#include <string.h>
#include <stdio.h>

#define BITE_DEBUG

void print_binary(unsigned int value, int bits) {
#ifndef BITE_DEBUG
	int i;

	for (i = bits - 1; i >= 0; i--) {
		putchar((value & (1U << i)) ? '1' : '0');
	}
	putchar('\n');
#endif
	(void)value;
	(void)bits;
}

#include "bite.h"

struct bite bite;
uint8_t buf[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
void clearbuf(uint8_t val)
{
	int i;

	for (i = 0; i < 8; i++) { buf[i] = val; }
}

void bite_test_print_result(uint8_t *buf)
{
	int i;

	for (i = 0; i < 8; i++) {
		printf("0x%02X ", buf[i]);
	}
	putchar('\n');

	fflush(0);
}

void bite_test_print_result_binary(uint8_t *buf)
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
	uint8_t res = 0;

	/* Trigger UBsan
	 * volatile uint8_t x = 0xFF;
	   volatile uint8_t y = 32;
	   x >> y; */

	bite_init(&bite, buf/*, 8*/);
	
	/* TEST NORMAL */
	clearbuf(0xAA);
	bite_begin(&bite, 15, 12, BITE_ORDER_BIG_ENDIAN);
	bite_write(&bite, 0x12);
	bite_write(&bite, 0x34);
	bite_end(&bite);
	bite_test_print_result(buf);
	assert(bite.flags == 0);

	/* TEST OVERFLOW */
	bite_begin(&bite, 16, 16, BITE_ORDER_BIG_ENDIAN);
	bite_write(&bite, 0x56);
	bite_write(&bite, 0x78);
	bite_write(&bite, 0x90);
	bite_end(&bite);
	bite_test_print_result(buf);
	assert(bite.flags != 0);

	/* TEST MISALIGNED (one byte) */
	clearbuf(0x00);
	bite_begin(&bite, 0, 2, BITE_ORDER_BIG_ENDIAN);
	bite_write(&bite, 0xFF);
	bite_write(&bite, 0xFF);
	bite_end(&bite);
	bite_test_print_result(buf);
	bite_test_print_result_binary(buf);

	bite_begin(&bite, 0, 2, BITE_ORDER_BIG_ENDIAN);
	assert(bite_read(&bite) == 0x03);
	bite_end(&bite);

	/* TEST MISALIGNED (multibyte) */
	clearbuf(0xAA);
	bite_begin(&bite, 2, 16, BITE_ORDER_BIG_ENDIAN);
	bite_write(&bite, 0xFF);
	bite_write(&bite, 0xFF);
	bite_end(&bite);
	bite_test_print_result(buf);
	bite_test_print_result_binary(buf);

	bite_begin(&bite, 2, 16, BITE_ORDER_BIG_ENDIAN);
	assert(bite_read(&bite) == 0xFF);
	assert(bite_read(&bite) == 0xFF);
	bite_end(&bite);

	/* TEST MISALIGNED (Ending) */
	clearbuf(0xAA);
	bite_begin(&bite, 8, 9, BITE_ORDER_BIG_ENDIAN);
	bite_write(&bite, 0xF2);
	bite_write(&bite, 0xAF);
	bite_end(&bite);
	bite_test_print_result(buf);
	bite_test_print_result_binary(buf);

	bite_begin(&bite, 8, 9, BITE_ORDER_BIG_ENDIAN);
	assert(bite_read(&bite) == 0xF2);
	assert(bite_read(&bite) == 0xAF >> 7);
	bite_end(&bite);

	/* TEST UNDERFLOW */
	bite_begin(&bite, 16, 16, BITE_ORDER_BIG_ENDIAN);
	bite_write(&bite, 0x12);
	bite_end(&bite);

	bite_test_print_result(buf);
	assert(bite.flags == BITE_FLAG_UNDERFLOW);

	/* TEST OTHER */
	bite_init(&bite, buf/*, 8*/);
	
	clearbuf(0xAA);
	bite_begin(&bite, 1, 8, BITE_ORDER_BIG_ENDIAN);
	bite_write(&bite, 0xFF);
	bite_test_print_result(buf);
	bite_test_print_result_binary(buf);

	bite_rewind(&bite);
	res = bite_read(&bite);
	print_binary(res, 8);
	assert(res == 0xFF);
	bite_end(&bite);
}

void bite_test_brute(enum bite_order order)
{
	struct bite bite;
	int i, j;

	/* Buffers will be compared and should remain intact 
	 * after all bitE operations! */
	uint8_t buf_a[8] = {0x12, 0x34, 0x56, 0x78, 0x89, 0x9A, 0xAB, 0xBC};
	uint8_t buf_b[8] = {0x12, 0x34, 0x56, 0x78, 0x89, 0x9A, 0xAB, 0xBC};
	
	/* Temporary buffer */
	uint8_t buf_t[8];
	
	bite_init(&bite, buf_a);
	
	for (i = 0; i < 32; i++) {
		for (j = 1; j <= 32; j++) {
			/* Read values from buffer buf_a and store to buf_t */
			bite_begin(&bite, i, j, order);
			buf_t[0] = bite_read(&bite);
			buf_t[1] = bite_read(&bite);
			buf_t[2] = bite_read(&bite);
			buf_t[3] = bite_read(&bite);
			bite_end(&bite);

			bite_test_print_result(buf_a);
			bite_test_print_result_binary(buf_a);
			bite_test_print_result(buf_b);
			bite_test_print_result_binary(buf_b);

			/* Check if buffers still intact */
			assert(!memcmp(buf_a, buf_b, 8));

			/* Override buf_a values */
			bite_begin(&bite, i, j, order);
			bite_write(&bite, 0xEF);
			bite_write(&bite, 0xFF);
			bite_write(&bite, 0xCD);
			bite_write(&bite, 0xAB);
			bite_end(&bite);

			bite_test_print_result(buf_a);
			bite_test_print_result_binary(buf_a);

			/* Check if buffers are different */
			/* assert(memcmp(buf_a, buf_b, 8)); */

			/* Rewrite buf_a with the previous values */
			bite_begin(&bite, i, j, order);
			bite_write(&bite, buf_t[0]);
			bite_write(&bite, buf_t[1]);
			bite_write(&bite, buf_t[2]);
			bite_write(&bite, buf_t[3]);
			bite_end(&bite);

		}
	}
}

int main()
{
	bite_init(&bite, buf);
	
	clearbuf(0x00);
	bite_begin(&bite, 0, 8, BITE_ORDER_LIL_ENDIAN);
	bite_write(&bite, 0xAA);
	bite_end(&bite);
	bite_test_print_result(buf);
	bite_test_print_result_binary(buf);

	bite_begin(&bite, 0, 8, BITE_ORDER_LIL_ENDIAN);
	assert(bite_read(&bite) == 0xAA);
	bite_end(&bite);

	/*clearbuf(0x00);
	bite_begin(&bite, idx2, 8);
	bite_write(&bite, 0xFA);
	bite_end(&bite);*/
	/*bite_test_print_result();
	bite_test_print_result_binary();*/

	/* bite_test(); */

	bite_test_brute(BITE_ORDER_BIG_ENDIAN);
	bite_test_brute(BITE_ORDER_LIL_ENDIAN);

	return 0;
}
