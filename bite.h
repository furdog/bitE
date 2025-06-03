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

enum bite_state
{
	BITE_STATE_INVALID   = (uint8_t)-1,
	BITE_STATE_ALIGNED   = 0U,
	BITE_STATE_UNALIGNED = 1U
};

enum bite_flags {
	BITE_FLAG_NONE,
	
	/* Read or write operation has exceed configured boundary */
	BITE_FLAG_OVERFLOW    = 1U,
	
	/* Configure call during unfinished read or write operation */
	BITE_FLAG_UNDERFLOW   = 2U,

	/* Incorect use of API */
	BITE_FLAG_INVALID_USE = 4U
};

struct bite {
	/* Config */
	uint8_t *_data;

	size_t _ofs_bits;
	size_t _len_bits;

	/* Runtime */
	size_t  _iter_bits;

	uint8_t _state;
	uint8_t  flags;
	
	uint8_t _order;
};

/******************************************************************************
 * PRIVATE
 *****************************************************************************/
uint8_t *_bite_get_dst_data_u8(struct bite *self)
{
	uint8_t *result = NULL;

	/* Check state */
	if (self->_state == (uint8_t)BITE_STATE_INVALID) {
		self-> flags |= BITE_FLAG_INVALID_USE;
	/* Check if not out of allowed bounds */
	} else if (self->_iter_bits >= self->_len_bits) {
		self->_state = BITE_STATE_INVALID;
		self->flags |= BITE_FLAG_OVERFLOW;
	/* Read data if everything is ok */
	} else {
		result = &self->_data[
				(self->_ofs_bits + self->_iter_bits) / 8U
			 ];
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

	self->_state = BITE_STATE_INVALID;
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

	if (((self->_ofs_bits + 1U) % 8U) != 0U) {
		self->_state = BITE_STATE_UNALIGNED;
	} else {
		self->_state = BITE_STATE_ALIGNED;
	}

	self->_iter_bits = 0U;
}

void bite_end(struct bite *self)
{	
	if (self->_iter_bits < self->_len_bits) {
		self->_state  = BITE_STATE_INVALID;
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
	uint8_t *d = _bite_get_dst_data_u8(self);
	uint8_t chunk_len = self->_len_bits - self->_iter_bits;

	/* Clamp chunk */
	if (chunk_len > 8U) { chunk_len = 8U; }
			
	switch (self->_state) {
	case BITE_STATE_ALIGNED:
		d[0] = data << (8U - chunk_len);
		self->_iter_bits += chunk_len;
		break;

	case BITE_STATE_UNALIGNED: {
		if (chunk_len > ofs) {
			/* Data is split into MSB and LSB parts */
			uint8_t msb_len = ofs;
			uint8_t lsb_len = chunk_len - msb_len;

			/* Masks to bound our data */
			uint8_t mask_msb = 0xFFU << msb_len;
			uint8_t mask_lsb = 0xFFU >> lsb_len;

			/* MSB goes into first byte */
			d[0] = (d[0] & mask_msb) |
				((data >> lsb_len)  & (uint8_t)~mask_msb);

			/* LSB goes into second byte */
			d[1] = (d[1] & mask_lsb) |
			       ((data << (8U - lsb_len)) & (uint8_t)~mask_lsb);
		} else {
			/* Data fits into one byte */
			uint8_t mask = (0xFFU <<                       ofs) ^
				       (0xFFU << (uint8_t)(ofs - chunk_len));
			
			d[0] = (d[0] & (uint8_t)~mask) |
			      ((data << (ofs - chunk_len)) & mask);
		}

		self->_iter_bits += chunk_len;
		break;
	}

	default:
		break;
	}
}

uint8_t bite_read(struct bite *self)
{
	uint8_t r = 0;

	/* Offset from LSB */
	uint8_t ofs = (self->_ofs_bits + 1U) % 8U;
	
	/* Read data chunk */
	uint8_t *d = _bite_get_dst_data_u8(self);
	uint8_t chunk_len = self->_len_bits - self->_iter_bits;

	/* Clamp chunk */
	if (chunk_len > 8U) { chunk_len = 8U; }

	switch (self->_state) {
	case BITE_STATE_ALIGNED:
		r = d[0] >> (8U - chunk_len);
		self->_iter_bits += chunk_len;
		break;

	case BITE_STATE_UNALIGNED: {
		if (chunk_len > ofs) {
			/* Data is split into MSB and LSB parts */
			uint8_t msb_len = ofs;
			uint8_t lsb_len = chunk_len - msb_len;

			/* Masks to bound our data */
			uint8_t mask_msb = 0xFFU << msb_len;
			uint8_t mask_lsb = 0xFFU >> lsb_len;

			r = ((d[0] & (uint8_t)~mask_msb) << (8U - msb_len)) |
			    ((d[1] & (uint8_t)~mask_lsb) >> (8U - lsb_len));
		} else {
			uint8_t mask = (0xFFU <<                       ofs) ^
				       (0xFFU << (uint8_t)(ofs - chunk_len));

			r = (d[0] & mask) >> (ofs - chunk_len);
		}

		self->_iter_bits += chunk_len;
		break;
	}

	default:
		break;
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
