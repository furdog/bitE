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

/* High level logic */
struct bite bite_init(uint8_t *buf, int8_t order, uint8_t start, uint8_t len)
{
	struct bite self;

	if (len == 0U) {
		BYTE_LOGE("Invalid length");
	}

	self.order = order;
	self.start = start;
	self.len   = len;

	if (order == (int8_t)BITE_ORDER_LIL_ENDIAN) {
		self.buf    = &buf[start / 8U];
		self.lshift = (start % 8U);
		self.rshift = (8U - self.lshift) % 8U;
	} else if (order == (int8_t)BITE_ORDER_BIG_ENDIAN) {
		self.buf    = &buf[((start ^ 7U) + len - 1U) / 8U];
		self.rshift =     (((start ^ 7U) + len) % 8U);
		self.lshift = (8U - self.rshift) % 8U;
	} else {
		BYTE_LOGE("Invalid endianness");
	}

	return self;
}

void bite_put_u8(struct bite *self, uint8_t data)
{
	uint8_t lshift = self->lshift;
	uint8_t rshift = self->rshift;

	if (self->len == 0U) {
		BYTE_LOG("overflow");
	} else if ((lshift == 0U) && (self->len >= 8U)) { /* Aligned */
		*self->buf  = data;
		 self->buf  = &self->buf[self->order];
		 self->len -= 8U;
		BYTE_LOG("aligned");
	} else if ((self->len + lshift) >= 8U) { /* Carried */
		uint8_t max_carry; /* How many bits we can carry? */
		uint8_t mask;

		/* LSB part */
		 mask = 0xFFU >> rshift;
		*self->buf &= mask;
		*self->buf |= (uint8_t)(data << lshift);
		 self->len -= rshift;
			 
		/* MSB part (carried to the next byte) */
		self->buf = &self->buf[self->order];
		 max_carry = (self->len >= lshift) ? lshift : self->len;
		 mask = 0xFFU << max_carry;
		*self->buf &= mask;
		*self->buf |= (data >> rshift) & (uint8_t)~mask;
		 self->len -= max_carry;
		BYTE_LOG("carry");
	} else { /* Special case (bit range like: 00111100) */
		uint8_t mask = (0xFFU >> (8U - self->len)) << lshift;

		*self->buf &= ~mask;
		*self->buf |= (uint8_t)(data << lshift) & mask;
		 self->len  = 0U; /* That's will be last byte anyways */
		BYTE_LOG("spec");
	}
}

uint8_t bite_get_u8(struct bite *self)
{
	uint8_t data = 0U;

	uint8_t lshift = self->lshift;
	uint8_t rshift = self->rshift;

	if (self->len == 0U) { /* Aligned */
		BYTE_LOG("underflow");
	} else if ((lshift == 0U) && (self->len >= 8U)) {
		data       = *self->buf;
		self->buf  = &self->buf[self->order];
		self->len -= 8U;
		BYTE_LOG("aligned");
	} else if ((self->len + lshift) >= 8U) { /* Carried */
		uint8_t max_carry; /* How many bits we can carry? */
		uint8_t mask;

		/* Extract LSB part from current byte */
		data       = (uint8_t)(*self->buf >> lshift);
		self->len -= rshift; /* Bits consumed from the first byte */

		/* Extract MSB part from the next byte */
		self->buf = &self->buf[self->order];
		max_carry = (self->len >= lshift) ? lshift : self->len;
		mask = 0xFFU << max_carry;
		data |= (*self->buf & (uint8_t)~mask) << rshift;
		self->len -= max_carry;
		BYTE_LOG("carry");
	} else { /* Special case (bit range like: 00111100) */
		uint8_t mask = (0xFFU >> (8U - self->len)) << lshift;
		data = (uint8_t)((*self->buf & mask) >> lshift);
		self->len = 0U; /* That's will be last byte anyways */
		BYTE_LOG("spec");
	}

	return data;
}

int16_t bite_get_i8(struct bite *self)
{
	uint8_t result = 0U;
	uint8_t sign_bit_offset = self->len - 1U;

	result = bite_get_u8(self);

	/* Check sign, if present - invert MSB */
	if ((result & (1U << sign_bit_offset)) > 0U) {
		result |= (0xFFU << sign_bit_offset);
	}

	return result;
}

void bite_put_u16(struct bite *self, uint16_t data)
{
	bite_put_u8(self, data >> 0U);
	bite_put_u8(self, data >> 8U);
}

uint16_t bite_get_u16(struct bite *self)
{
	return ((uint16_t)bite_get_u8(self) << 0U) |
	       ((uint16_t)bite_get_u8(self) << 8U);
}

int16_t bite_get_i16(struct bite *self)
{
	uint16_t result = 0U;
	uint8_t  sign_bit_offset = self->len - 1U;

	result = bite_get_u16(self);

	/* Check sign, if present - invert MSB */
	if ((result & (1U << (uint16_t)sign_bit_offset)) > 0U) {
		result |= (0xFFU << (uint16_t)sign_bit_offset);
	}

	return (int16_t)result;
}

void bite_put_u32(struct bite *self, uint32_t data)
{
	bite_put_u8(self, data >> 0U);
	bite_put_u8(self, data >> 8U);
	bite_put_u8(self, data >> 16U);
	bite_put_u8(self, data >> 24U);
}

uint32_t bite_get_u32(struct bite *self)
{
	return ((uint32_t)bite_get_u8(self) << 0U)  |
	       ((uint32_t)bite_get_u8(self) << 8U)  |
	       ((uint32_t)bite_get_u8(self) << 16U) |
	       ((uint32_t)bite_get_u8(self) << 24U);
}

int32_t bite_get_i32(struct bite *self)
{
	uint32_t result = 0U;
	uint8_t  sign_bit_offset = self->len - 1U;

	result = bite_get_u32(self);

	/* Check sign, if present - invert MSB */
	if ((result & (1U << (uint32_t)sign_bit_offset)) > 0U) {
		result |= (0xFFU << (uint32_t)sign_bit_offset);
	}

	return (int32_t)result;
}
