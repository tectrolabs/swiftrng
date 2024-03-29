# SwiftRNG Software Kit

[SwiftRNG](https://tectrolabs.com/swiftrng/) devices are true random number generators that plug directly into any Windows, Linux, freeBSD or macOS computer via USB. SwiftRNG random numbers pass top industry statistical test suites such as Dieharder, NIST, Rngtest, BigCrush, EA IID or Ent.

Our latest SwiftRNG devices work with Linux, macOS, freeBSD, Windows 10/11 (64-bit), and Windows Server 2016/2019 (64-bit), and run in virtual machines using software such as VMware Player, VMware ESXi, VirtualBox, and Parallels.

This repository contains the SwiftRNG Software Kit, which provides everything you need to use SwiftRNG with Linux, macOS, freeBSD and Windows.

* [SwiftRNG quick start guide](https://tectrolabs.com/docs/swiftrng/quick-start/)
* [SwiftRNG Documentation](https://tectrolabs.com/docs/swiftrng/)
* [More about SwiftRNG](https://tectrolabs.com/swiftrng/)

## Contents

* `linux` contains all necessary files and source code for building the `swrandom` kernel module/driver used with Linux distributions. The driver allows concurrent access to SwiftRNG entropy data streams from user space and also works with all SwiftRNG versions and models.
* `linux-and-macOS` contains all necessary files and source code for building `bitcount`, `swrngseqgen`, `swrng`, `swrng-cl`, `swperftest`, `swperf-cl-test`, `sample`, `sample-cl`, `swdiag`, `swrawrandom`, and `swdiag-cl` utilities used with Linux, freeBSD and macOS distributions.
* `windows-x64` contains all necessary files and source code for building x64 versions of the `SwiftRNG.dll` component, `entropy-server.exe`, `entropy-cl-server.exe`, `bitcount.exe`, `bitcount-cl.exe`, `swrngseqgen.exe`, `swrng.exe`,`swrng-cl.exe`, `swdiag.exe`, `swdiag-cl.exe`, `swrawrandom.exe`, `sample.exe`, `sample-cl.exe`, `swperf-test.exe`, `swperf-cl-test.exe`, `dll-sample.exe` and `dll-test.exe` utilities for Windows 10/11 (64 bit), and Windows Server 2016/2019 (64 bit) using Visual Studio 2015/2017/2019.
* `windows` (currently not supported) contains all necessary files and source code for building WIN32 versions of the `SwiftRNG.dll` component, `swrng.exe` and `swdiag.exe` utilities for older versions of Windows such as Windows 7 (32 bits) using Visual C++ 2010 Express. This version of the SwiftRNG software API is deprecated. New application development should use the `windows-x64` version of the software API.
* `windows-x86` (currently not supported) contains all necessary files and source code for building x86 versions of the `SwiftRNG.dll` component, `entropy-server.exe`, `bitcount.exe`, `swrngseqgen.exe`, `swrng.exe`, `swdiag.exe`, `swrawrandom.exe`, `sample.exe`, `dll-sample.exe` and `dll-test.exe` utilities for Windows 7+ (32 bit) using Visual Studio C++ 2010 Express or newer.

## Authors

Andrian Belinski  
