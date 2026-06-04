/**
 * @file bite.h
 * @brief bit stream serialization tool (Hardware-Agnostic)
 *
 * **Conventions:**
 * C89, Linux kernel style, MISRA, rule of 10, No hardware specific code,
 * only generic C and some binding layer. Be extra specific about types.
 *
 * Scientific units where posible at end of the names, for example:
 * - timer_10s (timer_10s has a resolution of 10s per bit)
 * - power_150w (power 150W per bit or 0.15kw per bit)
 *
 * Keep variables without units if they're unknown or not specified or hard
 * to define with short notation.
 *
 * ```LICENSE
 * Copyright (c) 2025 furdog <https://github.com/furdog>
 *
 * SPDX-License-Identifier: 0BSD
 * ```
 *
 * Be free, be wise and take care of yourself!
 * With best wishes and respect, furdog
 */

#ifndef BITE_HEADER_GUARD
#define BITE_HEADER_GUARD

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

/** Order of operation. Selected by user. */
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

/** Main instance. Is used to store intermediate and setup data,
 *  while various bite calls are performed */
struct bite {
	uint8_t *buf; /**< Data buffer */
	uint8_t  cap; /**< Data buffer capacity */

	int8_t	 idx; /**< Data index */

	/** Physical data order within buf (endianness):
	 * 	-1 -> Big-endian    (low byte at high memory address)
	 * 	+1 -> Little-endian (low byte at low  memory address) */
	int8_t order;

	uint8_t start; /**< Data start position (in bits) */
	uint8_t len;   /**< Data length (in bits) */

	/* LSB and MSB masks are used in case if original data
	 * has a binary offset within a payload. For example if the data is
	 * 3 bits shifted to the left:
	 *
	 * msb_mask_len == 5    3 == lsb_mask_len
	 *                  \  /
	 *                   ||
	 *        (MSB)|11111||000|(LSB)
	 **/
	uint8_t lsb_mask_len; /**< Mask used to read/write LSB part */
	uint8_t msb_mask_len; /**< Mask used to read/write MSB part */

	/* Flag that catches overflows */
	bool overflow;
};

/** Initializes and returns bite instance.
 *  Takes uint8_t buffer, its capacity, format, start bit position and data length (in bits).
 **/
struct bite bite_init(uint8_t *buf, const uint8_t cap, const int8_t order, const uint8_t start,
		      const uint8_t len);

/** Puts at least 8 bit of data into self->buf, until fills all the bits
 *  according to self->len.
 *
 *  The first byte is considered LOW and is pushed straight or in reverse
 *  order, according to selected endianness (r/w order) */
void bite_put_u8(struct bite *self, const uint8_t data);

/** Reads at least 8 bit from data into self->buf, until all self->len bits
 *  are read. If there are less than 8 bits available, the result MSB will be
 *  masked with zeros.
 *
 *  The first byte is considered LOW and is popped straight or in reverse
 *  order, according to selected endianness (r/w order) */
uint8_t bite_get_u8(struct bite *self);

/** Same as bite_get_u8, but last MSB bit is considered a sign bit */
int16_t bite_get_i8(struct bite *self);

/** Two bite_put_u8 operation in a row, resulting in 16bit value or less. */
void bite_put_u16(struct bite *self, const uint16_t data);

/** Two bite_get_u8 operation in a row, resulting in 16bit value or less. */
uint16_t bite_get_u16(struct bite *self);

/** Two bite_get_u8 operation in a row, resulting in 16bit value or less.
 *  The last MSB bit is considered a sign bit */
int16_t bite_get_i16(struct bite *self);

/** Four bite_put_u8 operation in a row, resulting in 32bit value or less. */
void bite_put_u32(struct bite *self, const uint32_t data);

/** Four bite_get_u8 operation in a row, resulting in 32bit value or less. */
uint32_t bite_get_u32(struct bite *self);

/** Four bite_get_u8 operation in a row, resulting in 32bit value or less.
 *  The last MSB bit is considered a sign bit */
int32_t bite_get_i32(struct bite *self);

#ifdef BITE_IMPLEMENTATION
struct bite bite_init(uint8_t *buf, const uint8_t cap, const int8_t order, const uint8_t start,
		      const uint8_t len)
{
	struct bite self;

	if ((cap == 0u) && (len == 0u)) {
		BITE_LOGE("Invalid capacity or length");
	}

	self.buf   = buf;
	self.cap   = cap;
	self.order = order;
	self.start = start;
	self.len   = len;

	if (order == (int8_t)BITE_ORDER_LIL_ENDIAN) {
		/* LOW byte goes first. For example:
		 * payload: 0xFF 0xFF ... 0xFF 0xFF 0xFF
		 *            ||                      ||
		 *            LOW --(r/w order +1)--> HIGH */
		self.idx	  = start / 8u;
		self.lsb_mask_len = (start % 8u);
		self.msb_mask_len = (8u - self.lsb_mask_len) % 8u;
	} else if (order == (int8_t)BITE_ORDER_BIG_ENDIAN) {
		/* LOW byte goes last. For example:
		 * payload: 0xFF 0xFF ... 0xFF 0xFF 0xFF
		 *            ||                      ||
		 *           HIGH <--(r/w order -1)-- LOW */
		self.idx	  = ((start ^ 7u) + len - 1u) / 8u;
		self.msb_mask_len = (((start ^ 7u) + len) % 8u);
		self.lsb_mask_len = (8u - self.msb_mask_len) % 8u;
	} else {
		BITE_LOGE("Invalid endianness");
		self.len = 0u;
	}

	self.overflow = false;

	return self;
}

/** @private
 *  Returns pointer to currently indexed data in buffer */
uint8_t *_bite_peek(struct bite *self)
{
	return &self->buf[self->idx];
}

/** @private
 *  Iterate to the next byte in buffer.
 *  Will either decrement or increment buffer index (endianness specific) */
void _bite_next(struct bite *self) {
	self->idx += self->order;

	/* Handle overflows softly... */
	if (self->idx < 0) {
		self->idx = 0;
		self->overflow = true;
	} else if (self->idx > self->cap) {
		self->idx = self->cap;
		self->overflow = true;
	}

	/* TEST TODO */
	/* assert(!self->overflow); */
}

void bite_put_u8(struct bite *self, const uint8_t input)
{
	uint8_t lml = self->lsb_mask_len;
	uint8_t mml = self->msb_mask_len;

	if (self->len == 0u) {
		BITE_LOG("overflow");
	} else if ((lml == 0u) && (self->len >= 8u)) { /* Aligned */
		/* If lsb mask is zero, it means that input is not shifted
		 * inside payload. We can safely read the payload and map it
		 * directly to input. The only exception if there are less
		 * than 8 bits of length remaining */

		*_bite_peek(self) = input;
		_bite_next(self);
		self->len -= 8u;
		BITE_LOG("aligned");
	} else if ((self->len + lml) >= 8u) { /* Carried */
		/* This condition checks if input occupies more than 2
		 * bytes(where MSB is carried) within payload. In this case we
		 * read/write LSB from/to buf[0] and MSB from/to buf[1],
		 * respecting the endian order. */

		uint8_t max_carry; /* How many bits we can carry? */
		uint8_t mask;

		/* Masking and writing LSB part to buffer[0] */
		mask		   = 0xFFu >> mml;
		*_bite_peek(self) &= mask;
		*_bite_peek(self) |= (uint8_t)(input << lml);
		self->len	  -= mml;

		/* Masking and writing MSB part to buffer[1] */
		_bite_next(self);

		/* If this is a last byte, MSB may be shorter than LSB mask */
		max_carry = (self->len >= lml) ? lml : self->len;
		mask	  = 0xFFu << max_carry;

		*_bite_peek(self) &= mask;
		*_bite_peek(self) |= (input >> mml) & (uint8_t)~mask;
		self->len	  -= max_carry;
		BITE_LOG("carry");
	} else { /* Special case (sub-byte range like: 00111100) */
		uint8_t mask = (0xFFu >> (8u - self->len)) << lml;

		*_bite_peek(self) &= ~mask;
		*_bite_peek(self) |= (uint8_t)(input << lml) & mask;
		self->len	   = 0u; /* That's will be last byte anyways */
		BITE_LOG("spec");
	}
}

uint8_t bite_get_u8(struct bite *self)
{
	/* The code here is basically the same as in bite_put_u8.
	 * It just does thing the opposite way. */

	uint8_t output = 0u;

	uint8_t lml = self->lsb_mask_len;
	uint8_t mml = self->msb_mask_len;

	if (self->len == 0u) { /* Aligned */
		BITE_LOG("underflow");
	} else if ((lml == 0u) && (self->len >= 8u)) {
		output = *_bite_peek(self);
		_bite_next(self);
		self->len -= 8u;
		BITE_LOG("aligned");
	} else if ((self->len + lml) >= 8u) { /* Carried */
		uint8_t max_carry;	      /* How many bits we can carry? */
		uint8_t mask;

		/* Extract LSB part from current byte */
		output	   = (uint8_t)(*_bite_peek(self) >> lml);
		self->len -= mml; /* Bits consumed from the first byte */

		/* Extract MSB part from the next byte */
		_bite_next(self);

		/* If this is a last byte, MSB may be shorter than LSB mask */
		max_carry  = (self->len >= lml) ? lml : self->len;
		mask	   = 0xFFu << max_carry;
		output	  |= (*_bite_peek(self) & (uint8_t)~mask) << mml;
		self->len -= max_carry;
		BITE_LOG("carry");
	} else { /* Special case (bit range like: 00111100) */
		uint8_t mask = (0xFFu >> (8u - self->len)) << lml;
		output	     = (uint8_t)((*_bite_peek(self) & mask) >> lml);
		self->len    = 0u; /* That's will be last byte anyways */
		BITE_LOG("spec");
	}

	return output;
}

int16_t bite_get_i8(struct bite *self)
{
	uint8_t result		= 0u;
	uint8_t sign_bit_offset = self->len - 1u;

	result = bite_get_u8(self);

	/* Check sign, if present - invert MSB part of the result */
	if ((result & (1u << sign_bit_offset)) > 0u) {
		result |= (0xFFu << sign_bit_offset);
	}

	return result;
}

void bite_put_u16(struct bite *self, const uint16_t data)
{
	bite_put_u8(self, data >> 0u);
	bite_put_u8(self, data >> 8u);
}

uint16_t bite_get_u16(struct bite *self)
{
	return ((uint16_t)bite_get_u8(self) << 0u) |
	       ((uint16_t)bite_get_u8(self) << 8u);
}

int16_t bite_get_i16(struct bite *self)
{
	uint16_t result		 = 0u;
	uint8_t	 sign_bit_offset = self->len - 1u;

	result = bite_get_u16(self);

	/* Check sign, if present - invert MSB part of the result */
	if ((result & (1u << (uint16_t)sign_bit_offset)) > 0u) {
		result |= (0xFFFFu << (uint16_t)sign_bit_offset);
	}

	return (int16_t)result;
}

void bite_put_u32(struct bite *self, const uint32_t data)
{
	bite_put_u8(self, data >> 0u);
	bite_put_u8(self, data >> 8u);
	bite_put_u8(self, data >> 16u);
	bite_put_u8(self, data >> 24u);
}

uint32_t bite_get_u32(struct bite *self)
{
	return ((uint32_t)bite_get_u8(self) << 0u) |
	       ((uint32_t)bite_get_u8(self) << 8u) |
	       ((uint32_t)bite_get_u8(self) << 16u) |
	       ((uint32_t)bite_get_u8(self) << 24u);
}

int32_t bite_get_i32(struct bite *self)
{
	uint32_t result		 = 0u;
	uint8_t	 sign_bit_offset = self->len - 1u;

	result = bite_get_u32(self);

	/* Check sign, if present - invert MSB part of the result */
	if ((result & (1u << (uint32_t)sign_bit_offset)) > 0u) {
		result |= (0xFFFFFFFFu << (uint32_t)sign_bit_offset);
	}

	return (int32_t)result;
}
#endif /* BITE_IMPLEMENTATION */

#endif /* BITE_HEADER_GUARD */
