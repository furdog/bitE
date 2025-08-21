#include <stdint.h>

struct bite {
	uint8_t *buf;

	uint8_t start;
	uint8_t len;

	uint8_t carry;
};

/* Set base (little endian) */
void bite_set_base_le(struct bite *self)
{
	self->buf = &self->buf[self->start / 8U];
}

/* Put byte into buf (little endian) */
void bite_put_le(struct bite *self, uint8_t data)
{
	uint8_t lshift = (self->start % 8U);
	uint8_t rshift =  8U - lshift;

	*self->buf &= (0xFFU >> rshift);
	*self->buf |= (uint8_t)(data << lshift);

	self->carry = data >> rshift;

	self->len -= 8U;
}

/* Put byte into buf with carry (little endian) */
void bite_put_lec(struct bite *self, uint8_t data)
{
	uint8_t lshift = (self->start % 8U);
	uint8_t rshift =  8U - lshift;

	uint8_t result = self->carry | (uint8_t)(data << lshift);

	/* Goto next byte */
	self->buf++;

	if (self->len >= 8U) {
		printf("a: %u\n",self->len);
		/* Write all 8 bits */
		*self->buf = result;
		self->len -= 8U;
	} else if (self->len + lshift >= 8U) {
		printf("b: %u\n",self->len);

		*self->buf = result;
		 self->len -= rshift;
	} else {
		printf("c: %u\n",self->len);

	//TODO FIXME
		/* Write self->len bits */
		self->carry = data >> rshift;
		*self->buf  = result & (0xFFU >> self->len));
	}

	self->carry = data >> rshift;
}
