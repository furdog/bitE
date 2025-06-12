# bitE
<p align="center"><img src="logo.jpg" /></p>

## Overview

The `bitE` is a tool for bit-wise read and write operations performed on
arbitary uint8_t buffers. The bit access is compatible with CAN DBC format.

---

## Features

- Critical safety (hardcore input and boundary checks)
- Constant time complexity for each operation
- 100% MISRA compilance (cppcheck)
- Written fully on C (std=c89)
- Linux kernel style
- Zero dependencies
- Single header library
- Automated tests
- Verbose and colored debug output for every operation
- Stream oriented (byte by byte read/write)
- Pedantic mode (asserts enabled)

---

## API

### Initialization

```c
void bite_init(struct bite *self, uint8_t *buf, size_t size);
```

Initializes the `bitE` context.

---

### Begin / End

```c
void bite_begin(struct bite *self, size_t ofs_bits, size_t len_bits,
		enum bite_order order);
```

Begin bit-wise operations on a specified range according to CAN DBC format.
The `order` specifies endianness and can have various values:
```c
BITE_ORDER_BIG_ENDIAN = 0, /**< Big endian */
BITE_ORDER_MOTOROLA   = 0, /**< Big endian */
BITE_ORDER_DBC_0      = 0, /**< Big endian */

BITE_ORDER_LIL_ENDIAN = 1, /**< Little endian */
BITE_ORDER_INTEL      = 1, /**< Little endian */
BITE_ORDER_DBC_1      = 1  /**< Little endian */
```

* may set various error flags

---

```c
void bite_end(struct bite *self);
```

Ends a bit-wise operation on a range.

* may set various error flags

---

### Rewind

```c
void bite_rewind(struct bite *self);
```

This will reset internal `bitE` stream iterator.
Any read/write operations will start from beggining.

---

### Write bits

```c
void bite_write(struct bite *self, uint8_t data);
```

Writes up to 8 bits INTO a range, iterates forward onto next 8 bits.
If there are less than 8 bits to write - it only writes LSB bits.
Exceed MSB bits are masked and not included into a stream in any way.

* may set various error flags

---

### Read bits

```c
uint8_t bite_read(struct bite *self);
```

reads up to 8 bits FROM range, iterates forward onto next 8 bits.
If there are less than 8 bits to read - it only reads LSB bits (MSB is zeroed).

* may set various error flags

---

### Debugging (enabled via `-DBITE_DEBUG`)

When compiled with `BITE_DEBUG`, debug logs show detailed tracing of all
read/write operations, including binary representation and intermediate
calculations. Logs are color-coded.

---

## Building

The library is a single-header file (`bite.h`). Include it directly:

```c
#include "bite.h"
```

To enable debugging:

```
gcc -DBITE_DEBUG ...
```

---

## TODO
- `bite_read` and `bite_write` should never be used in the same context
- Some variables unnecessarily calculated with every read/write call (OPTIMIZE)
- Tests and use cases should be more descriptive, as well as the README

---

## Contributing

Bug reports, suggestions, and improvements are welcome!

---

## License

MIT License.

---

## Contact

For questions or contributions, open an issue or pull request.

---

[![Stand With Ukraine](https://raw.githubusercontent.com/vshymanskyy/StandWithUkraine/main/banner-direct-single.svg)](https://stand-with-ukraine.pp.ua)
