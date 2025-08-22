#include <stdbool.h>
#include <stdint.h>

/* Normal log message */
#ifndef BYTE_LOG
#define BYTE_LOG(s)
#endif

/* Error log message */
#ifndef BYTE_LOGE
#define BYTE_LOGE(s)
#endif

/******************************************************************************
 * CLASS
 *****************************************************************************/
enum bite_order
{
	/* Big endian orders */
	BITE_ORDER_BIG_ENDIAN = -1,
	BITE_ORDER_MOTOROLA   = -1,
	BITE_ORDER_DBC_0      = -1,

	/* Little endian orders */
	BITE_ORDER_LITTLE_ENDIAN = 1,
	BITE_ORDER_LIL_ENDIAN    = 1,
	BITE_ORDER_INTEL         = 1,
	BITE_ORDER_DBC_1         = 1
};

struct bite {
	/* Data buffer */
	uint8_t *buf;

	/* Input/output order (endianness):
	 * 	-1 -> Big-endian    (LSB at high memory address)
	 * 	+1 -> Little-endian (LSB at low  memory address) */
	int8_t  order;

	/* Data start position and length in bits,
	 * considering CAN DBC bit indexing */
	uint8_t start;
	uint8_t len;

	/* Data shift relative to a 8 bit boundary */
	uint8_t lshift;
	uint8_t rshift;
};

void bite_init(struct bite *self, uint8_t *buf,
	       int8_t order, uint8_t start, uint8_t len)
{
	if (len == 0U) {
		BYTE_LOGE("Invalid length");
	}

	self->order = order;
	self->start = start;
	self->len   = len;

	if (order == (int8_t)BITE_ORDER_LIL_ENDIAN) {
		self->buf    = &buf[start / 8U];
		self->lshift =     (start % 8U);
		self->rshift =  8U - self->lshift;
	} else if (order == (int8_t)BITE_ORDER_BIG_ENDIAN) {
		/* CAN DBC reverses bit indexing for big endian
		 * thus it's so complicated... Sorry! ;_; */
		self->buf    = &buf[((start ^ 7U) + len - 1U) / 8U];
		self->rshift =     (((start ^ 7U) + len)      % 8U);
		self->lshift =  (8U - self->rshift)           % 8U;
	} else {
		BYTE_LOGE("Invalid endianness");
	}
}

/* Put byte into buf (little endian) */
void bite_put_u8(struct bite *self, uint8_t data)
{
	uint8_t lshift = self->lshift;
	uint8_t rshift = self->rshift;

	bool byte_aligned     = (lshift == 0U);
	bool less_than_8_bits = (self->len < 8U);
	bool may_carry        = (self->len + lshift) >= 8U;

	if (self->len == 0U) {
		BYTE_LOG("overflow");	
	/* If byte aligned and has more than 8 bits */
	} else if (byte_aligned && !less_than_8_bits) {
		BYTE_LOG("aligned");
		*self->buf  = data;
		 self->buf  = &self->buf[self->order];
		 self->len -= 8U;
	/* Unaligned data that wont fit into a single byte will be carried */
	} else if (may_carry) {
		/* LSB part */
		BYTE_LOG("carry");
		*self->buf &= (0xFFU >> rshift);
		*self->buf |= (uint8_t)(data << lshift);
		 self->len -= rshift;
			 
		/* Advance to the byte we will carry MSB part into */
		self->buf = &self->buf[self->order];

		/* If there are more than 7 bits in stream - carry full MSB */
		if (self->len >= 8U) {
			*self->buf  = (data >> rshift);
			 self->len -= lshift;
		/* Carry partial MSB */	
		} else {
			uint8_t mask = 0xFFU << self->len;

			*self->buf &= mask;
			*self->buf |= (data >> rshift) & (uint8_t)~mask;
			 self->len -= self->len;
		}
	/* Last byte in the stream */
	} else {
		/* Make mask like this: 00011100 */
		uint8_t mask = (0xFFU >> (8U - self->len)) << lshift;

		BYTE_LOG("short");
		*self->buf &= ~mask;
		*self->buf |= (uint8_t)(data << lshift) & mask;
		 self->len  = 0U; /* That's the last byte anyways */
	}
}

/* Get byte from buf */
uint8_t bite_get_u8(struct bite *self)
{
	uint8_t data = 0U;
	uint8_t lshift = self->lshift;
	uint8_t rshift = self->rshift;

	bool byte_aligned     = (lshift == 0U);
	bool less_than_8_bits = (self->len < 8U);
	bool may_carry        = (self->len + lshift) >= 8U;

	if (self->len == 0U) {
		BYTE_LOG("underflow");
		return 0U; // Return 0 or an error indicator
	}

	if (byte_aligned && !less_than_8_bits) {
		BYTE_LOG("aligned read");
		data       = *self->buf;
		self->buf  = &self->buf[self->order];
		self->len -= 8U;
	} else if (may_carry) {
		BYTE_LOG("carry read");
		/* Extract LSB part from current byte */
		data       = (uint8_t)(*self->buf >> lshift);
		self->len -= rshift; /* Bits consumed from the first byte */

		/* Advance to the next byte for MSB part */
		self->buf = &self->buf[self->order];

		/* How many bits do we still need to form the 8-bit data?
		 * (8 - rshift), which is lshift */
		uint8_t bits_needed_from_next_byte = lshift;

		/* If enough bits are available in the next byte */
		if (self->len >= bits_needed_from_next_byte) {
			uint8_t mask_next_byte = (1U << bits_needed_from_next_byte) - 1U; // Mask for the bits to read from the next byte
			data |= (uint8_t)((*self->buf & mask_next_byte) << rshift);
			self->len -= bits_needed_from_next_byte; // Consume the bits read from the second byte
		} else { // Not enough bits for a full 8-bit read, take what's left
			uint8_t mask_next_byte = (1U << self->len) - 1U; // Mask for the remaining bits in the next byte
			data |= (uint8_t)((*self->buf & mask_next_byte) << rshift);
			self->len = 0U; // All remaining bits consumed
		}
	} else { // Last byte in the stream, short read
		// Make mask like this: 00011100 for self->len bits starting at lshift
		uint8_t mask = (0xFFU >> (8U - self->len)) << lshift;
		BYTE_LOG("short read");
		data = (uint8_t)((*self->buf & mask) >> lshift);
		self->len = 0U; // Last bits read
	}

	return data;
}
