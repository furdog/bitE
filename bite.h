#include <stdint.h>
#include <stdbool.h>
#include <assert.h>

/******************************************************************************
 * CLASS
 *****************************************************************************/
enum bite_order
{
	BITE_ORDER_LIL_ENDIAN = 0,
	BITE_ORDER_BIG_ENDIAN = 1
};

enum bite_flags {
	BITE_FLAG_NONE,
	
	/* Read or write operation has exceed configured boundary */
	BITE_FLAG_OVERFLOW    = 1U,
	
	/* Configure call during unfinished read or write operation */
	BITE_FLAG_UNDERFLOW   = 2U
};

struct bite {
	/* Config */
	uint8_t *_data;

	size_t _ofs_bits;
	size_t _len_bits;

	/* Runtime */
	size_t  _iter_bits;

	uint8_t  flags;
	
	uint8_t _order;

#ifdef BITE_DEBUG
	bool debug;
	int8_t nest;
#endif
};

/******************************************************************************
 * DEBUG
 *****************************************************************************/
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
		(void)printf("bite_set_flag (ctx=%p, %s)\n", (void *)&self,
		flag_name);
	}
}

void _bite_debug_push(struct bite *self, const char *name)
{
	if (self->debug) {
		_bite_debug_nest_apply(self);
		(void)printf("%s(ctx=%p)\n", name, (void *)&self);

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

#endif

/******************************************************************************
 * PRIVATE
 *****************************************************************************/
uint8_t *_bite_get_dst_buf(struct bite *self, uint8_t *chunk_len)
{
	uint8_t *result = NULL;
	
	_bite_debug_push(self, "_bite_get_dst_buf");

	if (self->_iter_bits >= self->_len_bits) {
		_bite_debug_str(self, "ERR: Attempt to read out of bounds!");
		self->flags |= BITE_FLAG_OVERFLOW;
		_bite_debug_flag(self, BITE_FLAG_OVERFLOW);
	/* Return pointer to data if everything is ok */
	} else {
		size_t _chunk_len = self->_len_bits - self->_iter_bits;
		if (_chunk_len > 8U) { _chunk_len = 8U; }
		*chunk_len = _chunk_len;

		result = &self->_data[
				(self->_ofs_bits + self->_iter_bits) / 8U
			 ];
	
		self->_iter_bits += _chunk_len;
	}

	_bite_debug_pop(self);

	return result;
}

/******************************************************************************
 * PUBLIC
 *****************************************************************************/
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
	_bite_debug_str(self, "WARN: debug mode is activated! This "
			      "may cause serious performance impact!");
	_bite_debug_pop(self);
}

void bite_begin(struct bite *self, size_t ofs_bits, size_t len_bits)
{
	_bite_debug_push(self, "bite_begin");

	if (self->_iter_bits < self->_len_bits) {
		self->flags |= BITE_FLAG_UNDERFLOW;
		_bite_debug_str(self, "ERR: Previous operation unfinished!");
		_bite_debug_flag(self, BITE_FLAG_UNDERFLOW);
	} else {
		self->flags = 0U;
	}

	self->_ofs_bits = ofs_bits;
	self->_len_bits = len_bits;

	_bite_debug_int(self, "ofs_bits", ofs_bits);
	_bite_debug_int(self, "len_bits", len_bits);

	self->_iter_bits = 0U;

	_bite_debug_nest(self, 1);
	_bite_debug_pop(self);
}

void bite_end(struct bite *self)
{	
	_bite_debug_push(self, "bite_end");

	if (self->_iter_bits < self->_len_bits) {
		self-> flags |= BITE_FLAG_UNDERFLOW;
		_bite_debug_str(self, "ERR: Previous operation unfinished!");
		_bite_debug_flag(self, BITE_FLAG_UNDERFLOW);
	}

	_bite_debug_nest(self, -99);
	_bite_debug_pop(self);
}

void bite_rewind(struct bite *self)
{
	_bite_debug_push(self, "bite_rewind");

	self->_iter_bits = 0U;
	
	_bite_debug_pop(self);
}

void bite_write(struct bite *self, uint8_t data)
{
	/* Offset from LSB */
	uint8_t ofs = (self->_ofs_bits + 1U) % 8U;
	
	/* Data chunk */
	uint8_t chunk_len;
	uint8_t *d;

	_bite_debug_push(self, "bite_write");

	/* Get destination data buffer */
	d = _bite_get_dst_buf(self, &chunk_len);
	
	if (d != NULL) {
		_bite_debug_bin(self, "source data      ", data);
		_bite_debug_int(self, "offset (from LSB)", ofs);
		_bite_debug_int(self, "bits to write    ", chunk_len);
	}

	if (d == NULL) {
		/* Nothing to do here */
		_bite_debug_str(self, "ERR: destination data fetch failed!");
	} else if (ofs == 0U) {
		_bite_debug_str(self, "");
		_bite_debug_str(self, "INFO: chunk is 8bit aligned");
		if (chunk_len < 8U) {
			_bite_debug_str(self, "INFO: chunk end is short");
			_bite_debug_str(self, "");
			_bite_debug_int(self, "chunk_len", chunk_len);

			_bite_debug_bin(self, "destination data ", d[0]);
			d[0] = data << (8U - chunk_len);
			_bite_debug_bin(self, "write result     ", d[0]);
		} else {
			_bite_debug_str(self, "");
			_bite_debug_bin(self, "destination data ", d[0]);
			d[0] = data;
			_bite_debug_bin(self, "write result     ", d[0]);
		}
	} else if (chunk_len > ofs) {
		/* Data is split into MSB and LSB parts */
		uint8_t msb_len = ofs;
		uint8_t lsb_len = chunk_len - msb_len;

		/* Masks to bound our data */
		uint8_t mask_msb = 0xFFU << msb_len;
		uint8_t mask_lsb = 0xFFU >> lsb_len;

		/* Temporary variables */
		uint8_t src;
		uint8_t dst;

		/* MSB goes into first byte */
		_bite_debug_str(self, "");
		_bite_debug_str(self, "INFO: chunk is fragmented");
		_bite_debug_str(self, "");

		_bite_debug_int(self, "MSB len       ", msb_len);
		_bite_debug_bin(self, "MSB mask      ", mask_msb);
		
		dst = (d[0] & mask_msb);
		_bite_debug_bin(self, "dst[0] data   ", d[0]);
		_bite_debug_bin(self, "dst[0] masked ", dst);
		
		src = (data >> lsb_len);
		_bite_debug_bin(self, "src data      ", data);
		_bite_debug_bin(self, "src shifted   ", src);
		src &= (uint8_t)~mask_msb;
		_bite_debug_bin(self, "src masked    ", src);

		d[0] = dst | src;
		_bite_debug_bin(self, "result (MSB)  ", d[0]);

		_bite_debug_str(self, "");

		/* LSB goes into second byte */
		_bite_debug_int(self, "LSB len       ", lsb_len);
		_bite_debug_bin(self, "LSB mask      ", mask_lsb);
		
		dst = (d[1] & mask_lsb);
		_bite_debug_bin(self, "dst[1] data   ", d[1]);
		_bite_debug_bin(self, "dst[1] masked ", dst);
		
		src = (data << (8U - lsb_len));
		_bite_debug_bin(self, "src data      ", data);
		_bite_debug_bin(self, "src shifted   ", src);
		src &= (uint8_t)~mask_lsb;
		_bite_debug_bin(self, "src masked    ", src);
		
		d[1] = dst | src;
		_bite_debug_bin(self, "result (LSB)  ", d[1]);
	} else {
		/* Data fits into one byte */
		uint8_t mask = (0xFFU <<              ofs) ^
			       (0xFFU << (ofs - chunk_len));

		/* Temporary variables */
		uint8_t src;
		uint8_t dst;

		_bite_debug_str(self, "");
		_bite_debug_str(self, "INFO: chunk is a bitfield");
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

void bite_write_le(struct bite *self, uint8_t data)
{
	/* Offset from LSB */
	uint8_t ofs = self->_ofs_bits % 8U;
	
	/* Data chunk */
	uint8_t chunk_len;
	uint8_t *d;

	_bite_debug_push(self, "bite_write");

	/* Get destination data buffer */
	d = _bite_get_dst_buf(self, &chunk_len);
	
	if (d != NULL) {
		_bite_debug_bin(self, "source data      ", data);
		_bite_debug_int(self, "offset (from LSB)", ofs);
		_bite_debug_int(self, "bits to write    ", chunk_len);
	}

	if (d == NULL) {
		/* Nothing to do here */
		_bite_debug_str(self, "ERR: destination data fetch failed!");
	} else if (ofs == 0U) {
		_bite_debug_str(self, "");
		_bite_debug_str(self, "INFO: chunk is 8bit aligned");
		if (chunk_len < 8U) {
			_bite_debug_str(self, "INFO: chunk end is short");
			_bite_debug_str(self, "");
			_bite_debug_int(self, "chunk_len", chunk_len);

			_bite_debug_bin(self, "destination data ", d[0]);
			d[0] = data & (0xFFU >> (8U - chunk_len));
			_bite_debug_bin(self, "write result     ", d[0]);
		} else {
			_bite_debug_str(self, "");
			_bite_debug_bin(self, "destination data ", d[0]);
			d[0] = data;
			_bite_debug_bin(self, "write result     ", d[0]);
		}
	} else if ((ofs + chunk_len) > 8U) {
		/* Data is split into MSB and LSB parts */
		uint8_t msb_len = (ofs + chunk_len) - 8U;
		uint8_t lsb_len = 8U - ofs;

		/* Masks to bound our data */
		uint8_t mask_msb = 0xFFU << msb_len;
		uint8_t mask_lsb = 0xFFU >> lsb_len;

		/* Temporary variables */
		uint8_t src;
		uint8_t dst;

		/* MSB goes into first byte */
		_bite_debug_str(self, "");
		_bite_debug_str(self, "INFO: chunk is fragmented");
		_bite_debug_str(self, "");

		_bite_debug_int(self, "LSB len       ", msb_len);
		_bite_debug_bin(self, "LSB mask      ", mask_msb);
		
		dst = (d[1] & mask_msb);
		_bite_debug_bin(self, "dst[1] data   ", d[1]);
		_bite_debug_bin(self, "dst[1] masked ", dst);
		
		src = (data >> lsb_len);
		_bite_debug_bin(self, "src data      ", data);
		_bite_debug_bin(self, "src shifted   ", src);
		src &= (uint8_t)~mask_msb;
		_bite_debug_bin(self, "src masked    ", src);

		d[1] = dst | src;
		_bite_debug_bin(self, "result (LSB)  ", d[1]);

		_bite_debug_str(self, "");

		/* LSB goes into second byte */
		_bite_debug_int(self, "MSB len       ", lsb_len);
		_bite_debug_bin(self, "MSB mask      ", mask_lsb);
		
		dst = (d[0] & mask_lsb);
		_bite_debug_bin(self, "dst[0] data   ", d[0]);
		_bite_debug_bin(self, "dst[0] masked ", dst);
		
		src = (data << (8U - lsb_len));
		_bite_debug_bin(self, "src data      ", data);
		_bite_debug_bin(self, "src shifted   ", src);
		src &= (uint8_t)~mask_lsb;
		_bite_debug_bin(self, "src masked    ", src);
		
		d[0] = dst | src;
		_bite_debug_bin(self, "result (MSB)  ", d[0]);
	} else {
		/* Data fits into one byte */
		uint8_t mask = ((uint8_t)~(0xFFU << chunk_len)) << ofs;

		/* Temporary variables */
		uint8_t src;
		uint8_t dst;

		_bite_debug_str(self, "");
		_bite_debug_str(self, "INFO: chunk is a bitfield");
		_bite_debug_str(self, "");
		_bite_debug_bin(self, "mask       ", mask);
		
		dst = (d[0] & (uint8_t)~mask);
		_bite_debug_bin(self, "dst data   ", d[0]);
		_bite_debug_bin(self, "dst masked ", dst);
		
		src = data << ofs;
		_bite_debug_bin(self, "src data   ", data);
		_bite_debug_bin(self, "src shifted", src);
		src &= mask;
		_bite_debug_bin(self, "src masked ", src);

		d[0] = dst | src;
		_bite_debug_bin(self, "result     ", d[0]);
	}

	_bite_debug_pop(self);
}

uint8_t bite_read(struct bite *self)
{
	uint8_t r = 0;

	/* Offset from LSB */
	uint8_t ofs = (self->_ofs_bits + 1U) % 8U;
	
	/* Data chunk */
	uint8_t chunk_len;
	uint8_t *d;

	_bite_debug_push(self, "bite_read");

	/* Read data chunk */
	*d = _bite_get_dst_buf(self, &chunk_len);

	if (d == NULL) {
		/* Nothing to do here */
	} else if (ofs == 0U) {
		if (chunk_len < 8U) {
			r = d[0] >> (8U - chunk_len);
		} else {
			r = d[0];
		}
	} else if (chunk_len > ofs) {
		/* Data is split into MSB and LSB parts */
		uint8_t msb_len = ofs;
		uint8_t lsb_len = chunk_len - msb_len;

		/* Masks to bound our data */
		uint8_t mask_msb = 0xFFU << msb_len;
		uint8_t mask_lsb = 0xFFU >> lsb_len;

		r = ((d[0] & (uint8_t)~mask_msb) <<       lsb_len) |
		    ((d[1] & (uint8_t)~mask_lsb) >> (8U - lsb_len));
	} else {
		uint8_t mask = (0xFFU <<               ofs) ^
			       (0xFFU << (ofs - chunk_len));

		r = (d[0] & mask) >> (ofs - chunk_len);
	}

	_bite_debug_pop(self);

	return r;
}

void bite_set_flag(struct bite *self, size_t bit_idx, bool set)
{
	uint8_t  mask = 1U << (uint8_t)(bit_idx % 8U);
	uint8_t *data = &self->_data[bit_idx / 8U];

	*data = set ? (*data | mask) : (*data & (uint8_t)~mask);
}

bool bite_get_flag(struct bite *self, size_t bit_idx)
{
	uint8_t mask = 1U << (uint8_t)(bit_idx % 8U);
	uint8_t data = self->_data[bit_idx / 8U];

	return (data & mask) > 0U;
}
