#include <stdint.h>
#include <stdbool.h>
#include <assert.h>

enum bite_state
{
	BITE_STATE_INVALID   = (uint8_t)-1,
	BITE_STATE_ALIGNED   = 0U,
	BITE_STATE_UNALIGNED = 1U
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

bool bite_catch_overflow(struct bite *self)
{
	bool result = false;

	if (self->_iter_bits >= self->_len_bits) {
		self->_state = BITE_STATE_INVALID;
		self->flags |= BITE_FLAG_OVERFLOW;

		result = true;
	}

	return result;
}

uint8_t *bite_get_dst_data_u8(struct bite *self)
{
	uint8_t *result = NULL;

	/* Only read data if no overflow detected */
	if (!bite_catch_overflow(self)) {
		result = &self->_data[
				(self->_ofs_bits + self->_iter_bits) / 8U
			 ];
	}
	
	return result;
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
		self->_state = BITE_STATE_UNALIGNED;
	} else {
		self->_state = BITE_STATE_ALIGNED;
	}

	self->_iter_bits = 0U;
}

void bite_reset(struct bite *self)
{
	bite_config(self, self->_ofs_bits, self->_len_bits);
}

/* Insert data byte by byte */
void bite_write(struct bite *self, uint8_t data)
{
	uint8_t *d;
	uint8_t ofs = self->_ofs_bits % 8U;
	uint8_t bits_left = self->_len_bits - self->_iter_bits;

	d = bite_get_dst_data_u8(self);
	
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

/* Insert data byte by byte */
uint8_t bite_read(struct bite *self)
{
	uint8_t r = 0;

	uint8_t *d;
	uint8_t ofs = self->_ofs_bits % 8U;
	uint8_t bits_left = self->_len_bits - self->_iter_bits;

	d = bite_get_dst_data_u8(self);

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

			print_binary(r, 8);
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
