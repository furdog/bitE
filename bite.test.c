#include <assert.h>
#include <string.h>
#include <stdio.h>

/* #define BYTE_LOG(s)  printf("%s\n", s) */
#define BYTE_LOGE(s) printf("%s\n", s)

#include "bite.h"

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

/* Test if bit bit positions correspond to CAN DBC data format */
void bite_test_bit_positions_are_correct()
{
	uint8_t i;
	uint8_t j;

	struct bite b;
	uint8_t buf[8U];

	for (i = 0U; i <= 8U; i++) {
		memset(buf, 0U, 8U);
		b = bite_init(buf, BITE_ORDER_LIL_ENDIAN, i, 16U);
		bite_put_u16(&b, 0xFFFFU);
		assert(buf[0U] == (uint8_t)(0xFFU << i));
		assert(buf[1U] == 0xFFU);
		assert(buf[2U] == ((i == 0U) ? 0U : (0xFFU >> (8U - i))));
		printf("bit_idx = %02u | ", i); print_bits(buf, 3U);
	}

	for (j = 0U; j <= 8U; j++) {
		i = (j ^ 7U);

		memset(buf, 0U, 8U);
		b = bite_init(buf, BITE_ORDER_BIG_ENDIAN, i, 16U);
		bite_put_u16(&b, 0xFFFFU);
		assert(buf[0U] == (uint8_t)((j == 8U) ? 0x00U : (0xFFU >> j)));
		assert(buf[1U] == 0xFFU);
		assert(buf[2U] == (uint8_t)((j == 0U) ? 0U :
						       (0xFFU << (8U - j))));
		printf("bit_idx = %02u | ", i); print_bits(buf, 3U);
	}
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
	b = bite_init(candata, BITE_ORDER_LIL_ENDIAN, 3U, 31U);
	bite_put_u32(&b, voltage_V);
	print_bits(candata, 8U);

	b = bite_init(candata, BITE_ORDER_LIL_ENDIAN, 3U, 31U);
	voltage_V2 = bite_get_u32(&b);
	printf("Read is: %08X\n\n", voltage_V2);
	assert(voltage_V2 == 0x7FAABBFFU);

	memset(candata, 0U, 8U);
	b = bite_init(candata, BITE_ORDER_BIG_ENDIAN, 6U, 28U);
	bite_put_u32(&b, voltage_V);
	print_bits(candata, 8U);

	b = bite_init(candata, BITE_ORDER_BIG_ENDIAN, 6U, 28U);
	voltage_V2 = bite_get_u32(&b);
	printf("Read is: %08X\n\n", voltage_V2);
	assert(voltage_V2 == 0x0FAABBFFU);

	bite_test_bit_positions_are_correct();

	return 0;
}
