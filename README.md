# SwiftRNG Software Kit

[SwiftRNG](https://tectrolabs.com/swiftrng/) devices are true random number generators that plug directly into any Windows, Linux, FreeBSD, or macOS computer via USB. SwiftRNG random numbers pass top industry statistical test suites such as Dieharder, NIST, Rngtest, BigCrush, EA IID or Ent.

Our latest SwiftRNG devices work with Linux, macOS, FreeBSD, Windows 10/11 (64-bit), and Windows Server 2016/2019 (64-bit), and run in virtual machines using software such as VMware Player, VMware ESXi, VirtualBox, and Parallels.

This repository contains the SwiftRNG Software Kit, which provides everything you need to use SwiftRNG with Linux, macOS, FreeBSD, and Windows.

* [SwiftRNG quick start guide](https://tectrolabs.com/docs/swiftrng/quick-start/)
* [SwiftRNG Documentation](https://tectrolabs.com/docs/swiftrng/)
* [More about SwiftRNG](https://tectrolabs.com/swiftrng/)

## Contents

* `linux` contains all necessary files and source code for building the `swrandom` kernel module/driver used with Linux distributions. The driver allows concurrent access to SwiftRNG entropy data streams from user space and also works with all SwiftRNG versions and models.
* `linux-and-macOS/swrng` contains all necessary files and source code for building the `bitcount`, `bitcount-cl`, `swrngseqgen`, `swrng`, `swrng-cl`, `swperftest`, `swperf-cl-test`, `sample`, `sample++`, `sample-cl`, `swdiag`, `swrawrandom`, and `swdiag-cl` utilities used with Linux, FreeBSD, and macOS systems, the optional OpenSSL engine and OpenSSL 4 provider, and the unit tests.
* `windows-x64` contains all necessary files and source code for building x64 versions of the `SwiftRNG.dll` component, `entropy-server.exe`, `entropy-cl-server.exe`, `bitcount.exe`, `bitcount-cl.exe`, `swrngseqgen.exe`, `swrng.exe`, `swrng-cl.exe`, `swdiag.exe`, `swdiag-cl.exe`, `swrawrandom.exe`, `sample.exe`, `sample-cl.exe`, `swperftest.exe`, `swperf-cl-test.exe`, `entropy-client-test.exe`, `dll-sample.exe` and `dll-test.exe` utilities for Windows 10/11 (64-bit) and Windows Server 2016/2019 (64-bit) using Visual Studio 2015/2017/2019.
* `windows-dll` contains all necessary files and source code for building the `SwiftRNG-64.dll` and `SwiftRNG-32.dll` libraries for Windows 10/11 and Windows Server 2016/2019 using Visual Studio 2015/2017/2019. Windows applications built with different programming languages can concurrently access the SwiftRNG entropy server through a unified API.
* `windows` (currently not supported) contains all necessary files and source code for building WIN32 versions of the `SwiftRNG.dll` component, `swrng.exe` and `swdiag.exe` utilities for older versions of Windows such as Windows 7 (32 bits) using Visual C++ 2010 Express. This version of the SwiftRNG software API is deprecated. New application development should use the `windows-x64` version of the software API.
* `windows-x86` (currently not supported) contains all necessary files and source code for building x86 versions of the `SwiftRNG.dll` component, `entropy-server.exe`, `bitcount.exe`, `swrngseqgen.exe`, `swrng.exe`, `swdiag.exe`, `swrawrandom.exe`, `sample.exe`, `dll-sample.exe` and `dll-test.exe` utilities for Windows 7+ (32 bit) using Visual Studio C++ 2010 Express or newer.

## License

The SwiftRNG Software Kit is provided under the terms in [LICENSE](LICENSE). It uses the libusb 1.0 library, which is distributed under the GNU Lesser General Public License.

## Authors

Andrian Belinski  
