#include <stdint.h>
#include <stdbool.h>
#include <assert.h>

enum bite_flags {
	BITE_FLAG_NONE,
	BITE_FLAG_OVERFLOW = 1U
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

uint8_t bite_mask_from_msb(struct bite *self)
{
	uint8_t ofs_from_msb = self->_ofs_bits % 8U;
	uint8_t len = self->_len_bits;

	return ((1U << len) - 1U) << (8U - ofs_from_msb - len);
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
	
	self->flags = 0U;
	
	self->_iter_bits = 0U;
}

/* Insert data byte by byte */
void bite_insert(struct bite *self, uint8_t data)
{
	switch (self->_state) {
	case 0U: /* Simplest case, no misalignment */
		if (self->_iter_bits >= self->_len_bits) {
			self->flags |= BITE_FLAG_OVERFLOW;
			break;
		}

		self->_data[(self->_ofs_bits + self->_iter_bits) / 8U] = data;

		self->_iter_bits += 8U;
	
		break;

	case 1U: { /* Misaligned, but fits one byte */
		uint8_t *d = &self->_data[
				(self->_ofs_bits + self->_iter_bits) / 8U];
		
		uint8_t mask = bite_mask_from_msb(self);
		
		print_binary(mask, 8);
	
		*d = (*d & (uint8_t)~mask) |
		     ((data << (8 - self->_ofs_bits - self->_len_bits)) & mask);

		break;
	}
		
	case 2U: /* Start-End misalignment */
		assert(0);
		break;	

	default:
		break;
	}
}
