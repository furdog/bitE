#include <stdint.h>
#include <stdbool.h>
#include <assert.h>

/******************************************************************************
 * CLASS
 *****************************************************************************/
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

	if ((self->_ofs_bits % 8U) != 0U) {
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
	uint8_t *d;
	uint8_t ofs       = self->_ofs_bits % 8U;
	uint8_t bits_left = self->_len_bits - self->_iter_bits;

	d = _bite_get_dst_data_u8(self);
	
	switch (self->_state) {
	case BITE_STATE_ALIGNED:
		if (bits_left >= 8U) {
			/* Just insert byte as it is */
			d[0] = data;
		} else {
			/* If there are still bits left,
			 * Shift them to the left part of the byte (MSB) */
			d[0] = data << (8U - bits_left);

			/* example: 00001101 -> 11010000 */
		}

		self->_iter_bits += 8U;
		break;

	case BITE_STATE_UNALIGNED: {
		/* How many uncarried bits are left? */
		uint8_t uncarried = 8U - ofs;

		/* Check if we have to carry remaining bits */
		if (bits_left > uncarried) {
			/* How many bits are carried to the next byte? */
			uint8_t carried;

			/* Masks for left and right bytes */
			uint8_t mask_l;
			uint8_t mask_r;

			if (bits_left >= 8U) {
				/* We carry exactly OFFSET of bits, if there
				 * are more data to get from stream */
				carried = ofs;
			} else {
				/* We only carry remaining bits. */
				carried = (bits_left - uncarried);
			}

			/* Apply masks */
			mask_l = 0xFFU << uncarried;
			mask_r = 0xFFU >> carried;

			d[0] = (d[0] & mask_l) |
				((data >>       carried)  & (uint8_t)~mask_l);

			d[1] = (d[1] & mask_r) |
				((data << (8U - carried)) & (uint8_t)~mask_r);
		} else {
			uint8_t mask = (0xFFU >>              ofs) ^
				       (0xFFU >> (ofs + bits_left));
			
			d[0] = (d[0] & (uint8_t)~mask) |
				((data << (8U - (ofs + bits_left))) & mask);
		}

		self->_iter_bits += 8U;
		break;
	}

	default:
		break;
	}
}

uint8_t bite_read(struct bite *self)
{
	uint8_t r = 0;

	uint8_t *d;
	uint8_t ofs = self->_ofs_bits % 8U;
	uint8_t bits_left = self->_len_bits - self->_iter_bits;

	d = _bite_get_dst_data_u8(self);

	switch (self->_state) {
	case BITE_STATE_ALIGNED:
		if (bits_left >= 8U) {
			r = d[0];
		} else {
			r = d[0] >> (8U - bits_left);
		}

		self->_iter_bits += 8U;
		break;

	case BITE_STATE_UNALIGNED: {
		/* How many uncarried bits are left? */
		uint8_t uncarried = 8U - ofs;

		/* Check if we have to carry remaining bits */
		if (bits_left > uncarried) {
			/* How many bits are carried to the next byte? */
			uint8_t carried;

			/* Masks for left and right bytes */
			uint8_t mask_l;
			uint8_t mask_r;

			if (bits_left >= 8U) {
				/* We carry exactly OFFSET of bits, if there
				 * are more data to get from stream */
				carried = ofs;
			} else {
				/* We only carry remaining bits. */
				carried = (bits_left - uncarried);
			}

			/* Apply masks */
			mask_l = 0xFFU << uncarried;
			mask_r = 0xFFU >> carried;

			r = ((d[0] & (uint8_t)~mask_l) << (8U - uncarried)) |
			    ((d[1] & (uint8_t)~mask_r) >> (8U - carried));
		} else {
			uint8_t mask = (0xFFU >>              ofs) ^
				       (0xFFU >> (ofs + bits_left));

			r = (d[0] & mask) >> (8U - (ofs + bits_left));
		}

		self->_iter_bits += 8U;
		break;
	}

	default:
		break;
	}

	return r;
}
