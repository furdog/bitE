#include <stdint.h>
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
	
	uint16_t _chunk;
};

void bite_init(struct bite *self, uint8_t *buf/*, size_t len*/)
{
	self->_state = 0U;
	
	self->_data = buf;
	/* self->_len  = len; */
}

void bite_config(struct bite *self, size_t ofs_bits, size_t len_bits)
{
	self->_ofs_bits = ofs_bits;
	self->_len_bits = len_bits;
	
	self->_state = 0U;
	
	if ((ofs_bits % 8U) != 0U) {
		self->_state = 1U;
	}

	if (((ofs_bits + len_bits) % 8U) != 0U)
	{
		self->_state = 1U;
	}
	
	self->flags = 0U;
	
	self->_chunk = self->_data[self->_ofs_bits / 8U] >>
							(8U - (ofs_bits % 8U));
}

/* Insert data byte by byte */
void bite_insert(struct bite *self, uint8_t data)
{
	if (self->_len_bits == 0U) {
		self->flags |= BITE_FLAG_OVERFLOW;
		return;
	}
	
	switch (self->_state) {
	case 0U: /* Simplest case, no misalignment */
		self->_data[self->_ofs_bits / 8U] = data;

		break;

	case 1U: /* Start-End misalignment */
		self->_chunk <<= 8U;
		self->_chunk  |= (uint16_t)data;

		self->_data[self->_ofs_bits / 8U] = self->_chunk >>
						    (self->_ofs_bits % 8U);

		if (self->_len_bits <= 8U) {
			self->_chunk <<= (8 - (self->_ofs_bits % 8U));

			print_binary(self->_chunk, 16);
			print_binary(0x34, 16);
			printf("%i\n", self->_len_bits);

			uint8_t *data = &self->_data[
						 (self->_ofs_bits / 8U) + 1U];
			*data &= 0xFF >> (self->_ofs_bits % self->_len_bits);
			
			*data |= self->_chunk;

			self->_len_bits = 8U;

			break;
		}
		
		break;

	default:
		break;
	}

	self->_ofs_bits += 8U;
	self->_len_bits -= 8U;
}
