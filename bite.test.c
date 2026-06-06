#include <assert.h>
#include <stdio.h>
#include <string.h>

#define BITE_LOG(s) printf("%s\n", s)
#define BITE_LOGE(s) printf("%s\n", s)
#define BITE_CGEN(s) printf s
#define BITE_IMPLEMENTATION
#define BITE_SIG_IMPLEMENTATION
#define BITE_C_GEN_IMPLEMENTATION

#include "bite.h"

void print_bits(uint8_t *arr, uint8_t size)
{
	uint8_t i;
	int8_t	j;

	for (i = 0U; i < size; i++) {
		for (j = 7; j >= 0; j--) {
			printf("%d", (arr[i] >> j) & 1U);
		}
		printf("|%02X ", arr[i]);
	}
	printf("\n");
}

void bite_test_cgen()
{
	struct bite_cgen   b;
	struct bite_signal s;

	b.cpu_wordlen	= 1u;
	b.cpu_endianess = -1;

	bite_sig_init(&s);
	s.label = "mysig";
	s.order = BITE_ORDER_LIL_ENDIAN;
	s.start = 0;
	s.len	= 23;

	bite_cgen_sig(&b, &s);
}

/* Test if bit bit positions correspond to CAN DBC data format */
void bite_test_bit_positions_are_correct()
{
	uint8_t i;
	uint8_t j;
	uint8_t buf[8U];

	struct bite	   b;
	struct bite_signal s;

	bite_init(&b, buf, 8u);
	bite_sig_init(&s);

	for (i = 0U; i <= 8U; i++) {
		memset(buf, 0U, 8U);
		s.order = BITE_ORDER_LIL_ENDIAN;
		s.start = i;
		s.len	= 16U;
		bite_set_sig(&b, &s);

		bite_put_u16(&b, 0xFFFFU);
		assert(buf[0U] == (uint8_t)(0xFFU << i));
		assert(buf[1U] == 0xFFU);
		assert(buf[2U] == ((i == 0U) ? 0U : (0xFFU >> (8U - i))));
		printf("bit_idx = %02u | ", i);
		print_bits(buf, 3U);
	}

	for (j = 0U; j <= 8U; j++) {
		i = (j ^ 7U);

		memset(buf, 0U, 8U);

		s.order = BITE_ORDER_BIG_ENDIAN;
		s.start = i;
		s.len	= 16U;
		bite_set_sig(&b, &s);

		bite_put_u16(&b, 0xFFFFU);
		assert(buf[0U] == (uint8_t)((j == 8U) ? 0x00U : (0xFFU >> j)));
		assert(buf[1U] == 0xFFU);
		assert(buf[2U] ==
		       (uint8_t)((j == 0U) ? 0U : (0xFFU << (8U - j))));
		printf("bit_idx = %02u | ", i);
		print_bits(buf, 3U);
	}
}

int main(void)
{
	/*uint16_t voltage_V  = 1337U;*/
	/*uint32_t voltage_V  = 0x3377BBFFU;*/
	uint32_t	   voltage_V   = 0xFFAABBFFU;
	uint32_t	   voltage_V2  = 0U;
	uint8_t		   candata[8U] = {0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U};
	struct bite	   b;
	struct bite_signal s;

	bite_init(&b, candata, 8u);
	bite_sig_init(&s);

	memset(candata, 0U, 8U);

	s.order = BITE_ORDER_LIL_ENDIAN;
	s.start = 3u;
	s.len	= 31U;
	bite_set_sig(&b, &s);
	bite_put_u32(&b, voltage_V);
	print_bits(candata, 8U);

	bite_set_sig(&b, &s);
	voltage_V2 = bite_get_u32(&b);
	printf("Read is: %08X\n\n", voltage_V2);
	assert(voltage_V2 == 0x7FAABBFFU);

	memset(candata, 0U, 8U);
	s.order = BITE_ORDER_BIG_ENDIAN;
	s.start = 6u;
	s.len	= 28U;
	bite_set_sig(&b, &s);
	bite_put_u32(&b, voltage_V);
	print_bits(candata, 8U);

	bite_set_sig(&b, &s);
	voltage_V2 = bite_get_u32(&b);
	printf("Read is: %08X\n\n", voltage_V2);
	assert(voltage_V2 == 0x0FAABBFFU);

	bite_test_bit_positions_are_correct();

	bite_test_cgen();

	return 0;
}
