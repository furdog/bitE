# bitE
`bitE` is a binary data serializer which uses `.dbc` -like format to read and write data into `uint8_t` arrays.
It is primarily used as a helper tool to prevent user from manualy managing bit arrays, especially if there's '`.dbc`' description available.
It's particularly useful for handling data formats like those used in automotive CAN networks.

For example `.dbc` variable: `SG_ PCS_dcdcHvBusVolt m6: 16|12@1+ (0.146484,0) [0|0] "V" X`
may be expressed as: `struct bite b_obj = bite_init(u8array, BITE_ORDER_DBC1, 16, 12);`
then it may be written or read, using `bite_put_u8(struct bite *self, uint8_t data)` or `bite_get_u8(struct bite *self)`.
bitE will read `8` or `less` bits and will increment its internal pointer to the next relevant byte inside `u8array`.
If there's less than 8 bits available, `MSB` bits will be masked with zeros.

## Danger
⚠️There are no data conversion performed. All data read or written are just integers, either signed or unsigned.
It's up to user to interpretet data, parsing offsets and multipliers.

⚠️There are no `.dbc` parsing involved. It's up to user to manually express `.dbc` data, performing `bitE` calls.

## Usage
1. **Initialize the `bite` struct:**
Use `bite_init()` to set up a `bite` object. You'll need to provide the data buffer, endianness, start bit, and length in bits.
```c
#include "bite.h"

uint8_t my_buffer[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
struct bite my_data;

...

/* Initialize a little-endian bit field starting at bit 12, with a length of 16 bits */
my_data = bite_init(my_buffer, BITE_ORDER_LITTLE_ENDIAN, 12, 16);
```

2. **Read or Write Data:**
    Use the provided functions to get or put data of different sizes. The `bite` struct's internal state is automatically updated after each operation, allowing you to chain calls to process consecutive bit fields.
```c
bite_put_u8(struct bite *self, uint8_t data);
bite_get_u8(struct bite *self);
bite_get_i8(struct bite *self);
bite_put_u16(struct bite *self, uint16_t data);
bite_get_u16(struct bite *self);
bite_get_i16(struct bite *self);
bite_put_u32(struct bite *self, uint32_t data);
bite_get_u32(struct bite *self);
bite_get_i32(struct bite *self);

...

/* Write a 16-bit value */
bite_put_u16(&my_data, 0xABCD);

/* Read a 16-bit value */
uint16_t value = bite_get_u16(&my_data);
 ```

## Implementation notes
> 01.06.2026

I plan to eliminate all dangers and enhance functionality by:
- Explicit bound checking
- Creating internal `.dbc` variable database.
- Creating simple `.dbc` parser which automatically adds variable entries to internal database.

I have found a bug, pointed by fanalyzer. Looks like there are out of bound access during normal tests…
Can't confirm yet.

> 02.06.2026

I have wrote more docs and comments inside code

I have found that static analyzer was just extra cautios about boundary checks. Everything is safe after all.
i have added a check `assert((self->order == 1) || (self->buf > self->ori));`, but i think it's better to change approach.