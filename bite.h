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
};

/******************************************************************************
 * PRIVATE
 *****************************************************************************/
uint8_t *_bite_get_dst_data_u8(struct bite *self, uint8_t *chunk_len)
{
	uint8_t *result = NULL;

	if (self->_iter_bits >= self->_len_bits) {
		self->flags |= BITE_FLAG_OVERFLOW;
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
	
	self->_order = BITE_ORDER_LIL_ENDIAN;
}

void bite_begin(struct bite *self, size_t ofs_bits, size_t len_bits)
{
	if (self->_iter_bits < self->_len_bits) {
		self->flags |= BITE_FLAG_UNDERFLOW;
	} else {
		self->flags = 0U;
	}

	self->_ofs_bits = ofs_bits;
	self->_len_bits = len_bits;

	self->_iter_bits = 0U;
}

void bite_end(struct bite *self)
{	
	if (self->_iter_bits < self->_len_bits) {
		self-> flags |= BITE_FLAG_UNDERFLOW;
	}
}

void bite_rewind(struct bite *self)
{
	self->_iter_bits = 0U;
}

void bite_write(struct bite *self, uint8_t data)
{
	/* Offset from LSB */
	uint8_t ofs = (self->_ofs_bits + 1U) % 8U;
	
	/* Read data chunk */
	uint8_t chunk_len;
	uint8_t *d = _bite_get_dst_data_u8(self, &chunk_len);
	
	if (d == NULL) {
		/* Nothing to do here */
	} else if (ofs == 0U) {
		if (chunk_len < 8U) {
			d[0] = data << (8U - chunk_len);
		} else {
			d[0] = data;
		}
	} else if (chunk_len > ofs) {
		/* Data is split into MSB and LSB parts */
		uint8_t msb_len = ofs;
		uint8_t lsb_len = chunk_len - msb_len;

		/* Masks to bound our data */
		uint8_t mask_msb = 0xFFU << msb_len;
		uint8_t mask_lsb = 0xFFU >> lsb_len;

		/* MSB goes into first byte */
		d[0] = (d[0] & mask_msb) |
		      ((data >> lsb_len) & (uint8_t)~mask_msb);

		/* LSB goes into second byte */
		d[1] = (d[1] & mask_lsb) |
		      ((data << (8U - lsb_len)) & (uint8_t)~mask_lsb);
	} else {
		/* Data fits into one byte */
		uint8_t mask = (0xFFU <<              ofs) ^
			       (0xFFU << (ofs - chunk_len));
		
		d[0] = (d[0] & (uint8_t)~mask) |
		      ((data << (ofs - chunk_len)) & mask);
	}
}

uint8_t bite_read(struct bite *self)
{
	uint8_t r = 0;

	/* Offset from LSB */
	uint8_t ofs = (self->_ofs_bits + 1U) % 8U;
	
	/* Read data chunk */
	uint8_t chunk_len;
	uint8_t *d = _bite_get_dst_data_u8(self, &chunk_len);

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
