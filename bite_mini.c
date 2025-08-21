#include <stdio.h>
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
	uint32_t voltage_V  = 0xFFBBCCDDU;
	uint8_t candata[8U] = {0U};
	struct bite b;

	b.buf   = candata;
	b.len   = 31U;
	b.start = 3U;
	b.carry = 0U;

	bite_set_base_le(&b);
	bite_put_le( &b, (uint8_t)(voltage_V >> 0U));
	bite_put_lec(&b, (uint8_t)(voltage_V >> 8U));
	bite_put_lec(&b, (uint8_t)(voltage_V >> 16U));
	bite_put_lec(&b, (uint8_t)(voltage_V >> 24U));
	bite_put_lec(&b, 0);
	print_bits(candata, 8U);

	return 0;
}
