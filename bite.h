#include <stdint.h>
#include <stdbool.h>
#include <assert.h>

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

uint8_t bite_mix_u8(uint8_t left, uint8_t offset, uint8_t right)
{
	uint8_t result;

	/* Note for bitwise operators from C/C++ specs:
	 *	If the value of the right operand is negative or is greater
	 * 	than or equal to the width of the promoted left operand,
	 * 	the behavior is undefined. */

	if (offset >= 8U) {
		result = left;
	} else if 
	   (offset == 0U) {
		result = right;
	} else {
		uint8_t mask = 0xFFU >> offset;
		result = (left & (uint8_t)~mask) | (right & mask);
	}

	return result;
}

/* Check if range is bitwise misaligned */
bool bite_range_is_byte_aligned(struct bite *self)
{
	return !( ( ( self->_ofs_bits % 8U)                    != 0U) ||
		  ( ((self->_ofs_bits + self->_len_bits) % 8U) != 0U)
		);
}

/* Check if data segment starting at bit offset
 * within byte fits inside 1 byte */
bool bite_range_fits_one_byte(struct bite *self)
{
	return (((self->_ofs_bits % 8U) + self->_len_bits) <= 8U);
}

void bite_catch_overflow(struct bite *self)
{
	if (self->_iter_bits >= self->_len_bits) {
		self->_state = (uint8_t)-1;
		self->flags |= BITE_FLAG_OVERFLOW;
	}
}

void bite_init(struct bite *self, uint8_t *buf/*, size_t len*/)
{
	self->_state = 0U;

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
	
	if (!bite_range_is_byte_aligned(self)) {
		if (bite_range_fits_one_byte(self)) {
			self->_state = 1U;
		} else {
			self->_state = 2U;
		}
	} else {
		self->_state = 0U;
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
	bite_catch_overflow(self);
	
	switch (self->_state) {
	case 0U: /* Simplest case, no misalignment */
		self->_data[(self->_ofs_bits + self->_iter_bits) / 8U] = data;
		
		break;

	case 1U: { /* Misaligned, but fits one byte */
		uint8_t *d = &self->_data[
				(self->_ofs_bits + self->_iter_bits) / 8U];
		
		uint8_t ofs_from_msb = self->_ofs_bits % 8U;
		uint8_t ofs_from_lsb = (8U - ofs_from_msb - self->_len_bits);

		uint8_t mask = ((1U << self->_len_bits) - 1U) << ofs_from_lsb;
	
		*d = (*d & (uint8_t)~mask) | ((data << ofs_from_lsb) & mask);

		break;
	}
		
	case 2U: { /* Multibyte misalignment */
		uint8_t *d = &self->_data[
				(self->_ofs_bits + self->_iter_bits) / 8U];

		uint8_t ofs_from_msb = self->_ofs_bits % 8U;
		uint8_t ofs_from_lsb = (8U - ofs_from_msb);

		d[0] = bite_mix_u8(d[0], ofs_from_msb, data >> ofs_from_msb);
		d[1] = bite_mix_u8(data << ofs_from_lsb, ofs_from_msb, d[1]);

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

	bite_catch_overflow(self);
	
	switch (self->_state) {
	case 0U: /* Simplest case, no misalignment */
		r = self->_data[(self->_ofs_bits + self->_iter_bits) / 8U];
		
		break;

	case 1U: { /* Misaligned, but fits one byte */
		uint8_t *d = &self->_data[
				(self->_ofs_bits + self->_iter_bits) / 8U];
		
		uint8_t ofs_from_msb = self->_ofs_bits % 8U;
		uint8_t ofs_from_lsb = (8U - ofs_from_msb - self->_len_bits);

		uint8_t mask = ((1U << self->_len_bits) - 1U) << ofs_from_lsb;
	
		r = (*d & (uint8_t)mask) >> ofs_from_lsb;

		break;
	}

	case 2U: { /* Multibyte misalignment */
		uint8_t *d = &self->_data[
				(self->_ofs_bits + self->_iter_bits) / 8U];

		uint8_t ofs_from_msb = self->_ofs_bits % 8U;
		uint8_t ofs_from_lsb = (8U - ofs_from_msb);

		/* Protection from undefined behaviour 
		 * WARNING FLAWED LOGIC, FIXME */
		if (ofs_from_msb == 0U) {
			r = d[0];
		} else {
			r = (d[0] << ofs_from_msb) | (d[1] >> ofs_from_lsb);
		}

		break;
	}

	default:
		break;
	}

	self->_iter_bits += 8U;

	return r;
}
