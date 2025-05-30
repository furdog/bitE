#include <stdint.h>
#include <stdbool.h>
#include <assert.h>

enum bite_state
{
	BITE_STATE_INVALID    = (uint8_t)-1,
	BITE_STATE_ALIGNED    = 0U,
	BITE_STATE_MISALIGNED = 1U
};

enum bite_flags {
	BITE_FLAG_NONE,
	
	/* Read or write operation has exceed configured boundary */
	BITE_FLAG_OVERFLOW  = 1U,
	
	/* Configure call during unfinished read or write operation */
	BITE_FLAG_UNDERFLOW = 2U
};

struct bite {
	uint8_t _state;

	/* Pointer to external buffer */
	uint8_t *_data;
	/* size_t   _len; */

	/* Config */
	size_t _ofs_bits;
	size_t _len_bits;

	/* Runtime */
	uint8_t flags;

	size_t  _iter_bits;
};

uint8_t *bite_get_dst_data_u8(struct bite *self)
{
	return &self->_data[(self->_ofs_bits + self->_iter_bits) / 8U];
}

void bite_catch_overflow(struct bite *self)
{
	if (self->_iter_bits >= self->_len_bits) {
		self->_state = BITE_STATE_INVALID;
		self->flags |= BITE_FLAG_OVERFLOW;
	}
}

void bite_init(struct bite *self, uint8_t *buf/*, size_t len*/)
{
	self->_state = BITE_STATE_INVALID;

	self->_data = buf;
	/* size_t   _len; */

	self->_ofs_bits = 0U;
	self->_len_bits = 0U;

	/* Runtime */
	self->flags = 0U;

	self->_iter_bits = 0U;
}

void bite_config(struct bite *self, size_t ofs_bits, size_t len_bits)
{
	if (self->_iter_bits < self->_len_bits) {
		self->flags = BITE_FLAG_UNDERFLOW;
	} else {
		self->flags = 0U;
	}

	self->_ofs_bits = ofs_bits;
	self->_len_bits = len_bits;

	if ((self->_ofs_bits % 8U) != 0U) {
		self->_state = BITE_STATE_MISALIGNED;
	} else {
		self->_state = BITE_STATE_ALIGNED;
	}

	self->_iter_bits = 0U;
}

void bite_reset(struct bite *self)
{
	bite_config(self, self->_ofs_bits, self->_len_bits);
}

/* WARNING: Offset SHALL be >= 0 and < 8 and to prevent UB
   WARNING: Multiple use of macro arguments
   WARNING: Multiple len + ofs shall not exceed 7
 * Offset is from MSB */
#define BITE_MASK_U8(ofs, len) \
	((0xFFU >> (ofs)) ^ (0xFFU >> ((ofs) + (len))))

#define BITE_COPY_U8(src, dst, ofs, len)                        \
	do {                                                    \
		uint8_t mask = BITE_MASK_U8(ofs, len);          \
		(dst) = ((dst) &       (uint8_t)~mask) |        \
			((src) << (8U - (ofs) - (len)) & mask); \
	} while (false)

/* Insert data byte by byte */
void bite_write(struct bite *self, uint8_t data)
{
	uint8_t *d;
	uint8_t ofs = self->_ofs_bits % 8U;
	uint8_t bits_left = self->_len_bits - self->_iter_bits;
		
	bite_catch_overflow(self);
	
	d = bite_get_dst_data_u8(self);
	
	switch (self->_state) {
	case BITE_STATE_ALIGNED:
		if (bits_left >= 8U) {
			d[0] = data;
		} else {
			/* Copy/shift remaining bits into MSB */
			BITE_COPY_U8(data, d[0], 0U, bits_left);
		}

		break;
		
	case BITE_STATE_MISALIGNED: {
		/* Check if we have to carry remaining LSB bits */
		if (bits_left > (8U - ofs)) {
			/* Split onto two parts */
			BITE_COPY_U8(data >> ofs, d[0], ofs, 8U - ofs);
			BITE_COPY_U8(data,        d[1],  0U,      ofs);
		} else {
			/* Fit into single byte */
			BITE_COPY_U8(data, d[0], ofs, self->_len_bits);
		}

		break;
	}
	
	default:
		break;
	}

	self->_iter_bits += 8U;
}

/* Insert data byte by byte */
uint8_t bite_read(struct bite *self)
{
	uint8_t r = 0;

	uint8_t *d;
	uint8_t ofs = self->_ofs_bits % 8U;
	uint8_t bits_left = self->_len_bits - self->_iter_bits;
		
	bite_catch_overflow(self);
	
	d = bite_get_dst_data_u8(self);
	
	switch (self->_state) {
	case BITE_STATE_ALIGNED:
		if (bits_left >= 8U) {
			r = d[0];
		} else {
			/* Copy/shift remaining bits into LSB */
			BITE_COPY_U8(d[0], r, 8U - bits_left, bits_left);
		}

		break;

	case BITE_STATE_MISALIGNED: {
		/* Check if we have to carry remaining LSB bits */
		if (bits_left > (8U - ofs)) {
			/* Split onto two parts */
	
		} else {
			/* Fit into single byte */
	
		}

		break;
	}

	default:
		break;
	}

	self->_iter_bits += 8U;

	return r;
}
