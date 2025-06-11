/**
 * @file bite.h
 * @brief Bit-wise read/write operations with debugging.
 */
#ifndef BITE_GUARD
#define BITE_GUARD

#include <stdint.h>
#include <stdbool.h>
#include <assert.h>

/******************************************************************************
 * CLASS
 *****************************************************************************/
 
/**
 * @enum bite_order
 * @brief Byte order options.
 */
enum bite_order
{
	BITE_ORDER_BIG_ENDIAN = 0, /**< Big endian */
	BITE_ORDER_MOTOROLA   = 0, /**< Big endian */
	BITE_ORDER_DBC_0      = 0, /**< Big endian */

	BITE_ORDER_LIL_ENDIAN = 1, /**< Little endian */
	BITE_ORDER_INTEL      = 1, /**< Little endian */
	BITE_ORDER_DBC_1      = 1  /**< Little endian */
};

/**
 * @enum bite_flags
 * @brief Status flags.
 */
enum bite_flags {
	BITE_FLAG_NONE,           /**< No flag */
	BITE_FLAG_OVERFLOW  = 1U, /**< Exceeded boundary */
	BITE_FLAG_UNDERFLOW = 2U  /**< Unfinished operation */
};

/**
 * @struct bite
 * @brief Bit-wise I/O context.
 */
struct bite {
	/* Config */
	uint8_t *_data; /**< Data buffer */

	size_t _ofs_bits; /**< Start bit offset */
	size_t _len_bits; /**< Total bit length */

	/* Runtime */
	size_t  _iter_bits; /**< Iteration offset */
	uint8_t  flags;     /**< Status flags */
	uint8_t _order;     /**< Byte order */

#ifdef BITE_DEBUG
	bool debug;  /**< Debug mode flag */
	int8_t nest; /**< Debug nesting level */
#endif
};

/******************************************************************************
 * DEBUG
 *****************************************************************************/
#define BITE_RED    "\x1b" "[1;31m"
#define BITE_YELLOW "\x1b" "[1;33m"
#define BITE_ORANGE "\x1b" "[38;5;208m"
#define BITE_WHITE  "\x1b" "[1;37m"
#define BITE_GREEN  "\x1b" "[1;32m"
#define BITE_CRST    "\x1b" "[0m"

#define BITE_ERR  BITE_RED    "ERR: "  BITE_CRST
#define BITE_WARN BITE_ORANGE "WARN: " BITE_CRST
#define BITE_INFO BITE_GREEN  "INFO: " BITE_CRST

#ifdef BITE_DEBUG
void _bite_debug_nest(struct bite *self, int8_t level)
{
	self->nest += level;
}

void _bite_debug_nest_apply(struct bite *self)
{
	int8_t i = 0;

	if (self->nest < 0) {
		self->nest = 0;
	}

	for (i = 0; i < self->nest; i++) {
		(void)printf("\t");
	}
}

void _bite_debug_flag(struct bite *self, uint8_t flag)
{
	const char *flag_names[] = {"BITE_FLAG_NONE", "BITE_FLAG_OVERFLOW",
				   "BITE_FLAG_UNDERFLOW", "BITE_FLAG_UNKNOWN"};

	const char *flag_name = NULL;

	if (flag <= 2U) {
		flag_name = flag_names[flag];
	} else {
		flag_name = flag_names[3];
	}

	if (self->debug) {
		_bite_debug_nest_apply(self);
		(void)printf("self->flag |= %s\n", flag_name);
	}
}

void _bite_debug_push(struct bite *self, const char *name)
{
	if (self->debug) {
		_bite_debug_nest_apply(self);
		(void)printf(BITE_WHITE "%s\n" BITE_CRST, name);

		_bite_debug_nest(self, 1);
	}
}

void _bite_debug_int(struct bite *self, const char *name, int val)
{
	if (self->debug) {
		_bite_debug_nest_apply(self);
		(void)printf("%s: 0x%02X (%ud)\n", name, val, val);
	}
}

void _bite_debug_u8x(struct bite *self, const char *name, int val)
{
	if (self->debug) {
		_bite_debug_nest_apply(self);
		(void)printf("%s: 0x%02X\n", name, val);
	}
}

void _bite_debug_str(struct bite *self, const char *str)
{
	if (self->debug) {
		_bite_debug_nest_apply(self);
		(void)printf("%s\n", str);
	}
}

void _bite_debug_bin(struct bite *self, const char *name, uint8_t val)
{
	int8_t i;

	if (self->debug) {
		_bite_debug_nest_apply(self);
		(void)printf("%s: 0x%02X (", name, val);
		for (i = 7U; i >= 0; i--) {
			(void)putchar((val & (1U << (uint8_t)i)) ? '1' : '0');
		}
	
		(void)printf("b)\n");
	}
}

void _bite_debug_pop(struct bite *self)
{
	if (self->debug) {
		_bite_debug_nest(self, -1);
		(void)printf("\n");
	}
}

#else
void _bite_debug_nest(struct bite *self, int8_t level)
{
	(void)self;
	(void)level;
}

void _bite_debug_nest_apply(struct bite *self)
{
	(void)self;
}

void _bite_debug_flag(struct bite *self, uint8_t flag)
{
	(void)self;
	(void)flag;
}

void _bite_debug_push(struct bite *self, const char *name)
{
	(void)self;
	(void)name;
}

void _bite_debug_int(struct bite *self, const char *name, int val)
{
	(void)self;
	(void)name;
	(void)val;
}

void _bite_debug_u8x(struct bite *self, const char *name, int val)
{
	(void)self;
	(void)name;
	(void)val;
}

void _bite_debug_str(struct bite *self, const char *str)
{
	(void)self;
	(void)str;
}

void _bite_debug_bin(struct bite *self, const char *name, uint8_t val)
{
	(void)self;
	(void)name;
	(void)val;
}

void _bite_debug_pop(struct bite *self)
{
	(void)self;
}

#endif

/******************************************************************************
 * PRIVATE
 *****************************************************************************/
uint8_t *_bite_get_buf(struct bite *self, uint8_t *chunk_len)
{
	uint8_t *result = NULL;
	
	_bite_debug_push(self, "_bite_get_buf");

	if (self->_iter_bits >= self->_len_bits) {
		_bite_debug_str(self,
				BITE_ERR"Attempt to read out of bounds!");
		self->flags |= BITE_FLAG_OVERFLOW;
		_bite_debug_flag(self, BITE_FLAG_OVERFLOW);
	/* Return pointer to data if everything is ok */
	} else {
		size_t _chunk_len = self->_len_bits - self->_iter_bits;
		if (_chunk_len > 8U) { _chunk_len = 8U; }
		*chunk_len = _chunk_len;

		if (self->_order == (uint8_t)BITE_ORDER_BIG_ENDIAN) {
			result = &self->_data[
				      (self->_ofs_bits + self->_iter_bits) / 8U
				 ];
		} else {
			result = &self->_data[
				    ((self->_ofs_bits + self->_len_bits - 1U) -
				      self->_iter_bits) / 8U
				 ];
		}
	
		self->_iter_bits += _chunk_len;
	}

	_bite_debug_pop(self);

	return result;
}

/******************************************************************************
 * PUBLIC
 *****************************************************************************/
/**
 * @brief Initialize bite context.
 * @param self Context
 * @param buf  Data buffer
 */
void bite_init(struct bite *self, uint8_t *buf)
{
	/* Config */
	self->_data = buf;

	self->_ofs_bits = 0U;
	self->_len_bits = 0U;

	/* Runtime */
	self->_iter_bits = 0U;
	self-> flags = 0U;
	self->_order = BITE_ORDER_BIG_ENDIAN;

#ifdef BITE_DEBUG
	self->debug = true;
	self->nest  = 0;
#endif
	_bite_debug_push(self, "bite_init");
	_bite_debug_str(self, BITE_WARN"debug mode is activated! This "
			      "may cause serious performance impact!");
	_bite_debug_pop(self);
}

/**
 * @brief Begin bit-wise operation.
 * @param self     Context
 * @param ofs_bits Bit offset
 * @param len_bits Bit length
 * @param order    Byte order
 */
void bite_begin(struct bite *self, size_t ofs_bits, size_t len_bits,
		enum bite_order order)
{
	_bite_debug_push(self, "bite_begin");

	assert(len_bits > 0U);

	if (self->_iter_bits < self->_len_bits) {
		self->flags |= BITE_FLAG_UNDERFLOW;
		_bite_debug_str(self, 
				BITE_ERR"Previous operation unfinished!");
		_bite_debug_flag(self, BITE_FLAG_UNDERFLOW);
	} else {
		self->flags &= ~BITE_FLAG_UNDERFLOW;
	}

	self->_ofs_bits = ofs_bits;
	self->_len_bits = len_bits;

	_bite_debug_int(self, "ofs_bits", ofs_bits);
	_bite_debug_int(self, "len_bits", len_bits);

	self->_iter_bits = 0U;

	self->_order = order;

	_bite_debug_nest(self, 1);
	_bite_debug_pop(self);
}

/**
 * @brief End current operation.
 * @param self Context
 */
void bite_end(struct bite *self)
{	
	_bite_debug_push(self, "bite_end");

	if (self->_iter_bits < self->_len_bits) {
		self-> flags |= BITE_FLAG_UNDERFLOW;
		_bite_debug_str(self,
				BITE_ERR"Previous operation unfinished!");
		_bite_debug_flag(self, BITE_FLAG_UNDERFLOW);
	}

	_bite_debug_nest(self, -99);
	_bite_debug_pop(self);
}

/**
 * @brief Rewind iteration.
 * @param self Context
 */
void bite_rewind(struct bite *self)
{
	_bite_debug_push(self, "bite_rewind");

	self->_iter_bits = 0U;
	
	_bite_debug_pop(self);
}

/**
 * @brief Write 8-bit or less data.
 * @param self Context
 * @param data Data to write
 */
void bite_write(struct bite *self, uint8_t data)
{
	/* Offset from LSB */
	uint8_t ofs;

	/* Data chunk */
	uint8_t chunk_len;
	uint8_t *d;

	_bite_debug_push(self, "bite_write");

	if (self->_order == (uint8_t)BITE_ORDER_BIG_ENDIAN) {
		ofs = (self->_ofs_bits + 1U) % 8U;
	} else {
		ofs = (self->_ofs_bits + (self->_len_bits % 8U)) % 8U;
	}

	/* Get destination data buffer */
	d = _bite_get_buf(self, &chunk_len);
	
	if (d != NULL) {
		_bite_debug_bin(self, "source data      ", data);
		_bite_debug_int(self, "offset (from LSB)", ofs);
		_bite_debug_int(self, "bits to write    ", chunk_len);
	}

	if (d == NULL) {
		/* Nothing to do here */
		_bite_debug_str(self,
				BITE_ERR"destination data fetch failed!");
	} else if (ofs == 0U) {
		_bite_debug_str(self, "");
		_bite_debug_str(self, BITE_INFO"chunk is 8bit aligned");
		if (chunk_len < 8U) {
			uint8_t mask = (0xFFU >> chunk_len);

			_bite_debug_str(self, BITE_INFO"chunk end is short");
			_bite_debug_str(self, "");
			_bite_debug_int(self, "chunk_len", chunk_len);

			_bite_debug_bin(self, "destination data ", d[0]);
			d[0] &= mask;
			_bite_debug_bin(self, "mask             ", mask);
			d[0] |= data << (8U - chunk_len);
			_bite_debug_bin(self, "write result     ", d[0]);
		} else {
			_bite_debug_str(self, "");
			_bite_debug_bin(self, "destination data ", d[0]);
			d[0] = data;
			_bite_debug_bin(self, "write result     ", d[0]);
		}
	} else if (chunk_len > ofs) {
		/* Data is split into MSB and LSB parts */
		uint8_t *msb = &d[0];
		uint8_t *lsb;

		uint8_t msb_len = ofs;
		uint8_t lsb_len = chunk_len - msb_len;

		/* Masks to bound our data */
		uint8_t mask_msb = 0xFFU << msb_len;
		uint8_t mask_lsb = 0xFFU >> lsb_len;

		/* Temporary variables */
		uint8_t src;
		uint8_t dst;

		if (self->_order == (uint8_t)BITE_ORDER_BIG_ENDIAN) {
			lsb = &d[1];
		} else {
			lsb = &d[-1];
		}

		/* MSB goes into first byte */
		_bite_debug_str(self, "");
		_bite_debug_str(self, BITE_INFO"chunk is fragmented");
		_bite_debug_str(self, "");

		_bite_debug_int(self, "MSB len       ", msb_len);
		_bite_debug_bin(self, "MSB mask      ", mask_msb);
		
		dst = (*msb & mask_msb);
		_bite_debug_bin(self, "MSB data   ", *msb);
		_bite_debug_bin(self, "MSB masked ", dst);
		
		src = (data >> lsb_len);
		_bite_debug_bin(self, "src data      ", data);
		_bite_debug_bin(self, "src shifted   ", src);
		src &= (uint8_t)~mask_msb;
		_bite_debug_bin(self, "src masked    ", src);

		*msb = dst | src;
		_bite_debug_bin(self, "result (MSB)  ", *msb);

		_bite_debug_str(self, "");

		/* LSB goes into second byte */
		_bite_debug_int(self, "LSB len       ", lsb_len);
		_bite_debug_bin(self, "LSB mask      ", mask_lsb);
		
		dst = (*lsb & mask_lsb);
		_bite_debug_bin(self, "LSB data   ", *lsb);
		_bite_debug_bin(self, "LSB masked ", dst);
		
		src = (data << (8U - lsb_len));
		_bite_debug_bin(self, "src data      ", data);
		_bite_debug_bin(self, "src shifted   ", src);
		src &= (uint8_t)~mask_lsb;
		_bite_debug_bin(self, "src masked    ", src);
		
		*lsb = dst | src;
		_bite_debug_bin(self, "result (LSB)  ", *lsb);
	} else {
		/* Data fits into one byte */
		uint8_t mask = (0xFFU <<              ofs) ^
			       (0xFFU << (ofs - chunk_len));

		/* Temporary variables */
		uint8_t src;
		uint8_t dst;

		_bite_debug_str(self, "");
		_bite_debug_str(self, BITE_INFO"chunk is a bitfield");
		_bite_debug_str(self, "");
		_bite_debug_bin(self, "mask       ", mask);
		
		dst = (d[0] & (uint8_t)~mask);
		_bite_debug_bin(self, "dst data   ", d[0]);
		_bite_debug_bin(self, "dst masked ", dst);
		
		src = data << (ofs - chunk_len);
		_bite_debug_bin(self, "src data   ", data);
		_bite_debug_bin(self, "src shifted", src);
		src &= mask;
		_bite_debug_bin(self, "src masked ", src);

		d[0] = dst | src;
		_bite_debug_bin(self, "result     ", d[0]);
	}

	_bite_debug_pop(self);
}

/**
 * @brief Read 8-bit or less data.
 * @param self Context
 * @return Data read
 */
uint8_t bite_read(struct bite *self)
{
	uint8_t r = 0;

	/* Offset from LSB */
	uint8_t ofs;

	/* Data chunk */
	uint8_t chunk_len;
	uint8_t *d;

	_bite_debug_push(self, "bite_read");

	if (self->_order == (uint8_t)BITE_ORDER_BIG_ENDIAN) {
		ofs = (self->_ofs_bits + 1U) % 8U;
	} else {
		ofs = (self->_ofs_bits + (self->_len_bits % 8U)) % 8U;
	}

	/* Get destination data buffer */
	d = _bite_get_buf(self, &chunk_len);
	
	if (d != NULL) {
		_bite_debug_int(self, "offset (from LSB)", ofs);
		_bite_debug_int(self, "bits to read     ", chunk_len);
	}

	if (d == NULL) {
		/* Nothing to do here */
		_bite_debug_str(self, BITE_ERR"source data fetch failed!");
	} else if (ofs == 0U) {
		_bite_debug_str(self, "");
		_bite_debug_str(self, BITE_INFO"chunk is 8bit aligned");
		if (chunk_len < 8U) {
			_bite_debug_str(self, BITE_INFO"chunk end is short");
			_bite_debug_str(self, "");
			_bite_debug_int(self, "chunk_len  ", chunk_len);
			
			_bite_debug_bin(self, "source data", d[0]);
			r = d[0] >> (8U - chunk_len);
			_bite_debug_bin(self, "read result", r);
		} else {
			_bite_debug_str(self, "");
			_bite_debug_bin(self, "source data", d[0]);
			r = d[0];
			_bite_debug_bin(self, "read result", r);
		}
	} else if (chunk_len > ofs) {
		/* Data is split into MSB and LSB parts */
		uint8_t *msb = &d[0];
		uint8_t *lsb;

		uint8_t msb_len = ofs;
		uint8_t lsb_len = chunk_len - msb_len;

		/* Masks to bound our data */
		uint8_t mask_msb = 0xFFU << msb_len;
		uint8_t mask_lsb = 0xFFU >> lsb_len;

		/* Temporary variables */
		uint8_t src;

		if (self->_order == (uint8_t)BITE_ORDER_BIG_ENDIAN) {
			lsb = &d[1];
		} else {
			lsb = &d[-1];
		}

		/* MSB goes into first byte */
		_bite_debug_str(self, "");
		_bite_debug_str(self, BITE_INFO"chunk is fragmented");
		_bite_debug_str(self, "");

		_bite_debug_int(self, "MSB len    ", msb_len);
		_bite_debug_bin(self, "MSB mask   ", mask_msb);
		
		src = (*msb & ~mask_msb);
		_bite_debug_bin(self, "MSB data   ", *msb);
		_bite_debug_bin(self, "MSB masked ", src);
		
		src = (src << lsb_len);
		_bite_debug_bin(self, "MSB shifted", src);

		r |= src;
		_bite_debug_str(self, "");

		/* LSB goes into second byte */
		_bite_debug_int(self, "LSB len    ", lsb_len);
		_bite_debug_bin(self, "LSB mask   ", mask_lsb);
		
		src = (*lsb & ~mask_lsb);
		_bite_debug_bin(self, "LSB data   ", *lsb);
		_bite_debug_bin(self, "LSB masked ",  src);
		
		src = (src >> (8U - lsb_len));
		_bite_debug_bin(self, "LSB shifted", src);
		
		r |= src;
		_bite_debug_bin(self, "result     ", r);
	} else {
		/* Data fits into one byte */
		uint8_t mask = (0xFFU <<              ofs) ^
			       (0xFFU << (ofs - chunk_len));

		/* Temporary variables */
		uint8_t src;

		_bite_debug_str(self, "");
		_bite_debug_str(self, BITE_INFO"chunk is a bitfield");
		_bite_debug_str(self, "");
		_bite_debug_bin(self, "mask       ", mask);
		
		src = (d[0] & mask);
		_bite_debug_bin(self, "src data   ", d[0]);
		_bite_debug_bin(self, "src masked ", src);

		src = src >> (ofs - chunk_len);
		_bite_debug_bin(self, "src shifted", src);
		
		r |= src;
		_bite_debug_bin(self, "result     ", r);
	}

	_bite_debug_pop(self);

	return r;
}

#endif /* BITE_GUARD */
