
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
	int i;

	/* Trigger UBsan
	 * volatile uint8_t x = 0xFF;
	   volatile uint8_t y = 32;
	   x >> y; */

	bite_init(&bite, buf/*, 8*/);
	
	bite_config(&bite, 16, 16);

	/* TEST NORMAL */
	bite_insert(&bite, 0x12);
	bite_insert(&bite, 0x34);
	bite_print_result();
	assert(bite.flags == 0);

	/* TEST OVERFLOW */
	bite_insert(&bite, 0x56);
	bite_print_result();
	assert(bite.flags != 0);

	/* TEST MISALIGNED */
	for (i = 0; i < 8; i++) { buf[i] = 0x00; }
	bite_config(&bite, 5, 2);
	bite_insert(&bite, 0xFF);
	bite_insert(&bite, 0xFF);
	/* bite_insert(&bite, 0x56); */
	bite_print_result();

	print_binary(bite_mix_u8(0xAA, 4, 0xFF), 8);

	return 0;
}
