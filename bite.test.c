
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

	bite_init(&bite, buf/*, 8*/);
	
	/* TEST NORMAL */
	clearbuf(0xAA);
	bite_begin(&bite, 16, 16);
	bite_write(&bite, 0x12);
	bite_write(&bite, 0x34);
	bite_end(&bite);
	bite_test_print_result();
	assert(bite.flags == 0);

	/* TEST OVERFLOW */
	bite_begin(&bite, 16, 16);
	bite_write(&bite, 0x56);
	bite_write(&bite, 0x78);
	bite_write(&bite, 0x90);
	bite_end(&bite);
	bite_test_print_result();
	assert(bite.flags != 0);

	/* TEST MISALIGNED (one byte) */
	clearbuf(0xAA);
	bite_begin(&bite, 5, 2);
	bite_write(&bite, 0xFF);
	bite_write(&bite, 0xFF);
	bite_end(&bite);
	bite_test_print_result();

	bite_begin(&bite, 5, 2);
	assert(bite_read(&bite) == 0x03);
	bite_end(&bite);

	/* TEST MISALIGNED (multibyte) */
	clearbuf(0xAA);
	bite_begin(&bite, 2, 16);
	bite_write(&bite, 0xF2);
	bite_write(&bite, 0xAF);
	bite_end(&bite);
	bite_test_print_result();

	bite_begin(&bite, 2, 16);
	assert(bite_read(&bite) == 0xF2);
	assert(bite_read(&bite) == 0xAF);
	bite_end(&bite);

	/* TEST MISALIGNED (Ending) */
	clearbuf(0xAA);
	bite_begin(&bite, 8, 9);
	bite_write(&bite, 0xF2);
	bite_write(&bite, 0xAF);
	bite_end(&bite);
	bite_test_print_result();

	bite_begin(&bite, 8, 9);
	assert(bite_read(&bite) == 0xF2);
	assert(bite_read(&bite) == 0xAF >> 7);
	bite_end(&bite);

	/* TEST UNDERFLOW */
	bite_begin(&bite, 16, 16);
	bite_write(&bite, 0x12);
	bite_end(&bite);

	bite_test_print_result();
	assert(bite.flags == BITE_FLAG_UNDERFLOW);
}


int main()
{
	uint8_t res = 0;

	print_binary(res, 8);

	bite_init(&bite, buf/*, 8*/);
	
	clearbuf(0x00);
	bite_begin(&bite, 1, 8);
	bite_write(&bite, 0xFF);
	bite_test_print_result();
	bite_test_print_result_binary();

	bite_rewind(&bite);
	res = bite_read(&bite);
	print_binary(res, 8);
	assert(res == 0xFF);
	bite_end(&bite);

	bite_test();

	return 0;
}
