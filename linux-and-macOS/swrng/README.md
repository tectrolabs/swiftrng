# SwiftRNG utilities for Linux, FreeBSD and macOS

Source code for the command line utilities and the C/C++ API that access a
[SwiftRNG](https://tectrolabs.com/swiftrng/) true random number generator over USB.

## Contents

| Program | Purpose |
| --- | --- |
| `swrng` | Download random bytes from a device to a file, standard output or a named pipe |
| `swrng-cl` | The same for a cluster of one or more devices |
| `swdiag`, `swdiag-cl` | Device diagnostics, for one device and for a cluster |
| `swperftest`, `swperf-cl-test` | Performance tests, for one device and for a cluster |
| `swrawrandom` | Download raw bytes from one of the noise sources, with no processing or tests |
| `swrngseqgen` | Generate a random sequence of unique integers within a range |
| `bitcount`, `bitcount-cl` | Count 1 and 0 bits in data from a device or from a file |
| `sample`, `sample++`, `sample-cl` | Examples of using the C and C++ API |
| `eng_swiftrng` | OpenSSL engine that uses SwiftRNG as the random source (optional, see below) |
| `run-swrng-pserver.sh` | Named pipe server script, see [below](#named-pipe-server) |
| `80-swiftrng-usb-access.rules` | udev rules for non-root USB access on Linux |

The API is in `api-inc/` (headers) and `api-src/` (sources). `SwiftRngApi.h` is the C++ interface,
`swrng.h` and `swrng-cl.h` are the C interfaces.

`swrng` and `swrng-cl` list their options when run without arguments; they have no `-h` option.
`swrawrandom`, `swrngseqgen` and `bitcount` print their usage when run without valid arguments.
`swdiag` and `swperftest` take no arguments.

## Requirements

* A C++11 compiler (GCC or Clang)
* libusb 1.0 development files
  * Debian/Ubuntu/Raspberry Pi OS: `sudo apt install build-essential libusb-1.0-0-dev`
  * macOS: `brew install libusb`
  * FreeBSD: libusb is part of the base system
* CMake 3.10 or newer, if building with CMake (`sudo apt install cmake`, `brew install cmake`, `pkg install cmake`)
* OpenSSL development files, only for the optional OpenSSL engine

## Building with CMake

From this directory:

```sh
cmake -S . -B build
cmake --build build -j4
```

The programs are created in `build/`, for example `./build/swrng`.

Optionally install them to `/usr/local/bin` (the `sample` programs and the engine are not installed):

```sh
sudo cmake --install build
```

Use `--prefix` to install elsewhere, for example `cmake --install build --prefix ~/.local`.
To remove what was installed (run this before deleting `build/`; use the same `DESTDIR` if you installed with one):

```sh
sudo cmake --build build --target uninstall
```

Notes:

* **macOS:** Homebrew's libusb in `/opt/homebrew/opt/libusb` or `/usr/local/opt/libusb` is found automatically.
* **OpenSSL engine:** add `-DSWRNG_BUILD_ENGINE=ON` to also build `eng_swiftrng.so`. It needs OpenSSL 3 or older:
  OpenSSL 4 no longer provides the `ENGINE` functions it uses, so with OpenSSL 4 pass
  `-DOPENSSL_ROOT_DIR=<path to an OpenSSL 3 install>` or leave the engine off.
* To start over, delete the `build/` directory.

## Building with make

```sh
make              # Linux and macOS
sudo make install
```

On FreeBSD use `Makefile.bsd`:

```sh
make -f Makefile.bsd
sudo make -f Makefile.bsd install
```

`make clean` removes the build output. The OpenSSL engine is built separately with `make eng_swiftrng`.

## Running the tests

The unit tests need no device and never open one, so they are safe to run with a SwiftRNG plugged in.
They are built only when you ask for them:

```sh
cmake -S . -B build -DSWRNG_BUILD_TESTS=ON
cmake --build build -j4
cd build && ctest --output-on-failure
```

Each test file in `tests/` is its own program (for example `build/tests/test_sha`), which you can also run
directly to see every test by name. They use a small framework in `tests/test_framework.h`, so there is nothing
extra to install.

| Suite | What it checks |
| --- | --- |
| `sha` | The SHA-256 and SHA-512 code used to post-process device data, against known answers |
| `xorshift` | The xorshift64 post-processing, against an independent implementation |
| `health` | The Repetition Count and Adaptive Proportion health tests, with good and with stuck data |
| `usb_serial` | The serial port class, using a pseudo terminal in place of the device |
| `api_no_device` | The C++ and C APIs refuse to run, and say why, when no device is open |
| `cl_api` | The cluster API's context handling and error reporting |

`SwiftRngApi` keeps its post-processing and health tests in private methods, so `tests/private_access.h` makes
them reachable for the tests without changing the library. The parts that read from a real device
(`SwiftRngApi::open()`, `RandomSeqGenerator` and opening a cluster) are not covered, because they need hardware.

## Linux USB access

To use the device without root, install the udev rules and re-plug the device:

```sh
sudo cp 80-swiftrng-usb-access.rules /etc/udev/rules.d/
sudo udevadm control --reload-rules && sudo udevadm trigger
```

## FreeBSD notes

**Speed.** Download speed is lower on FreeBSD than the device can deliver. In `swperftest`, a SwiftRNG Pro V2.3
on FreeBSD 14.3 reached about 58 Mbit/s with post-processing disabled and about 38 Mbit/s with SHA256. When the
same device was read directly with libusb in a test, bypassing the serial driver, it reached about 210 Mbit/s and
129 Mbit/s. The limit is in FreeBSD's `umodem` serial driver, which reads the device through a single 1 KB USB
transfer at a time, not in these utilities or in the device. The USB link itself runs at full high speed.

**Access.** Devices are found with `usbconfig`, which needs root. Run the programs with `sudo`, or give your user
access to the USB device nodes in `/dev/usb`. The serial port `/dev/cuaU*` is owned by the `dialer` group.

## Usage examples

List the available devices:

```sh
swrng -ld
```

Download 1 MB of random bytes to `rnd.bin`:

```sh
swrng -dd -fn rnd.bin -nb 1000000
```

Download 10 blocks (160,000 bytes) of raw bytes from the first noise source of the first device:

```sh
swrawrandom 10 0 0 ns0.bin
```

Generate a sequence of unique random numbers between 1 and 50 and show 6 of them:

```sh
swrngseqgen 0 1 50 6
```

Count the 1 and 0 bits in a file, and in data from the first device:

```sh
bitcount -fn rnd.bin
bitcount 10 0
```

Run the diagnostics and performance tests:

```sh
swdiag
swperftest
```

## swrng options

`swrng` needs one of two modes:

| Mode | Meaning |
| --- | --- |
| `-ld`, `--list-devices` | List the available devices that are not in use |
| `-dd`, `--download-data` | Download random bytes and store them in a file |

and accepts these options (each has a long form shown by `swrng` without arguments):

| Option | Meaning |
| --- | --- |
| `-fn FILE` | File for the random bytes. Use `STDOUT` to write them to standard output |
| `-nb NUMBER` | Number of bytes to download, up to 200000000000. Without it, the download is continuous |
| `-dn NUMBER` | Device number when more than one is connected. Use `-ld` to list them |
| `-ppn NUMBER` | Power profile from 0 (lowest power, slowest) to 9 (highest, the default) |
| `-ppm METHOD` | Post-processing method: `SHA256`, `SHA512` or `xorshift64` |
| `-dpp` | Disable post-processing, for devices with version 1.2 or newer |
| `-dst` | Disable the Repetition Count and Adaptive Proportion statistical tests |

For example, to download 12 MB using the lowest power profile:

```sh
swrng -dd -fn rnd.bin -nb 12000000 -ppn 0
```

On Linux and macOS, running `swrng` may need administrator permissions. `swrng-cl` takes similar options
for a cluster of devices; run it without arguments to see them.

## OpenSSL provider (OpenSSL 4.0 and later)

OpenSSL 4.0 removed engines, so `eng_swiftrng.so` does not work with it. The SwiftRNG provider takes its place: when
OpenSSL is told to use it as the random provider, `RAND_bytes()` and `RAND_priv_bytes()` return bytes from the SwiftRNG
device. The provider is built only when you ask for it. It needs OpenSSL 4.0, or a 3.5.x release that has
`OSSL_FUNC_PROVIDER_RANDOM_BYTES`, and configuring fails with a message if the OpenSSL found does not have it. For
OpenSSL 3.4 and earlier, use the engine.

```sh
cmake -S . -B build -DSWRNG_BUILD_PROVIDER=ON -DOPENSSL_ROOT_DIR=<path to OpenSSL 4>
cmake --build build
```

This creates `build/swiftrng.so` (`build/swiftrng.dylib` on macOS). Name it in the OpenSSL config file (`openssl.cnf`) and set `OPENSSL_CONF` to that file:

```
openssl_conf = openssl_init

[openssl_init]
random = random_sect
providers = provider_sect

[provider_sect]
default = default_sect
swiftrng = swiftrng_sect

[default_sect]
activate = 1

[swiftrng_sect]
module = /path/to/swiftrng.so
activate = 1

[random_sect]
random_provider = swiftrng
```

To check it, run `openssl list -providers` (it should list `SwiftRNG`) and `openssl rand -hex 16`.

Things to know:

* **Keep `random` before `providers`** in `[openssl_init]`. With OpenSSL 4.0.3, listing `providers` first makes the provider
  get unloaded again as soon as `random_provider` is read, and OpenSSL then uses its own random source.
* **Load the config before the first random call.** OpenSSL answers the very first `RAND_bytes()` call of a process from
  its own source if it has not read the config yet. Tools such as `openssl` read it first. In your own program, call
  `OPENSSL_init_crypto(OPENSSL_INIT_LOAD_CONFIG, NULL)` before generating random numbers.
* **A missing device is not an error for OpenSSL.** If the provider cannot start, for example because no SwiftRNG is found,
  it is not loaded and OpenSSL silently keeps using its own source. Applications that must use the device should check
  `OSSL_PROVIDER_available(NULL, "swiftrng")`. If the provider loads and the device then fails, `RAND_bytes()` returns 0.
* The provider uses the first SwiftRNG device and holds it while the process runs, so one process at a time can use it.

The provider tests (`-DSWRNG_BUILD_TESTS=ON` together with the provider option) use a stand-in provider with a known
byte stream instead of a device, so they need no hardware. The provider was run end to end with OpenSSL 4.0.3 and a
SwiftRNG Pro V2.3 on a Raspberry Pi 5.

## Named pipe server

`run-swrng-pserver.sh` keeps `/tmp/swiftrng` filled with random bytes from a cluster of one or more devices
using `swrng-cl`, so other programs can read from it like a file. It expects the programs in `/usr/local/bin`.
To start it at boot, add this line with `crontab -e`:

```
@reboot /usr/local/bin/run-swrng-pserver.sh >> /var/log/run-swrng-pserver.log 2>&1
```

## More information

* [SwiftRNG quick start guide](https://tectrolabs.com/docs/swiftrng/quick-start/)
* [SwiftRNG documentation](https://tectrolabs.com/docs/swiftrng/)
* [Top level README](../../README.md)
