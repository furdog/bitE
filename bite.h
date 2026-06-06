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

#ifndef BITE_LOG
#define BITE_LOG(s) /**< Normal log message */
#endif

#ifndef BITE_LOGE
#define BITE_LOGE(s) /**< Error log message */
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
	uint8_t	 cap; /**< Data buffer capacity */

	int8_t idx; /**< Data index */

	/** Physical data order within buf (endianness):
	 * 	-1 -> Big-endian    (low byte at high memory address)
	 * 	+1 -> Little-endian (low byte at low  memory address) */
	int8_t order;

	uint8_t rem; /**< Data length remaining (in bits) */

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
};

/** Initializes bite instance.
 *  Takes uint8_t buffer and its capacity. */
void bite_init(struct bite *self, uint8_t *buf, const uint8_t capacity);

/** Minimal setup to read/write bit range from/to buffer */
bool bite_config(struct bite *self, const int8_t order, const uint8_t start,
		 const uint8_t len);

/** Puts at least 8 bit of data into self->buf, until fills all the bits
 *  according to self->rem.
 *
 *  The first byte is considered LOW and is pushed straight or in reverse
 *  order, according to selected endianness (r/w order) */
void bite_put_u8(struct bite *self, const uint8_t data);

/** Reads at least 8 bit from data into self->buf, until all self->rem bits
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
void bite_init(struct bite *self, uint8_t *buf, const uint8_t capacity)
{
	assert(self && buf);

	self->buf = buf;
	self->cap = capacity;

	self->order = 0;
	self->rem   = 0u;

	self->lsb_mask_len = 0u;
	self->msb_mask_len = 0u;
}

bool bite_config(struct bite *self, const int8_t order, const uint8_t start,
		 const uint8_t len)
{
	bool success = true;

	assert(self);

	self->order = order;
	self->rem   = len;

	if (order == (int8_t)BITE_ORDER_LIL_ENDIAN) {
		/* LOW byte goes first. For example:
		 * payload: 0xFF 0xFF ... 0xFF 0xFF 0xFF
		 *            ||                      ||
		 *            LOW --(r/w order +1)--> HIGH */
		self->idx	   = start / 8u;
		self->lsb_mask_len = (start % 8u);
		self->msb_mask_len = (8u - self->lsb_mask_len) % 8u;
	} else if (order == (int8_t)BITE_ORDER_BIG_ENDIAN) {
		/* LOW byte goes last. For example:
		 * payload: 0xFF 0xFF ... 0xFF 0xFF 0xFF
		 *            ||                      ||
		 *           HIGH <--(r/w order -1)-- LOW */
		self->idx	   = ((start ^ 7u) + len - 1u) / 8u;
		self->msb_mask_len = (((start ^ 7u) + len) % 8u);
		self->lsb_mask_len = (8u - self->msb_mask_len) % 8u;
		self->order	   = order;
	} else {
		BITE_LOGE("Invalid endianness");
		self->rem = 0u;
		success	  = false;
	}

	return success;
}

/** @private
 *  Returns pointer to currently indexed data in buffer */
uint8_t *_bite_peek(struct bite *self) { return &self->buf[self->idx]; }

/** @private
 *  Iterate to the next byte in buffer.
 *  Will either decrement or increment buffer index (endianness specific) */
void _bite_next(struct bite *self)
{
	self->idx += self->order;

	/* Handle overflows */
	if (self->idx < 0) {
		self->idx = 0;
		assert(false && "Index underflow");
	} else if (self->idx > (int8_t)self->cap) {
		self->idx = self->cap;
		assert(false && "Index overflow");
	} else {
	}
}

/* TODO return boolean status */
void bite_put_u8(struct bite *self, const uint8_t input)
{
	uint8_t lml = self->lsb_mask_len;
	uint8_t mml = self->msb_mask_len;

	assert(self);

	if (self->rem == 0u) {
		BITE_LOG("overflow");
	} else if ((lml == 0u) && (self->rem >= 8u)) { /* Aligned */
		/* If lsb mask is zero, it means that input is not shifted
		 * inside payload. We can safely read the payload and map it
		 * directly to input. The only exception if there are less
		 * than 8 bits of length remaining */

		*_bite_peek(self)  = input;
		self->rem	  -= 8u;

		/* Advance to the next byte if there bits left */
		if (self->rem > 0u) {
			_bite_next(self);
		}

		BITE_LOG("aligned");
	} else if ((self->rem + lml) >= 8u) { /* Carried */
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
		self->rem	  -= mml;

		/* Masking and writing MSB part to buffer[1] */
		_bite_next(self);

		/* If this is a last byte, MSB may be shorter than LSB mask */
		max_carry = (self->rem >= lml) ? lml : self->rem;
		mask	  = 0xFFu << max_carry;

		*_bite_peek(self) &= mask;
		*_bite_peek(self) |= (input >> mml) & (uint8_t)~mask;
		self->rem	  -= max_carry;
		BITE_LOG("carry");
	} else { /* Special case (sub-byte range like: 00111100) */
		uint8_t mask = (0xFFu >> (8u - self->rem)) << lml;

		*_bite_peek(self) &= ~mask;
		*_bite_peek(self) |= (uint8_t)(input << lml) & mask;
		self->rem	   = 0u; /* That's will be last byte anyways */
		BITE_LOG("spec");
	}
}

/* TODO return boolean status */
uint8_t bite_get_u8(struct bite *self)
{
	/* The code here is basically the same as in bite_put_u8.
	 * It just does thing the opposite way. */

	uint8_t output = 0u;

	uint8_t lml = self->lsb_mask_len;
	uint8_t mml = self->msb_mask_len;

	assert(self);

	if (self->rem == 0u) { /* Aligned */
		BITE_LOG("underflow");
	} else if ((lml == 0u) && (self->rem >= 8u)) {
		output	   = *_bite_peek(self);
		self->rem -= 8u;

		/* Advance to the next byte if there bits left */
		if (self->rem > 0u) {
			_bite_next(self);
		}

		BITE_LOG("aligned");
	} else if ((self->rem + lml) >= 8u) { /* Carried */
		uint8_t max_carry;	      /* How many bits we can carry? */
		uint8_t mask;

		/* Extract LSB part from current byte */
		output	   = (uint8_t)(*_bite_peek(self) >> lml);
		self->rem -= mml; /* Bits consumed from the first byte */

		/* Extract MSB part from the next byte */
		_bite_next(self);

		/* If this is a last byte, MSB may be shorter than LSB mask */
		max_carry  = (self->rem >= lml) ? lml : self->rem;
		mask	   = 0xFFu << max_carry;
		output	  |= (*_bite_peek(self) & (uint8_t)~mask) << mml;
		self->rem -= max_carry;
		BITE_LOG("carry");
	} else { /* Special case (bit range like: 00111100) */
		uint8_t mask = (0xFFu >> (8u - self->rem)) << lml;
		output	     = (uint8_t)((*_bite_peek(self) & mask) >> lml);
		self->rem    = 0u; /* That's will be last byte anyways */
		BITE_LOG("spec");
	}

	return output;
}

/* TODO return boolean status */
int16_t bite_get_i8(struct bite *self)
{
	uint8_t result		= 0u;
	uint8_t sign_bit_offset = self->rem - 1u;

	result = bite_get_u8(self);

	/* Check sign, if present - invert MSB part of the result */
	if ((result & (1u << sign_bit_offset)) > 0u) {
		result |= (0xFFu << sign_bit_offset);
	}

	return result;
}

/* TODO return boolean status */
void bite_put_u16(struct bite *self, const uint16_t data)
{
	bite_put_u8(self, data >> 0u);
	bite_put_u8(self, data >> 8u);
}

/* TODO return boolean status */
uint16_t bite_get_u16(struct bite *self)
{
	return ((uint16_t)bite_get_u8(self) << 0u) |
	       ((uint16_t)bite_get_u8(self) << 8u);
}

/* TODO return boolean status */
int16_t bite_get_i16(struct bite *self)
{
	uint16_t result		 = 0u;
	uint8_t	 sign_bit_offset = self->rem - 1u;

	result = bite_get_u16(self);

	/* Check sign, if present - invert MSB part of the result */
	if ((result & (1u << (uint16_t)sign_bit_offset)) > 0u) {
		result |= (0xFFFFu << (uint16_t)sign_bit_offset);
	}

	return (int16_t)result;
}

/* TODO return boolean status */
void bite_put_u32(struct bite *self, const uint32_t data)
{
	bite_put_u8(self, data >> 0u);
	bite_put_u8(self, data >> 8u);
	bite_put_u8(self, data >> 16u);
	bite_put_u8(self, data >> 24u);
}

/* TODO return boolean status */
uint32_t bite_get_u32(struct bite *self)
{
	return ((uint32_t)bite_get_u8(self) << 0u) |
	       ((uint32_t)bite_get_u8(self) << 8u) |
	       ((uint32_t)bite_get_u8(self) << 16u) |
	       ((uint32_t)bite_get_u8(self) << 24u);
}

/* TODO return boolean status */
int32_t bite_get_i32(struct bite *self)
{
	uint32_t result		 = 0u;
	uint8_t	 sign_bit_offset = self->rem - 1u;

	result = bite_get_u32(self);

	/* Check sign, if present - invert MSB part of the result */
	if ((result & (1u << (uint32_t)sign_bit_offset)) > 0u) {
		result |= (0xFFFFFFFFu << (uint32_t)sign_bit_offset);
	}

	return (int32_t)result;
}
#endif /* BITE_IMPLEMENTATION */

/** Signal definition (as in CAN dbc).
 *  @note This structure may not be optimal for simple signals */
struct bite_signal {
	const char *label; /**< Label used by the signal */
	const char *unit;  /**< Unit */

	/* (TODO integer arithmetic) */
	float factor; /**< Real value must be multiplied by this factor */
	float offset; /**< Real value offset (calculated after factor) */
	float min;    /**< Minimum allowed range */
	float max;    /**< Maximum allowed range */

	uint8_t start; /**< Data start position (in bits) */
	uint8_t len;   /**< Data length (in bits) */

	/** Physical data order within buf (endianness):
	 * 	-1 -> Big-endian    (low byte at high memory address)
	 * 	+1 -> Little-endian (low byte at low  memory address) */
	int8_t order;

	/** Signed/Unsigned (+/-) */
	uint8_t type;
};

/** Initializes bite signal instance. */
void bite_sig_init(struct bite_signal *self);

/** Set signal config to work with (according to CAN dbc format)
 *  returns true if success. */
bool bite_set_sig(struct bite *self, const struct bite_signal *sig);

#ifdef BITE_SIG_IMPLEMENTATION
void bite_sig_init(struct bite_signal *self)
{
	assert(self);

	self->label = NULL;
	self->unit  = NULL;

	self->factor = 0.0f;
	self->offset = 0.0f;
	self->min    = 0.0f;
	self->max    = 0.0f;

	self->start = 0u;
	self->len   = 0u;

	self->order = 0u;

	self->type = 0u;
}

bool bite_set_sig(struct bite *self, const struct bite_signal *sig)
{
	assert(self && sig);

	return bite_config(self, sig->order, sig->start, sig->len);
}
#endif /* BITE_SIG_IMPLEMENTATION */

/** This struct represents Can DBC message
 *  @warning HEAVY WIP */
struct bite_msg {
	uint32_t    bo_id;
	const char *bo_msgname;
	uint8_t	    bo_msglen;
	const char *bo_sender;

	struct bite_msg *sig_list;
	size_t		 sig_count;
};

#ifndef BITE_CGEN
#define BITE_CGEN(s) /**< Code generation output */
#endif

/** C generator setup instance.
 * Responsible for code generation output.
 *
 * Ongoing WIP (NOT FOR PRODUCTION CODE) */
struct bite_cgen {
	struct bite b;

	struct bite_msg *msg_list; /**< We must allocate list of messages... */
	size_t		 msg_count;

	/** Supported cpu word len: 1, 2, 4, 8, 16 (bytes)
	 * All input data will be multiple of wordlen
	 * Can be bigger than actual machine instruction, if supported by
	 * target C compiler. Can be less than machine instruction too.
	 *
	 * For maximum portablity select 1byte wordlen. Slow, but safe. */
	uint8_t cpu_wordlen;

	/** -1 Big endian; +1 Little endian. 0 - omit.
	 * Target CPU endianness.
	 *
	 * Though bite will try to generate portable C code.
	 * Some Non portable features may depend on platform endianness
	 * for better optimization. */
	uint8_t cpu_endianess;
};

#ifdef BITE_C_GEN_IMPLEMENTATION
void cgen_init(struct bite_cgen *self)
{
	self->cpu_wordlen   = 0u;
	self->cpu_endianess = 0u;
}

void bite_cgen_sig_write(struct bite_cgen *self, const struct bite_signal *sig)
{
	/*uint8_t output_wordlen = 1u;*/ /* At least 8 bit */

	assert(self && sig);

	/* Output wordlen determined based on signal data */
	/* Nearest power of 2 */
	/*while (output_wordlen < ((sig->len + 8u) / 8u)) {
		output_wordlen *= 2u;
	}

	BITE_CGEN(("#include <stdint.h>\n\n"));

	BITE_CGEN(("void bite_sig_%s_write(uint%u_t *dst, uint%u_t *src)\n",
		    sig->label, self->cpu_wordlen * 8u, output_wordlen * 8u));

	BITE_CGEN(("{\n"));
	BITE_CGEN(("\t#warning Nothing has been yet generated (WIP)\n"));
	BITE_CGEN(("}\n"));*/
}

void bite_cgen_sig(struct bite_cgen *self, const struct bite_signal *sig)
{
	assert(self && sig);

	/* We don't need to initialize bite struct, since we do not utilize
	 * standard bite functionality and only need some calculated values. */
	(void)bite_set_sig(&self->b, sig);

	bite_cgen_sig_write(self, sig);
}
#endif /* BITE_C_GEN_IMPLEMENTATION */

#endif /* BITE_HEADER_GUARD */
