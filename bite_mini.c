#include <assert.h>
#include <string.h>
#include <stdio.h>

#define BYTE_LOG(s)  printf("%s\n", s)
#define BYTE_LOGE(s) printf("%s\n", s)

#include "bite_mini.h"

void print_bits(uint8_t *arr, uint8_t size)
{
	uint8_t i;
	int8_t  j;

	for (i = 0U; i < size; i++) {
		for (j = 7; j >= 0; j--) { 
			printf("%d", (arr[i] >> j) & 1U);
		}
		printf("|%02X ", arr[i]);
	}
	printf("\n");
}

int main(void)
{
	/*uint16_t voltage_V  = 1337U;*/
	/*uint32_t voltage_V  = 0x3377BBFFU;*/
	uint32_t voltage_V  = 0xFFAABBFFU;
	uint32_t voltage_V2 = 0U;
	uint8_t candata[8U] = {0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U};
	struct bite b;

	memset(candata, 0U, 8U);
	bite_init(&b, candata, BITE_ORDER_LIL_ENDIAN, 3U, 31U);
	bite_put_u32(&b, voltage_V);
	print_bits(candata, 8U);

	bite_init(&b, candata, BITE_ORDER_LIL_ENDIAN, 3U, 31U);
	voltage_V2 = bite_get_u32(&b);
	printf("Read is: %08X\n\n", voltage_V2);
	assert(voltage_V2 == 0x7FAABBFFU);

	memset(candata, 0U, 8U);
	bite_init(&b, candata, BITE_ORDER_BIG_ENDIAN, 6U, 28U);
	bite_put_u32(&b, voltage_V);
	print_bits(candata, 8U);

	bite_init(&b, candata, BITE_ORDER_BIG_ENDIAN, 6U, 28U);
	voltage_V2 = bite_get_u32(&b);
	printf("Read is: %08X\n\n", voltage_V2);
	assert(voltage_V2 == 0x0FAABBFFU);

	return 0;
}
