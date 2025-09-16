# bitE

<p align="center">
  <img src="logo.jpg" height="300" alt="Your image description" />
</p>

-----

A simple C library for reading and writing data of various bit lengths and endianness from a byte array. It's particularly useful for handling data formats like those used in automotive CAN networks.

-----

### Key Features ⚙️

  * **Bit-level Operations:** Read and write data at the bit level, not just byte boundaries.
  * **Endianness Support:** Handles both **little-endian** (`BITE_ORDER_LITTLE_ENDIAN`, `BITE_ORDER_INTEL`) and **big-endian** (`BITE_ORDER_BIG_ENDIAN`, `BITE_ORDER_MOTOROLA`) data.
  * **DBC Compatibility:** Designed with CAN DBC (Database) indexing in mind, where the start bit for a signal can be non-byte-aligned.
  * **Simple API:** A high-level, `struct`-based API for easy integration.

-----

### Usage

1.  **Initialize the `bite` struct:**
    Use `bite_init()` to set up a `bite` object. You'll need to provide the data buffer, endianness, start bit, and length in bits.

    ```c
    #include "bite.h"

    uint8_t my_buffer[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    struct bite my_data;

    /* Initialize a little-endian bit field starting at bit 12, with a length of 16 bits */
    my_data bite_init(my_buffer, BITE_ORDER_LITTLE_ENDIAN, 12, 16);
    ```

2.  **Read or Write Data:**
    Use the provided functions to get or put data of different sizes. The `bite` struct's internal state is automatically updated after each operation, allowing you to chain calls to process consecutive bit fields.

      * `bite_put_u8(struct bite *self, uint8_t data)`
      * `bite_get_u8(struct bite *self)`
      * `bite_put_u16(struct bite *self, uint16_t data)`
      * `bite_get_u16(struct bite *self)`
      * `bite_get_i16(struct bite *self)`
      * `bite_put_u32(struct bite *self, uint32_t data)`
      * `bite_get_u32(struct bite *self)`
      * `bite_get_i32(struct bite *self)`

    <!-- end list -->

    ```c
    /* Write a 16-bit value */
    bite_put_u16(&my_data, 0xABCD);

    /* Read a 16-bit value */
    uint16_t value = bite_get_u16(&my_data);
    ```

-----

### Dependencies

  * `stdbool.h`
  * `stdint.h`
