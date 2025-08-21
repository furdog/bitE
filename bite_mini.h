#include <stdbool.h>
#include <stdint.h>
#include <assert.h>

struct bite {
	uint8_t *buf;

	uint8_t start;
	uint8_t len;
};

void bite_init_le(struct bite *self, uint8_t *buf, uint8_t start, uint8_t len)
{
	self->buf = &buf[start / 8U];

	self->start = start;
	self->len   = len;
}

/* Put byte into buf (little endian) */
void bite_put_le(struct bite *self, uint8_t data)
{
	uint8_t lshift = (self->start % 8U);
	uint8_t rshift =  8U - lshift;

	bool byte_aligned     = (lshift == 0U);
	bool less_than_8_bits = (self->len < 8U);
	bool may_carry        = (self->len + lshift) >= 8U;

	assert(self->len != 0U);

	if (byte_aligned && !less_than_8_bits) {
		*self->buf  = data;
		 self->buf++;
		 self->len -= 8U;
	} else if (may_carry) {
		/* LSB part */
		*self->buf &= (0xFFU >> rshift);
		*self->buf |= (uint8_t)(data << lshift);
		 self->len -= rshift;
			 
		/* Advance to the byte we will carry MSB part into into */
		self->buf++;

		/* If there are more than 7 bits in stream - carry full MSB */
		if (self->len >= 8U) {
			*self->buf = (data >> rshift);
			 self->len -= lshift;
		} else {
		/* Carry partial MSB */
			uint8_t mask = 0xFFU << self->len;

			*self->buf &= mask;
			*self->buf |= (data >> rshift) & (uint8_t)~mask;
			 self->len -= self->len;
		}
	} else {
		/* Make mask like this: 00011100 */
		uint8_t mask = (0xFFU >> (8U - self->len)) << lshift;

		*self->buf &= ~mask;
		*self->buf |= (uint8_t)(data << lshift) & mask;
		 self->len  = 0U; /* That's the last byte anyways */
	}
}
