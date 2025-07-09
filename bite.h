/**
 * @file bite.h
 * @brief Bit-wise read/write operations with debugging.
 */

#ifndef   BITE_GUARD
#define   BITE_GUARD

#include <stdint.h>
#include <stdbool.h>
#include <assert.h>

#ifdef    BITE_DEBUG
#pragma message ( "Debug output has been activated!" )
#endif /* BITE_DEBUG */

#ifdef    BITE_COLOR
#pragma message ( "ANSI color codes used for output!" )
#endif /* BITE_COLOR */

#ifdef    BITE_PEDANTIC
#pragma message ( "Pedantic assertions has been activated!" )
#endif /* BITE_PEDANTIC */

#ifdef    BITE_DEBUG_BUFFER_OVERFLOW
#pragma message ( "Buffer overflow assertions has been activated!" )
#endif /* BITE_DEBUG_BUFFER_OVERFLOW */

/* Unsafe optimizations disable safety functionality completely
 * It's advised to compile this code with O3 parameter to strip dead code */
#ifdef    BITE_UNSAFE_OPTIMIZATIONS
#pragma message ( "Unsafe optimizations has been activated!" )
#endif /* BITE_UNSAFE_OPTIMIZATIONS */

/******************************************************************************
 * CLASS
 *****************************************************************************/
 
/**
 * @enum bite_order
 * @brief Byte order options.
 */
enum bite_order
{
	BITE_ORDER_BIG_ENDIAN = 1, /**< Big endian */
	BITE_ORDER_MOTOROLA   = 1, /**< Big endian */
	BITE_ORDER_DBC_0      = 1, /**< Big endian */

	BITE_ORDER_LIL_ENDIAN = -1, /**< Little endian */
	BITE_ORDER_INTEL      = -1, /**< Little endian */
	BITE_ORDER_DBC_1      = -1  /**< Little endian */
};

/**
 * @enum bite_flags
 * @brief Error flags.
 */
enum bite_flags {
	BITE_FLAG_NONE,           /**< No flag */
	BITE_FLAG_OVERFLOW  = 1U, /**< Exceeded boundary */
	BITE_FLAG_UNDERFLOW = 2U, /**< Unfinished operation */
	BITE_FLAG_UNDEFINED = 4U, /**< Operation behaviour is undefined */
	BITE_FLAG_MEMORY    = 8U  /**< Operation may cause buffer overflow */
};

/**
 * @struct bite
 * @brief Bit-wise I/O context.
 */
struct bite {
	/* Config */
	uint8_t *_data;      /**< Data buffer */
	size_t   _data_size; /**< Data buffer size in bytes */

	size_t _ofs_bits; /**< Start bit offset */
	size_t _len_bits; /**< Total bit length */
	int8_t _order;    /**< Byte order */

	/* Runtime */
	size_t  _iter_bits; /**< Iteration offset */
#ifndef   BITE_UNSAFE_OPTIMIZATIONS
	uint8_t _flags;     /**< Error flags */
#endif /* BITE_UNSAFE_OPTIMIZATIONS */

	/* Precalculated */
	uint8_t _ofs;  /**< Offset from LSB inside every byte */
	size_t  _base; /**< Base offset inside data buffer */

#ifdef BITE_DEBUG
	bool debug;  /**< Debug mode flag */
	int8_t nest; /**< Debug nesting level */
#endif
};

/******************************************************************************
 * DEBUG
 *****************************************************************************/
#ifdef    BITE_COLOR
#define BITE_RED    "\x1b" "[1;31m"
#define BITE_YELLOW "\x1b" "[1;33m"
#define BITE_ORANGE "\x1b" "[38;5;208m"
#define BITE_WHITE  "\x1b" "[1;37m"
#define BITE_GREEN  "\x1b" "[1;32m"
#define BITE_CRST   "\x1b" "[0m"
#else  /* BITE_COLOR */
#define BITE_RED    ""
#define BITE_YELLOW ""
#define BITE_ORANGE ""
#define BITE_WHITE  ""
#define BITE_GREEN  ""
#define BITE_CRST   ""
#endif /* BITE_COLOR */

#define BITE_ERR  BITE_RED    "ERR:"  BITE_CRST " "
#define BITE_WARN BITE_ORANGE "WARN:" BITE_CRST " "
#define BITE_INFO BITE_GREEN  "INFO:" BITE_CRST " "

#ifdef    BITE_DEBUG
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
	const char *flag_name = NULL;

	if (flag == (uint8_t)BITE_FLAG_NONE) {
		flag_name = "BITE_FLAG_NONE";
	} else if (flag == (uint8_t)BITE_FLAG_OVERFLOW) {
		flag_name = "BITE_FLAG_OVERFLOW";
	} else if (flag == (uint8_t)BITE_FLAG_UNDERFLOW) {
		flag_name = "BITE_FLAG_UNDERFLOW";
	} else if (flag == (uint8_t)BITE_FLAG_UNDEFINED) {
		flag_name = "BITE_FLAG_UNDEFINED";
	} else if (flag == (uint8_t)BITE_FLAG_MEMORY) {
		flag_name = "BITE_FLAG_MEMORY";
	} else {
		flag_name = "BITE_FLAG_UNKNOWN";
	}

	if (self->debug) {
		_bite_debug_nest_apply(self);
		(void)printf("%s\n", flag_name);
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

#else /* BITE_DEBUG */
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

#endif /* BITE_DEBUG */
/******************************************************************************
 * PRIVATE
 *****************************************************************************/
uint8_t _bite_get_flags(struct bite *self)
{
	(void)self;
#ifndef   BITE_UNSAFE_OPTIMIZATIONS
	return self->_flags;
#else   /* BITE_UNSAFE_OPTIMIZATIONS */
	return 0U;
#endif  /* BITE_UNSAFE_OPTIMIZATIONS */
}

void _bite_set_flag(struct bite *self, uint8_t flag)
{
	if ((_bite_get_flags(self) | flag) != _bite_get_flags(self)) {
		_bite_debug_push(self, "_bite_set_flag");
		_bite_debug_flag(self, flag);
		_bite_debug_pop(self);

#ifdef   BITE_PEDANTIC
		(void)printf(BITE_ERR"flags has been set! "
			     BITE_RED"(BITE_PEDANTIC)\n"BITE_CRST);
		(void)fflush(0);
		assert(0);
#endif /*BITE_PEDANTIC*/
	}

#ifndef   BITE_UNSAFE_OPTIMIZATIONS
	self->_flags |= flag;
#endif  /* BITE_UNSAFE_OPTIMIZATIONS */
}

void _bite_remove_flag(struct bite *self, uint8_t flag)
{
	if ((_bite_get_flags(self) & (uint8_t)~flag) != _bite_get_flags(self))
	{
		_bite_debug_push(self, "_bite_remove_flag");
		_bite_debug_flag(self, flag);
		_bite_debug_pop(self);
	}

#ifndef   BITE_UNSAFE_OPTIMIZATIONS
	self->_flags &= ~flag;
#endif /* BITE_UNSAFE_OPTIMIZATIONS */
}

#ifdef    BITE_DEBUG_BUFFER_OVERFLOW
/* TODO Less ugly! (REFACTOR) */
void _bite_debug_buf_overflow(struct bite *self, uint8_t chunk_len,
			      size_t buf_idx)
{
	{
		size_t _buf_idx = buf_idx;

		_bite_debug_int(self, "buf idx", _buf_idx);
		assert(_buf_idx < self->_data_size);

		/* Pay attention to fragmented buffer (UGLY PART) */
		if ((self->_ofs != 0U) && (chunk_len > self->_ofs)) {
			_buf_idx += (size_t)self->_order;

			_bite_debug_int(self, "buf idx", _buf_idx);
			assert(_buf_idx < self->_data_size);
		}
	}
}
#endif /* BITE_DEBUG_BUFFER_OVERFLOW */

uint8_t *_bite_get_buf(struct bite *self, uint8_t *chunk_len)
{
	uint8_t *result = NULL;
	
	_bite_debug_push(self, "_bite_get_buf");

	if ((_bite_get_flags(self) & ((uint8_t)BITE_FLAG_UNDEFINED |
	                              (uint8_t)BITE_FLAG_MEMORY    )) > 0U) {
			_bite_debug_str(self,
				BITE_ERR"operation is undefined!");
	} else if (self->_iter_bits >= self->_len_bits) {
		_bite_debug_str(self,
				BITE_ERR"attempt to read out of bounds!");
		_bite_set_flag(self, BITE_FLAG_OVERFLOW);
	/* Return pointer to data if everything is ok */
	} else {
		size_t _chunk_len = self->_len_bits - self->_iter_bits;
		if (_chunk_len > 8U) { _chunk_len = 8U; }
		*chunk_len = _chunk_len;

#ifdef BITE_DEBUG_BUFFER_OVERFLOW
		_bite_debug_buf_overflow(self, *chunk_len, self->_base);
#endif

		result = &self->_data[self->_base];

		self->_base += (size_t)self->_order;

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
 */
void bite_init(struct bite *self)
{
	/* Config */
	self->_data      = NULL;
	self->_data_size = 0;

	self->_ofs_bits = 0U;
	self->_len_bits = 0U;

	/* Runtime */
	self->_iter_bits = 0U;
#ifndef   BITE_UNSAFE_OPTIMIZATIONS
	self->_flags     = 0U;
#endif /* BITE_UNSAFE_OPTIMIZATIONS */
	self->_order     = BITE_ORDER_BIG_ENDIAN;

	/* Precalculated */
	self->_ofs  = 0;
	self->_base = 0;

#ifdef    BITE_DEBUG
	self->debug = true;
	self->nest  = 0;
#endif /* BITE_DEBUG */
	_bite_debug_push(self, "bite_init");
	_bite_debug_str(self, BITE_WARN"debug mode is activated! This "
			      "WILL cause serious performance impact!");

#ifdef    BITE_PEDANTIC
	_bite_debug_str(self, BITE_WARN"pedantic mode is activated! This "
					"enables assertions and MAY lead to "
					"unexpected crashes! Please disable "
					"pedantic mode in production code!");
#endif /* BITE_PEDANTIC */

#ifdef    BITE_DEBUG_BUFFER_OVERFLOW
	_bite_debug_str(self, BITE_WARN"Buffer overflow checks enabled! "
					"This is only useful for bitE "
					"library internal test purposes.");
#endif /* BITE_DEBUG_BUFFER_OVERFLOW */

#ifdef    BITE_UNSAFE_OPTIMIZATIONS
	_bite_debug_str(self, BITE_WARN"Unsafe optimizations enabled! This "
					"MAY lead to undefined behaviour! "
					"Keep this only in case of 100% "
					"test coverage.");
#endif /* BITE_UNSAFE_OPTIMIZATIONS */

	_bite_debug_pop(self);
}

/**
 * @brief Set pointer to a buffer bitE will operate on.
 * @param self Context
 * @param buf  Data buffer
 * @param size Data buffer size (in bytes)
 */
void bite_set_buf(struct bite *self, uint8_t *buf, size_t size)
{
	_bite_debug_push(self, "bite_set_buf");

	if (self->_iter_bits < self->_len_bits) {
		_bite_debug_str(self,
			    BITE_ERR"Can not change buffer during operation!");
		_bite_set_flag(self, BITE_FLAG_UNDEFINED);
	} else {
		self->_data      = buf;
		self->_data_size = size;
	}

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
	size_t last_bit_index = 0;
	
	_bite_debug_push(self, "bite_begin");

	_bite_debug_int(self, "ofs_bits", ofs_bits);
	_bite_debug_int(self, "len_bits", len_bits);
	_bite_debug_str(self, "");

	/* Zero bit operations are undefined */
	if (len_bits == 0U) {
		_bite_debug_str(self, 
				BITE_ERR"zero bit operations are undefined!");
		_bite_set_flag(self, BITE_FLAG_UNDEFINED);
	} else {
		_bite_remove_flag(self, BITE_FLAG_UNDEFINED);
	}

	/* Previous `bite_begin` operation has not ended properly */
	if (self->_iter_bits < self->_len_bits) {
		_bite_debug_str(self, 
				BITE_WARN"previous operation is unfinished!");
		_bite_set_flag(self, BITE_FLAG_UNDERFLOW);
	} else {
		_bite_remove_flag(self, BITE_FLAG_UNDERFLOW);
	}

	/* TODO more precalculations */

	/* Calculate offset inside every byte, base offset and last bit idx */
	if (order == BITE_ORDER_BIG_ENDIAN) {
		self->_ofs  = (ofs_bits + 1U) % 8U;
		self->_base = ofs_bits / 8U;

		last_bit_index = (((ofs_bits ^ 7U) + len_bits) - 1U);
	} else {
		self->_ofs = (ofs_bits + len_bits) % 8U;

		/*       VISUAL EXPLAINATION       */
		/* ofs = 8, len = 8                */
		/*                                 */
		/*         ofs|       len|         */
		/*            .          .         */
		/* 0000 0000  1111 1111  0000 0000 */
		/*                    ^            */
		/*               len-1|            */
		/*                                 */
		self->_base = (ofs_bits + len_bits - 1U) / 8U;

		last_bit_index = (ofs_bits + len_bits) - 1U;
	}

	/* If last bit index goes beyond buffer - set error flag */
	if ((last_bit_index / 8U) >= self->_data_size) {
		_bite_debug_str(self, 
		      BITE_ERR"operation may cause buffer overflow!");
		_bite_set_flag(self, BITE_FLAG_MEMORY);
	} else {
		_bite_remove_flag(self, BITE_FLAG_MEMORY);
	}

	self->_ofs_bits = ofs_bits;
	self->_len_bits = len_bits;

	self->_iter_bits = 0U;

	self->_order = order;

	/* _bite_debug_nest(self, 1);
	   _bite_debug_pop(self); */
}

/**
 * @brief End current operation.
 * @param self Context
 */
void bite_end(struct bite *self)
{	
	_bite_debug_push(self, "bite_end");

	if (self->_iter_bits < self->_len_bits) {
		_bite_debug_str(self,
				BITE_ERR"current operation is unfinished!");
		_bite_set_flag(self, BITE_FLAG_UNDERFLOW);
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
	
	/* Hotfix: reset base pointer */
	if (self->_order == BITE_ORDER_BIG_ENDIAN) {
		self->_base = self->_ofs_bits / 8U;
	} else {
		self->_base = (self->_ofs_bits + self->_len_bits - 1U) / 8U;
	}

	_bite_debug_pop(self);
}

/**
 * @brief Write 8-bit or less data.
 * @param self Context
 * @param data Data to write
 */
void bite_write(struct bite *self, uint8_t data)
{
	/* Data chunk */
	uint8_t chunk_len;
	uint8_t *d;

	_bite_debug_push(self, "bite_write");

	/* Get destination data buffer */
	d = _bite_get_buf(self, &chunk_len);
	
	if (d != NULL) {
		_bite_debug_bin(self, "source data      ", data);
		_bite_debug_int(self, "offset (from LSB)", self->_ofs);
		_bite_debug_int(self, "bits to write    ", chunk_len);
	}

	if (d == NULL) {
		/* Nothing to do here */
		_bite_debug_str(self,
				BITE_ERR"destination data fetch failed!");
	} else if (self->_ofs == 0U) {
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
	} else if (chunk_len > self->_ofs) {
		/* Data is split into MSB and LSB parts */
		uint8_t *msb = &d[0];
		uint8_t *lsb;

		uint8_t msb_len = self->_ofs;
		uint8_t lsb_len = chunk_len - msb_len;

		/* Masks to bound our data */
		uint8_t mask_msb = 0xFFU << msb_len;
		uint8_t mask_lsb = 0xFFU >> lsb_len;

		/* Temporary variables */
		uint8_t src;
		uint8_t dst;

		lsb = &d[self->_order];

		/* MSB goes into first byte */
		_bite_debug_str(self, "");
		_bite_debug_str(self, BITE_INFO"chunk is fragmented");
		_bite_debug_str(self, "");

		_bite_debug_int(self, "MSB len     ", msb_len);
		_bite_debug_bin(self, "MSB mask    ", mask_msb);
		
		dst = (*msb & mask_msb);
		_bite_debug_bin(self, "MSB data    ", *msb);
		_bite_debug_bin(self, "MSB masked  ", dst);
		
		src = (data >> lsb_len);
		_bite_debug_bin(self, "src data    ", data);
		_bite_debug_bin(self, "src shifted ", src);
		src &= (uint8_t)~mask_msb;
		_bite_debug_bin(self, "src masked  ", src);

		*msb = dst | src;
		_bite_debug_bin(self, "result (MSB)", *msb);

		_bite_debug_str(self, "");

		/* LSB goes into second byte */
		_bite_debug_int(self, "LSB len     ", lsb_len);
		_bite_debug_bin(self, "LSB mask    ", mask_lsb);
		
		dst = (*lsb & mask_lsb);
		_bite_debug_bin(self, "LSB data    ", *lsb);
		_bite_debug_bin(self, "LSB masked  ", dst);
		
		src = (data << (8U - lsb_len));
		_bite_debug_bin(self, "src data    ", data);
		_bite_debug_bin(self, "src shifted ", src);
		src &= (uint8_t)~mask_lsb;
		_bite_debug_bin(self, "src masked  ", src);
		
		*lsb = dst | src;
		_bite_debug_bin(self, "result (LSB)", *lsb);
	} else {
		/* Data fits into one byte */
		uint8_t mask = (0xFFU <<              self->_ofs) ^
			       (0xFFU << (self->_ofs - chunk_len));

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
		
		src = data << (self->_ofs - chunk_len);
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

	/* Data chunk */
	uint8_t chunk_len;
	uint8_t *d;

	_bite_debug_push(self, "bite_read");

	/* Get destination data buffer */
	d = _bite_get_buf(self, &chunk_len);
	
	if (d != NULL) {
		_bite_debug_int(self, "offset (from LSB)", self->_ofs);
		_bite_debug_int(self, "bits to read     ", chunk_len);
	}

	if (d == NULL) {
		/* Nothing to do here */
		_bite_debug_str(self, BITE_ERR"source data fetch failed!");
	} else if (self->_ofs == 0U) {
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
	} else if (chunk_len > self->_ofs) {
		/* Data is split into MSB and LSB parts */
		uint8_t *msb = &d[0];
		uint8_t *lsb;

		uint8_t msb_len = self->_ofs;
		uint8_t lsb_len = chunk_len - msb_len;

		/* Masks to bound our data */
		uint8_t mask_msb = 0xFFU << msb_len;
		uint8_t mask_lsb = 0xFFU >> lsb_len;

		/* Temporary variables */
		uint8_t src;

		lsb = &d[self->_order];

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
		uint8_t mask = (0xFFU <<              self->_ofs) ^
			       (0xFFU << (self->_ofs - chunk_len));

		/* Temporary variables */
		uint8_t src;

		_bite_debug_str(self, "");
		_bite_debug_str(self, BITE_INFO"chunk is a bitfield");
		_bite_debug_str(self, "");
		_bite_debug_bin(self, "mask       ", mask);
		
		src = (d[0] & mask);
		_bite_debug_bin(self, "src data   ", d[0]);
		_bite_debug_bin(self, "src masked ", src);

		src = src >> (self->_ofs - chunk_len);
		_bite_debug_bin(self, "src shifted", src);
		
		r |= src;
		_bite_debug_bin(self, "result     ", r);
	}

	_bite_debug_pop(self);

	return r;
}

/* TODO, WIP, REVIEW, TEST, OPTIMIZE */
/**
 * @brief Write 16 bit integer to stream (sign doesn't matter)
 * @param self Context
 */
void bite_write_16(struct bite *self, uint16_t data)
{
	/* PROBABLY BUGGY TODO CHECKME! */
	bite_write(self, (uint8_t)((data & 0x00FFU) >> 0));
	bite_write(self, (uint8_t)((data & 0xFF00U) >> 8));
}

/**
 * @brief Read 16 bit integer from stream (unsigned)
 * @param self Context
 */
uint16_t bite_read_u16(struct bite *self)
{
	uint16_t result = 0U;
	uint8_t  offset = self->_len_bits % 8U;

	/* Offset can't be zero */
	if (offset == 0U) {
		offset = 8U;
	}

	result |= (uint16_t)bite_read(self) << offset;
	result |= (uint16_t)bite_read(self);

	return (int16_t)result;
}

/**
 * @brief Read 16 bit integer from stream (signed)
 * @param self Context
 */
int16_t bite_read_i16(struct bite *self)
{
	uint16_t result = 0U;
	uint8_t  sign_bit_offset = self->_len_bits - 1U;

	result = bite_read_u16(self);

	/* Check sign, if present - invert MSB */
	if ((result & (1U << (uint16_t)sign_bit_offset)) > 0U) {
		result |= (0xFFU << (uint16_t)sign_bit_offset);
	}

	return (int16_t)result;
}

#ifndef   BITE_UNSAFE_OPTIMIZATIONS
/**
 * @brief Get internal error flags (disabled if BITE_UNSAFE_OPTIMIZATIONS set).
 * @param self Context
 */
uint8_t bite_get_flags(struct bite *self)
{
	return self->_flags;
}
#endif  /* BITE_UNSAFE_OPTIMIZATIONS */

#endif /* BITE_GUARD */
