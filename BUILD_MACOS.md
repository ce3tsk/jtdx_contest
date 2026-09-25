# Building JTDX\_contest on macOS

How to compile JTDX\_contest on a Mac, run it, and make the drag-and-drop installer
(`.dmg`), plus a record of the build fixes made on 25 September 2026 to get there
with a current toolchain (Homebrew GCC 16, CMake 4, macOS 26 on Apple Silicon).

- [Build environment](#build-environment)
- [1. Install the prerequisites](#1-install-the-prerequisites)
- [2. Configure](#2-configure)
- [3. Compile](#3-compile)
- [4. Run from the build directory](#4-run-from-the-build-directory)
- [5. Create the installer (DMG)](#5-create-the-installer-dmg)
- [6. Check the installer](#6-check-the-installer)
- [7. Distributing the DMG](#7-distributing-the-dmg)
- [Changes made](#changes-made)
- [Effect on Linux and Windows](#effect-on-linux-and-windows)
- [Troubleshooting](#troubleshooting)

---

## Build environment

What the reference build used. Other versions may work; these are known to work.

| Component | Version / location | From |
|---|---|---|
| Mac | Apple Silicon (arm64), macOS 26.7 | |
| C / C++ compiler | Apple clang 21 (`/usr/bin/cc`, `/usr/bin/c++`) | Xcode Command Line Tools |
| Fortran compiler | GNU Fortran 16.2.0 (`/opt/homebrew/bin/gfortran`) | Homebrew `gcc` |
| CMake | 4.4.3 | Homebrew `cmake` |
| Qt | 5.15.19 (`/opt/local/libexec/qt5`) | MacPorts `qt5` |
| FFTW (single precision) | 3.3.11 (`/opt/local/lib`) | MacPorts `fftw-3-single +gfortran` |
| Boost (headers) | 1.76 (`/opt/local/include`) | MacPorts `boost` |
| libusb | `/opt/local/lib` | MacPorts `libusb-devel` |
| Hamlib | 4.7.2 (`/usr/local/lib`) | built from source, `make install` |
| Documentation tools | asciidoctor 2.0.26, asciidoc, texinfo | MacPorts |

The source tree is `jtdx_contest/` and the build directory is `build/` next to it:

```
jtdx-prefix/
├── jtdx_contest/   ← this repository
└── build/          ← out-of-source build directory
```

Apple clang has no OpenMP. That is fine: all OpenMP code is Fortran and gfortran
provides it (see [Changes made](#changes-made)).

---

## 1. Install the prerequisites

Once per machine.

```bash
xcode-select --install
```

```bash
brew install gcc cmake
```

```bash
sudo port install qt5 fftw-3-single +gfortran boost libusb-devel pkgconfig asciidoctor asciidoc texinfo
```

JTDX\_contest needs the Qt modules Core, Gui, Widgets, Network, Multimedia,
SerialPort, WebSockets, PrintSupport and Svg; the `qt5` port installs all of them.

**Hamlib** is built from source and installed into `/usr/local`
(`./bootstrap && ./configure && make && sudo make install` in the Hamlib source).

**Shared memory.** JTDX\_contest uses a shared memory segment that is larger than the macOS
default. Install `Darwin/com.jtdx.sysctl.plist` once and reboot:

```bash
sudo cp jtdx_contest/Darwin/com.jtdx.sysctl.plist /Library/LaunchDaemons/
```

Check after the reboot with `sysctl kern.sysv.shmmax`. It must be at least 14680064.

---

## 2. Configure

Once, or whenever you start from an empty build directory:

```bash
mkdir -p build && cd build
```

```bash
cmake -DCMAKE_PREFIX_PATH=/opt/local/libexec/qt5 -DCMAKE_Fortran_COMPILER=/opt/homebrew/bin/gfortran ../jtdx_contest
```

The default build type is Release. Only a Release build bundles the libraries
and signs the app, so leave `CMAKE_BUILD_TYPE` alone for anything you want to
package.

> Type the quotes in shell commands as plain `"` characters. The reference build's
> `CMakeCache.txt` has `CMAKE_PREFIX_PATH=”/opt/local/libexec/qt5` with a typographic
> quote, pasted from a word processor. It still worked, because Qt was found through
> `Qt5_DIR`, but it is a trap on a fresh configure.

---

## 3. Compile

```bash
cd build && make -j$(sysctl -n hw.ncpu)
```

This produces:

| Output | What it is |
|---|---|
| `JTDX_contest.app` | the program (the `jtdx` target is renamed `JTDX_contest` on macOS so it cannot collide with a stock `jtdx.app`) |
| `jtdxjt9` | the decoder process the program starts |
| `wsprd_jtdx`, `udp_daemon_jtdx` | helper programs |
| `ft8sim`, `ft4sim`, `sfoxsim`, `sfrx`, … | test and simulation tools |

There is no `jtdx` executable in `build/`. The program is
`build/JTDX_contest.app/Contents/MacOS/JTDX_contest`.

---

## 4. Run from the build directory

`make` builds the app without its helper programs and data files; the install step
adds them. Started as it is, the app fails with:

```
Running: .../JTDX_contest.app/Contents/MacOS/jtdxjt9 ...
execve: No such file or directory
```

To run it straight from `build/`, copy them in, once after the first build and
again after every rebuild of `jtdxjt9` (any change under `lib/`):

```bash
cd build && A=JTDX_contest.app/Contents && cp jtdxjt9 wsprd_jtdx udp_daemon_jtdx $A/MacOS/ && mkdir -p $A/Resources/jtdx && cp ../jtdx_contest/{cty.dat,cty.dat_copyright.txt,contrib/Ephemeris/JPLEPH,contrib/CallDB/ALLCALL7.TXT,contrib/CallDB/CALL3.TXT} $A/Resources/jtdx/
```

```bash
open build/JTDX_contest.app
```

This build-tree app still loads Qt, gfortran and the other libraries from MacPorts and
Homebrew, so it only runs on this Mac. For anything you give to other people, use the
DMG.

---

## 5. Create the installer (DMG)

```bash
cd build && make package
```

Result: **`build/jtdx-3.0.0-rc08-Darwin-arm64.dmg`** (about 49 MB). The name comes from
the version in `Versions.cmake` and the build machine's architecture.

What `make package` does (CPack, `DragNDrop` generator):

1. Installs the project into a staging folder,
   `build/_CPack_Packages/Darwin-arm64/DragNDrop/…`. That puts the helper programs into
   `Contents/MacOS`, the data files into `Contents/Resources/jtdx`, and the Qt plugins,
   `qt.conf`, docs and man pages into the bundle.
2. Runs **`fixup_bundle`**. It copies every non-system library into the bundle (Qt
   frameworks into `Contents/Frameworks`, the other dylibs into `Contents/MacOS`) and
   rewrites the references so the app no longer depends on Homebrew, MacPorts or
   `/usr/local`.
3. **Re-signs the bundle ad hoc** (`Darwin/codesign`), inside-out: helper programs,
   dylibs, plugins and frameworks first, then the app. Step 2 breaks the original
   signatures, and Apple Silicon will not run unsigned code. The step ends with
   `codesign --verify --deep --strict` and stops the install if that fails.
4. Builds the DMG. It holds the app, a link to `/Applications`, `ReadMe.txt` (user
   installation notes) and `com.jtdx.sysctl.plist`, with the background image and window
   layout from `Darwin/jtdx_DMG.DS_Store`. The volume is named `JTDX_contest`.

`make install` runs the same steps 1–3 into `CMAKE_INSTALL_PREFIX` (default
`/usr/local`). You don't need it to make the DMG. If you run it with `sudo`, see
[Troubleshooting](#troubleshooting).

---

## 6. Check the installer

Worth doing before handing a DMG out:

```bash
cd build && M=$(hdiutil attach -readonly -nobrowse jtdx-*-Darwin-*.dmg | tail -1 | cut -f3-) && codesign --verify --deep --strict "$M/JTDX_contest.app" && echo "signature OK"
```

```bash
find "$M/JTDX_contest.app" -type f \( -perm +111 -o -name "*.dylib" \) -exec otool -L {} \; | grep -E "/opt/(homebrew|local)|/usr/local" || echo "self-contained"
```

```bash
hdiutil detach "$M"
```

The reference DMG passes both checks. Its decoder loads `libgomp`, `libgfortran`,
`libquadmath` and `libgcc_s` from inside the bundle.

---

## 7. Distributing the DMG

- **Apple Silicon only.** The build is arm64. Intel Macs would need a separate build
  on (or for) x86\_64.
- **Minimum macOS = the build machine's macOS.** The project sets a deployment
  target of 10.12, but the libraries copied from Homebrew and MacPorts were compiled
  for the macOS they were installed on. In the reference DMG, `libgfortran`, `libgomp`,
  FFTW, Hamlib and ICU require macOS 26.0 and Qt requires 14.0, so the DMG runs on
  **macOS 26 or later**. Check with:

  ```bash
  otool -l JTDX_contest.app/Contents/MacOS/libgfortran.5.dylib | grep minos
  ```

- **Gatekeeper warning.** The app is signed ad hoc, not with an Apple Developer ID,
  and not notarized. The first time, users must right-click the app, choose **Open**, and
  confirm (or run `xattr -dr com.apple.quarantine /Applications/JTDX_contest.app`).
  To remove the warning you need a paid Apple Developer account: sign with a
  Developer ID certificate and hardened runtime, then notarize with `xcrun notarytool`.
- **Microphone permission.** macOS asks once for audio input (the prompt text comes
  from `NSMicrophoneUsageDescription` in `Darwin/Info.plist.in`). An ad-hoc signed app
  gets a new identity with each build, so macOS may ask again after an update.
- Users must also install the **shared memory** plist, as described in `ReadMe.txt`
  inside the DMG.

---

## Changes made

Six problems stopped the Mac build, packaging, or running the result. All fixes are
marked `CE3TSK:` in the source.

### 1. `decoder.f90`: "Type mismatch in argument 'ncores' … passed REAL(4) to INTEGER(4)"

**Cause.** The Fortran libraries were compiled with `OpenMP_C_FLAGS`, the C
compiler's OpenMP flags. On a Mac those are empty (Apple clang has no OpenMP), so
the plain library `wsjt_fort` got no `-fopenmp`. Without it the `!$ use omp_lib` line
is skipped, `omp_get_num_procs()` is implicitly typed REAL, and passing it to
`decoder_threads(…, ncores)` fails. Linux and Windows were unaffected because GCC's C
flags are also `-fopenmp` there.

**Fix** (`CMakeLists.txt`). The Fortran code now gets the Fortran compiler's OpenMP
flags (`OpenMP_Fortran_FLAGS`) on every platform, applied to Fortran sources only,
through one helper `wsjt_fortran_openmp()` used by `wsjt_fort`, `wsjt_fort_omp` and
`jtdxjt9`. That replaces three copies of the same Apple workaround. Fallbacks:
`-fopenmp` on a Mac, else the C flags (for CMake older than 3.9, which reports no
Fortran flags). On a Mac, `wsjt_fort` and `wsjt_fort_omp` also link `gomp` by name.
A cached full path would point into a versioned Homebrew `Cellar/gcc/<version>`
folder, which a `brew upgrade gcc` deletes.

On a Mac the program itself links `wsjt_fort`, so its in-process Fortran now also
runs with OpenMP active, as it already did on Linux. The only Fortran routine on the
audio thread is `fil4_`, which has no large local arrays, so the smaller macOS
thread stack is not a concern.

### 2. `jplsubs.f`: "Missing actual argument 'pos' in call to 'split'"

**Cause.** GCC 16 implements the Fortran 2023 intrinsic `SPLIT(string, set, pos
[, back])`. The JPL ephemeris code has its own `SUBROUTINE SPLIT(TT, FR)`, and its
three calls now resolve to the intrinsic.

**Fix.** Renamed to `JPLSPLIT`: the definition and the three calls, all in
`lib/jplsubs.f` and used nowhere else. This works with every gfortran version.
Linux and Windows would hit the same error once they move to GCC 16.

### 3. Linking the tools: "Undefined symbols … `_GOMP_critical_name_start`"

**Cause.** Once `wsjt_fort` is compiled with OpenMP (fix 1), the programs linking it
(`ft8sim`, `sfrx`, …) need libgomp.

**Fix.** Part of fix 1: `target_link_libraries (wsjt_fort gomp)` on macOS.

### 4. Packaging: `fixup_bundle` looked for the wrong executable

**Cause.** On a Mac the `jtdx` target's output is renamed `JTDX_contest`, but the
install step still pointed `fixup_bundle` at `…/Contents/MacOS/jtdx`.

**Fix** (`CMakeLists.txt`). On macOS the executable name is read from the target's
`OUTPUT_NAME`. Other platforms keep the old line.

### 5. Packaging: "otool failed … can't open file: @rpath/libquadmath.0.dylib"

**Cause.** Homebrew's `libgfortran` loads `@rpath/libquadmath.0.dylib` and
`@rpath/libgcc_s.1.1.dylib` through its own `@loader_path` rpath. The project's
copy of CMake's `GetPrerequisites.cmake` resolves `@rpath` only through a hard-coded
MacPorts path (`/opt/local/lib/gcc-devel/`), the main executable's rpaths, and a
list of directories. It never expands `@loader_path`, and neither does current
CMake. It also resolves every dependency a second time with the main executable as
context, which cannot reach a library that only `libgfortran`'s rpath points to.

**Fix** (`CMake/Modules/GetPrerequisites.cmake`).

- `gp_resolve_item` also searches the loading binary's own `LC_RPATH`s, with
  `@loader_path` and `@executable_path` expanded.
- `get_prerequisites` lists items found only that way by their full path, so the
  second resolution succeeds.
- Items that resolved before keep their `@rpath/…` names, which `fixup_bundle` needs
  to rewrite references (for example with an official Qt build).

This change only touches libraries that used to stop packaging with an error.

### 6. The packaged app would not run: signatures broken

**Cause.** `fixup_bundle` rewrites install names with `install_name_tool`, which
invalidates the code signatures. Apple Silicon refuses to run unsigned code.

**Fix.** New directory `Darwin/codesign`:

- `CMakeLists.txt` finds `codesign` and installs the script.
- `sign_bundle.cmake.in` signs the installed bundle ad hoc, inside-out, without the
  deprecated `--deep`, and then verifies it.
  - If the bundle is not installed, or `codesign` is missing, it warns and skips.
  - If signing or verification fails, it stops the install.

The top-level `CMakeLists.txt` adds this directory **as the very last thing in the
file**. Install rules run either top level first and then subdirectories in order
(policy CMP0082 OLD, today's behaviour with the 3.7.2 minimum), or all in declaration
order (CMP0082 NEW). Last is after `fixup_bundle` and after the `manpages`/`doc`
installs into the bundle either way. In an earlier version the man pages were
installed after signing and broke the seal.

### 7. `Darwin/ReadMe.txt` (the user notes in the DMG)

Updated for this fork:

- The volume path is `/Volumes/JTDX_contest`, not `/Volumes/JTDX`.
- The app, its menu and the microphone prompt are called JTDX\_contest.
- A stock JTDX no longer needs renaming, because the bundle names differ.
- Added the Apple Silicon / macOS 26 requirement and the first-launch Gatekeeper
  step.
- Fixed the plist name `com.wsjtx.sysctl.plist` to `com.jtdx.sysctl.plist`.

`Darwin/developer read me.txt` got the same volume path fix, and a DMG file pattern
(`jtdx-*-Darwin-*.dmg`) that matches the real file name.

### Files changed

| File | Change |
|---|---|
| `CMakeLists.txt` | Fortran OpenMP flags helper, `gomp` link on macOS, `fixup_exe` from `OUTPUT_NAME`, `Darwin/codesign` added last |
| `lib/jplsubs.f` | `SPLIT` → `JPLSPLIT` |
| `CMake/Modules/GetPrerequisites.cmake` | `@rpath` resolution through the loader's own rpaths |
| `Darwin/codesign/CMakeLists.txt` | new: finds `codesign`, installs the signing script |
| `Darwin/codesign/sign_bundle.cmake.in` | new: ad-hoc inside-out signing and verification |
| `Darwin/ReadMe.txt` | user notes brought up to date |
| `Darwin/developer read me.txt` | volume path and DMG file name pattern |
| `BUILD_MACOS.md` | this document |

---

## Effect on Linux and Windows

| Change | Linux / Windows |
|---|---|
| OpenMP flags for the Fortran libraries | **Applies.** With GCC the Fortran and C OpenMP flags are both `-fopenmp` and all of `wsjt_FSRCS` is Fortran, so the compile should be identical. **Confirm with a normal build on each.** |
| `SPLIT` → `JPLSPLIT` | Applies. Internal name only, and needed there too with GCC 16. |
| `GetPrerequisites.cmake` | Windows packaging uses it, but the new code only acts on `@rpath/…` names, which only macOS has. Linux doesn't use it. |
| `fixup_exe` | The non-Apple branch is the old line, unchanged. |
| `gomp` link, `Darwin/codesign`, ReadMe files | macOS only. |

---

## Troubleshooting

| Symptom | Cause and fix |
|---|---|
| `execve: No such file or directory` for `jtdxjt9` when starting the app from `build/` | The helpers aren't in the build-tree bundle. See [step 4](#4-run-from-the-build-directory), or use the DMG. |
| `make package` fails: `file failed to open for writing (Permission denied): …/build/install_manifest.txt` | An earlier `sudo make install` left the file owned by root. Run `sudo chown $USER build/install_manifest.txt`. The same install also left a root-owned `/usr/local/JTDX_contest.app`; remove it with `sudo rm -rf /usr/local/JTDX_contest.app` if unwanted. |
| After `brew upgrade gcc`: link errors or "directory not found" for `…/Cellar/gcc/<old version>/…` | CMake caches the Fortran compiler's library directories. Reconfigure in an empty build directory ([step 2](#2-configure)). |
| `Unable to create shared memory segment` | The shared memory plist isn't installed. See [step 1](#1-install-the-prerequisites). |
| `codesign not found: … is NOT re-signed` during install | Install the Xcode Command Line Tools. Without a signature the app won't start on Apple Silicon. |
| The app from the DMG won't open on another Mac ("damaged" / "cannot be opened") | Gatekeeper, see [step 7](#7-distributing-the-dmg). Also check the Mac is Apple Silicon with a new enough macOS. |
