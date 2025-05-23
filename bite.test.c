void print_binary(unsigned int value, int bits) {
	int i;

	for (i = bits - 1; i >= 0; i--) {
		putchar((value & (1U << i)) ? '1' : '0');
	}
	putchar('\n');
}

#include "bite.h"
#include <stdio.h>

struct bite bite;
uint8_t buf[8] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

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
	for (i = 0; i < 8; i++) { buf[i] = 0xFF; }
	bite_config(&bite, 6, 2);
	bite_insert(&bite, 0x12);
	bite_insert(&bite, 0x34);
	/* bite_insert(&bite, 0x56); */
	bite_print_result();

	return 0;
}
