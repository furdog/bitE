#include <stdbool.h>
#include <stdint.h>

/* Normal log message */
#ifndef BITE_LOG
#define BITE_LOG(s)
#endif

/* Error log message */
#ifndef BITE_LOGE
#define BITE_LOGE(s)
#endif

/******************************************************************************
 * CLASS
 *****************************************************************************/
enum bite_order {
	/* Big endian orders */
	BITE_ORDER_BIG_ENDIAN = -1,
	BITE_ORDER_MOTOROLA   = -1,
	BITE_ORDER_DBC_0      = -1,

	/* Little endian orders */
	BITE_ORDER_LITTLE_ENDIAN = 1,
	BITE_ORDER_LIL_ENDIAN	 = 1,
	BITE_ORDER_INTEL	 = 1,
	BITE_ORDER_DBC_1	 = 1
};

struct bite {
	/* Original Data buffer */
	/* uint8_t *ori; */

	/* Data buffer */
	uint8_t *buf;

	/** Physical data order within buf (endianness):
	 * 	-1 -> Big-endian    (low byte at high memory address)
	 * 	+1 -> Little-endian (low byte at low  memory address) */
	int8_t order;

	/* Data start position and length in bits,
	 * considering CAN DBC bit indexing */
	uint8_t start;
	uint8_t len;

	/* LSB and MSB masks are used in case if original data
	 * has a binary offset within a payload. For example if the data is
	 * 3 bits shifted to the left:
	 *  
	 * msb_mask_len == 5    3 == lsb_mask_len
	 *                  \  /
	 *                   ||
	 *        (MSB)|11111||000|(LSB)
	 **/
	uint8_t lsb_mask_len;
	uint8_t msb_mask_len;
};

/** Initializes and returns bite instance.
 *  Takes uint8_t buffer, format, start bit position and data length (in bits).
 **/
struct bite bite_init(uint8_t *buf, int8_t order, uint8_t start, uint8_t len)
{
	struct bite self;

	if (len == 0U) {
		BITE_LOGE("Invalid length");
	}

	/* self.ori   = buf; */
	self.order = order;
	self.start = start;
	self.len   = len;

	if (order == (int8_t)BITE_ORDER_LIL_ENDIAN) {
		/* LOW byte goes first. For example:
		 * payload: 0xFF 0xFF ... 0xFF 0xFF 0xFF
		 *            ||                      ||
		 *            LOW --(r/w order +1)--> HIGH */
		self.buf    = &buf[start / 8U];
		self.lsb_mask_len = (start % 8U);
		self.msb_mask_len = (8U - self.lsb_mask_len) % 8U;
	} else if (order == (int8_t)BITE_ORDER_BIG_ENDIAN) {
		/* LOW byte goes last. For example:
		 * payload: 0xFF 0xFF ... 0xFF 0xFF 0xFF
		 *            ||                      ||
		 *           HIGH <--(r/w order -1)-- LOW */
		self.buf    = &buf[((start ^ 7U) + len - 1U) / 8U];
		self.msb_mask_len = (((start ^ 7U) + len) % 8U);
		self.lsb_mask_len = (8U - self.msb_mask_len) % 8U;
	} else {
		BITE_LOGE("Invalid endianness");
	}

	return self;
}

/** Puts at least 8 bit of data into self->buf, until fills all the bits
 *  according to self->len.
 * 
 *  The first byte is considered LOW and is pushed straight or in reverse
 *  order, according to selected endianness (r/w order) */
void bite_put_u8(struct bite *self, uint8_t data)
{
	uint8_t lml = self->lsb_mask_len;
	uint8_t mml = self->msb_mask_len;

	if (self->len == 0U) {
		BITE_LOG("overflow");
	} else if ((lml == 0U) && (self->len >= 8U)) { /* Aligned */
		/* If lsb mask is zero, it means that data is not shifted
		 * inside payload. We can safely read the payload and map it
		 * directly to data. The only exception if there are less
		 * than 8 bits of length remaining */

		*self->buf  = data;
		self->buf   = &self->buf[self->order];
		self->len  -= 8U;
		BITE_LOG("aligned");
	} else if ((self->len + lml) >= 8U) { /* Carried */
		/* This condition checks if data occupies more than 2
		 * bytes(where MSB is carried) within payload. In this case we
		 * read/write LSB from/to buf[0] and MSB from/to buf[1],
		 * respecting the endian order. */

		uint8_t max_carry; /* How many bits we can carry? */
		uint8_t mask;

		/* Masking and writing LSB part to buffer[1] */
		mask	    = 0xFFU >> mml;
		*self->buf &= mask;
		*self->buf |= (uint8_t)(data << lml);
		self->len  -= mml;

		/* assert((self->order == 1) || (self->buf > self->ori)); */

		/* Masking and writing MSB part to buffer[0] */
		self->buf   = &self->buf[self->order];

		/* If this is a last byte, MSB may be shorter than LSB mask */
		max_carry   = (self->len >= lml) ? lml : self->len;
		mask	    = 0xFFU << max_carry;

		*self->buf &= mask;
		*self->buf |= (data >> mml) & (uint8_t)~mask;
		self->len  -= max_carry;
		BITE_LOG("carry");
	} else { /* Special case (sub-byte range like: 00111100) */
		uint8_t mask = (0xFFU >> (8U - self->len)) << lml;

		*self->buf &= ~mask;
		*self->buf |= (uint8_t)(data << lml) & mask;
		self->len   = 0U; /* That's will be last byte anyways */
		BITE_LOG("spec");
	}
}

/** Reads at least 8 bit from data into self->buf, until all self->len bits
 *  are read. If there are less than 8 bits available, the result MSB will be
 *  masked with zeros.
 * 
 *  The first byte is considered LOW and is popped straight or in reverse
 *  order, according to selected endianness (r/w order) */
uint8_t bite_get_u8(struct bite *self)
{
	uint8_t data = 0U;

	uint8_t lml = self->lsb_mask_len;
	uint8_t mml = self->msb_mask_len;

	if (self->len == 0U) { /* Aligned */
		BITE_LOG("underflow");
	} else if ((lml == 0U) && (self->len >= 8U)) {
		data	   = *self->buf;
		self->buf  = &self->buf[self->order];
		self->len -= 8U;
		BITE_LOG("aligned");
	} else if ((self->len + lml) >= 8U) { /* Carried */
		uint8_t max_carry; /* How many bits we can carry? */
		uint8_t mask;

		/* Extract LSB part from current byte */
		data	   = (uint8_t)(*self->buf >> lml);
		self->len -= mml; /* Bits consumed from the first byte */

		/* assert((self->order == 1) || (self->buf > self->ori)); */

		/* Extract MSB part from the next byte */
		self->buf  = &self->buf[self->order];
		max_carry  = (self->len >= lml) ? lml : self->len;
		mask	   = 0xFFU << max_carry;
		data	  |= (*self->buf & (uint8_t)~mask) << mml;
		self->len -= max_carry;
		BITE_LOG("carry");
	} else { /* Special case (bit range like: 00111100) */
		uint8_t mask = (0xFFU >> (8U - self->len)) << lml;
		data	     = (uint8_t)((*self->buf & mask) >> lml);
		self->len    = 0U; /* That's will be last byte anyways */
		BITE_LOG("spec");
	}

	return data;
}

int16_t bite_get_i8(struct bite *self)
{
	uint8_t result		= 0U;
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
	uint16_t result		 = 0U;
	uint8_t	 sign_bit_offset = self->len - 1U;

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
	return ((uint32_t)bite_get_u8(self) << 0U) |
	       ((uint32_t)bite_get_u8(self) << 8U) |
	       ((uint32_t)bite_get_u8(self) << 16U) |
	       ((uint32_t)bite_get_u8(self) << 24U);
}

int32_t bite_get_i32(struct bite *self)
{
	uint32_t result		 = 0U;
	uint8_t	 sign_bit_offset = self->len - 1U;

	result = bite_get_u32(self);

	/* Check sign, if present - invert MSB */
	if ((result & (1U << (uint32_t)sign_bit_offset)) > 0U) {
		result |= (0xFFU << (uint32_t)sign_bit_offset);
	}

	return (int32_t)result;
}
