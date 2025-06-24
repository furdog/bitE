# bitE

<p align="center"><img src="logo.jpg" /></p>

-----

## Overview

`bitE` is a versatile C library designed for **bit-wise read and write operations** on `uint8_t` buffers. It offers compatibility with the **CAN DBC format**, making it ideal for automotive and embedded systems development.

-----

## Key Features

  * **Robust Safety:** Includes rigorous input and boundary checks.
  * **Efficient Operations:** Ensures constant time complexity for every bit-wise operation.
  * **MISRA Compliance:** Achieves 100% MISRA compliance (verified with `cppcheck`).
  * **Pure C:** Written entirely in C (C89 standard).
  * **Linux Kernel Style:** Adheres to Linux kernel coding conventions.
  * **Zero Dependencies:** No external library dependencies.
  * **Single-Header:** Provided as a convenient single-header library.
  * **Automated Tests:** Comprehensive automated tests for reliability.
  * **Verbose Debugging:** Offers detailed, color-coded debug output for all operations, including binary representations and intermediate calculations.
  * **Stream-Oriented:** Supports byte-by-byte read/write operations.
  * **Pedantic Mode:** Enables assertions for stricter development and early error detection.

-----

## API Reference

### Initialization

```c
void bite_init(struct bite *self, uint8_t *buf, size_t size);
```

Initializes the `bitE` context with the provided data buffer and its size.

-----

### Begin / End Operations

```c
void bite_begin(struct bite *self, size_t ofs_bits, size_t len_bits,
		enum bite_order order);
```

Starts a bit-wise operation on a specified bit range, adhering to the CAN DBC format. The `order` parameter defines endianness:

```c
BITE_ORDER_BIG_ENDIAN = 0, /**< Big endian */
BITE_ORDER_MOTOROLA   = 0, /**< Big endian */
BITE_ORDER_DBC_0      = 0, /**< Big endian */

BITE_ORDER_LIL_ENDIAN = 1, /**< Little endian */
BITE_ORDER_INTEL      = 1, /**< Little endian */
BITE_ORDER_DBC_1      = 1  /**< Little endian */
```

  * This function may set various error flags.

-----

```c
void bite_end(struct bite *self);
```

Concludes the current bit-wise operation on a range.

  * This function may set various error flags.

-----

### Rewind Stream

```c
void bite_rewind(struct bite *self);
```

Resets the internal `bitE` stream iterator, causing subsequent read/write operations to start from the beginning of the defined range.

-----

### Write Bits

```c
void bite_write(struct bite *self, uint8_t data);
```

Writes up to 8 bits into the current range and advances the internal stream iterator. If fewer than 8 bits remain in the range, only the least significant bits (LSB) of `data` are written; excess most significant bits (MSB) are masked out.

  * This function may set various error flags.

-----

### Read Bits

```c
uint8_t bite_read(struct bite *self);
```

Reads up to 8 bits from the current range and advances the internal stream iterator. If fewer than 8 bits remain, only the least significant bits (LSB) are read, and the MSB are zeroed.

  * This function may set various error flags.

-----

## Debugging

To enable **debug output** for all read/write operations, compile your code with the `-DBITE_DEBUG` flag. This provides detailed tracing, including binary representations and intermediate calculations, invaluable for development and troubleshooting.

To enable **color coded output** use `-DBITE_COLOR` flag.

To enable **pedantic assertions** use `-DBITE_PEDANTIC` flag. **WARNING!** Pedantic assertions will treat all flags as critical!

To enable **Buffer overflow assertions** use `-DBITE_DEBUG_BUFFER_OVERFLOW` flag. **WARNING** This must only be enabled to debug bitE library itself!

-----

## Building

`bitE` is a **single-header library**. Simply include `bite.h` in your C source files:

```c
#include "bite.h"
```

To enable debugging during compilation:

```bash
gcc -DBITE_DEBUG ...
```

-----

## TODO

  * Ensure `bite_read` and `bite_write` are not used concurrently within the same context.
  * Optimize by reducing unnecessary variable calculations in read/write calls.
  * Enhance clarity of tests, use cases, and the README with more descriptive content.

-----

## Contributing

Bug reports, suggestions, and improvements are highly encouraged\! Feel free to open an issue or submit a pull request.

-----

## License

`bitE` is distributed under the **MIT License**.

-----

## Contact

For any questions or to contribute, please use the GitHub issues or pull requests.

-----

[![Stand With Ukraine](https://raw.githubusercontent.com/vshymanskyy/StandWithUkraine/main/banner-direct-single.svg)](https://stand-with-ukraine.pp.ua)
