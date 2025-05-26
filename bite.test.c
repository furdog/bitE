
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
void clearbuf()
{
	int i;

	for (i = 0; i < 8; i++) { buf[i] = 0xAA; }
}

void bite_print_result()
{
	int i;

	for (i = 0; i < 8; i++) {
		printf("0x%02X ", buf[i]);
	}

	printf("\n%s\n", bite.flags & BITE_FLAG_OVERFLOW ? "overflow" : "ok");
	fflush(0);
}

int main()
{
	/* Trigger UBsan
	 * volatile uint8_t x = 0xFF;
	   volatile uint8_t y = 32;
	   x >> y; */
	
	clearbuf();
	bite_init(&bite, buf/*, 8*/);
	
	bite_config(&bite, 16, 16);

	/* TEST NORMAL */
	bite_write(&bite, 0x12);
	bite_write(&bite, 0x34);
	bite_print_result();
	assert(bite.flags == 0);
	
	bite_reset(&bite);
	assert(bite_read(&bite) == 0x12);
	assert(bite_read(&bite) == 0x34);

	/* TEST OVERFLOW */
	bite_write(&bite, 0x56);
	bite_print_result();
	assert(bite.flags != 0);

	/* TEST MISALIGNED (one byte) */
	clearbuf();
	bite_config(&bite, 5, 2);
	bite_write(&bite, 0xFF);
	bite_write(&bite, 0xFF);
	/* bite_write(&bite, 0x56); */
	bite_print_result();

	bite_reset(&bite);
	assert(bite_read(&bite) == 0x03);

	/* print_binary(bite_mix_u8(0xAA, 4, 0xFF), 8); */

	/* TEST MISALIGNED (multibyte) */
	clearbuf();
	bite_config(&bite, 2, 16);
	bite_write(&bite, 0xF2);
	bite_write(&bite, 0xAF);
	/* bite_write(&bite, 0x56); */
	bite_print_result();

	bite_reset(&bite);
	assert(bite_read(&bite) == 0xF2);
	assert(bite_read(&bite) == 0xAF);

	/* print_binary(bite_mix_u8(0xAA, 4, 0xFF), 8); */

	/* TEST MISALIGNED (Ending) */
	clearbuf();
	bite_config(&bite, 8, 9);
	bite_write(&bite, 0xF2);
	bite_write(&bite, 0xAF);
	/* bite_write(&bite, 0x56); */
	bite_print_result();

	bite_reset(&bite);
	assert(bite_read(&bite) == 0xF2);
	assert(bite_read(&bite) != 0xAF);

	/* print_binary(bite_mix_u8(0xAA, 4, 0xFF), 8); */

	/* TEST UNDERFLOW */
	bite_config(&bite, 16, 16);
	bite_write(&bite, 0x12);
	bite_print_result();
	bite_reset(&bite);
	assert(bite.flags == BITE_FLAG_UNDERFLOW);

	return 0;
}
