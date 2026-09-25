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
- [8. Intel Macs: building on macOS 10.15 Catalina](#8-intel-macs-building-on-macos-1015-catalina)
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
| Hamlib | 4.7.2 (`/usr/local/lib`), built for macOS 11.0 | git tag `4.7.2`, see [step 1.2](#12-hamlib-472) |
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

### 1.1 Tools and libraries

```bash
xcode-select --install
```

```bash
brew install gcc cmake
```

```bash
sudo port install qt5 fftw-3-single +gfortran boost libusb-devel pkgconfig asciidoctor asciidoc texinfo autoconf automake libtool
```

JTDX\_contest needs the Qt modules Core, Gui, Widgets, Network, Multimedia,
SerialPort, WebSockets, PrintSupport and Svg; the `qt5` port installs all of them.
`autoconf`, `automake` and `libtool` are only needed to build Hamlib (next step).

### 1.2 Hamlib 4.7.2

JTDX\_contest uses **Hamlib 4.7.2**, the latest Hamlib 4 release, built from git and
installed into `/usr/local`. On Apple Silicon it is built for **macOS 11.0**, the oldest
macOS these Macs run. Background, and how to check for a newer 4.x release, are in
[Hamlib 4.7.2: versions and options](#hamlib-472-versions-and-options).

**Remove a previous installation first**, if there is one in `/usr/local`, so no old
files are left behind. This has to happen before an existing clone is switched to
another tag. On the reference Mac that's the first 4.7.2 build: its options
were ignored (see the note in the reference section), so it installed static and C++
libraries too and declares macOS 26 as its minimum. Run this in the source tree that
was last installed, before configuring it again:

```bash
cd ~/dev/Hamlib && sudo make uninstall && make distclean
```

**Get the source.** For a new copy:

```bash
cd ~/dev && git clone --branch 4.7.2 --depth 1 https://github.com/Hamlib/Hamlib.git
```

For an existing clone (like `~/dev/Hamlib` on the reference Mac), switch it to the
release:

```bash
cd ~/dev/Hamlib && git fetch --tags origin && git checkout 4.7.2
```

**Build and install:**

```bash
cd ~/dev/Hamlib && ./bootstrap && ./configure --prefix=/usr/local --enable-shared --disable-static --disable-winradio --without-readline --without-indi --without-cxx-binding CFLAGS="-g -O2 -mmacosx-version-min=11.0 -I/opt/local/include" LDFLAGS="-mmacosx-version-min=11.0" LIBUSB_LIBS="-L/opt/local/lib -lusb-1.0"
```

```bash
make -j$(sysctl -n hw.ncpu) && sudo make install
```

**Check it:**

```bash
rigctl --version; otool -l /usr/local/lib/libhamlib.4.dylib | grep minos; ls /usr/local/lib | grep -i hamlib
```

The output should show:

- `rigctl Hamlib 4.7.2`
- `minos 11.0`
- only `libhamlib.4.dylib`, `libhamlib.dylib` and `libhamlib.la`: no `.a` files and no
  `libhamlib++`

**Then rebuild JTDX\_contest** ([step 3](#3-compile) and
[step 5](#5-create-the-installer-dmg)). `make` relinks against the new library, and
`make package` bundles it together with the `rigctl`, `rigctld` and `rigctlcom` tools
from `/usr/local/bin`.

### 1.3 Shared memory

JTDX\_contest needs one shared memory segment of about 13 MB per running instance, and
macOS allows 4 MB. The launch daemon `Darwin/com.jtdx.sysctl.plist` raises the limits at
every boot: `kern.sysv.shmmax` to 100 MB and `kern.sysv.shmall` to 256 MB, about 19
instances (counted in pages of the Mac's page size, so 65536 pages on Intel and 16384 on
Apple Silicon). It only ever raises them, and never lowers values another program has
set higher.

**JTDX\_contest installs it itself.** When the limits are too small, the app offers to fix
them. After a Yes and the administrator password, it installs the daemon, raises the
limits at once, and carries on starting. By hand, this does the same:

```bash
sudo cp jtdx_contest/Darwin/com.jtdx.sysctl.plist /Library/LaunchDaemons/
```

After that, reboot, or apply it at once with the command from the plist:

```bash
sudo sh -c "$(/usr/libexec/PlistBuddy -c 'Print :ProgramArguments:2' /Library/LaunchDaemons/com.jtdx.sysctl.plist)"
```

Check with `sysctl kern.sysv.shmmax kern.sysv.shmall`.

### Hamlib 4.7.2: versions and options

Reference for [1.2](#12-hamlib-472) (Apple Silicon) and
[8.3](#83-build-hamlib-472) (Intel).

**Version.** Hamlib 4.7.2 is git tag `4.7.2` (commit `40f63488f`, 21 June 2026) in
https://github.com/Hamlib/Hamlib.git, the latest 4.x release at the time of writing.
Both DMGs use it:

| Mac | Built for (`-mmacosx-version-min`) |
|---|---|
| Apple Silicon | `11.0` |
| Intel, built on macOS 10.15 Catalina | `10.15` |

To see whether a newer 4.x release exists, list the tags (the last line is the newest):

```bash
git ls-remote --tags https://github.com/Hamlib/Hamlib.git | grep -oE 'refs/tags/4\.[0-9.]+$' | sed 's|refs/tags/||' | sort -V | tail -3
```

To move to a newer release, use its tag in place of `4.7.2` in the `git clone` or
`git checkout` command.

**The `configure` options:**

- `--enable-shared --disable-static`: only the shared library, which the installer
  bundles.
- `--disable-winradio`: that backend is Windows-only.
- `--without-readline`: `rigctl` without line editing, one less library to bundle.
- `--without-indi`: no INDI astronomy support.
- `--without-cxx-binding`: JTDX\_contest uses the C API.
- `-mmacosx-version-min` in `CFLAGS` and `LDFLAGS`: the oldest macOS the library runs
  on (the table above).
- `LIBUSB_LIBS`, `-I/opt/local/include`: use MacPorts' libusb (for USB rigs such as
  FUNcube or SI570 dongles).

`bootstrap` needs MacPorts' `autoconf`, `automake` and `libtool`; on macOS it uses
`glibtoolize` by itself.

> Type these commands (or copy them from this file) with plain ASCII `-` and `"`. A
> command pasted from a word processor or web page can contain typographic dashes and
> quotes (`–enable-shared`, `”-g -O2″`). `configure` doesn't recognize them, and the
> build silently falls back to the defaults. That happened to the first Apple Silicon
> build: its `doit` script used such characters, so Hamlib was built with default
> options, including static and C++ libraries, and declares macOS 26 as its minimum.
> `config.status` shows what `configure` really got: look for `ac_cs_config=`.

**Replacing an installed Hamlib.** Run `sudo make uninstall` in the source tree that was
installed last, before you reconfigure or switch it to another tag. Then build and
install the new one, and rebuild JTDX\_contest so the installer picks it up.

---

## 2. Configure

Once, or whenever you start from an empty build directory:

```bash
mkdir -p build && cd build
```

```bash
cmake -DCMAKE_PREFIX_PATH=/opt/local/libexec/qt5 -DCMAKE_Fortran_COMPILER=/opt/homebrew/bin/gfortran ../jtdx_contest
```

The default build type is Release. Only a Release build bundles the libraries, so
leave `CMAKE_BUILD_TYPE` alone for anything you want to give to other people. A Debug
build's DMG still gets its ReadMe filled in and is signed, but it only runs on a Mac
that has the same MacPorts and Homebrew libraries installed. Its ReadMe says so, and
names no macOS range.

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
3. **Records the minimum macOS** (`Darwin/finish_bundle/min_macos.cmake`).
   - It takes the highest minimum of any binary in the bundle, usually one of the copied
     libraries, separately for each architecture the app was built for (read from the
     app with `lipo -info`).
   - That value goes into `Info.plist` as `LSMinimumSystemVersion`, so on an older macOS
     the system itself says the app needs a newer version. A universal app also gets
     `LSMinimumSystemVersionByArchitecture`, so Intel and Apple Silicon each have their
     own minimum.
   - It is also filled into the DMG's `ReadMe.txt`, together with the kind of Mac:
     Apple Silicon, Intel, or both.
   - If `otool` can't read the app's own executable, packaging stops, so a DMG can't
     silently ship without its minimum. It also stops if a bundled library lacks one of
     the app's architectures, because the app would fail to load it on those Macs. A
     file that isn't a binary at all only gives a warning.
4. **Re-signs the bundle ad hoc** (`Darwin/finish_bundle/sign_bundle.cmake`),
   inside-out: helper programs, dylibs, plugins and frameworks first, then the app.
   Step 2 breaks the original signatures, and Apple Silicon will not run unsigned code.
   The step ends with `codesign --verify --deep --strict` and stops the install if that
   fails.
5. Builds the DMG. It holds the app, a link to `/Applications`, `ReadMe.txt` (user
   installation notes) and `com.jtdx.sysctl.plist`, with the background image and window
   layout from `Darwin/jtdx_DMG.DS_Store`. The volume is named `JTDX_contest`.

`make install` runs the same steps 1–4 into `CMAKE_INSTALL_PREFIX` (default
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

- **Apple Silicon only.** The build is arm64. For Intel Macs, make a second DMG on an
  Intel Mac, see [section 8](#8-intel-macs-building-on-macos-1015-catalina).
- **Minimum macOS = the build machine's macOS.** The project sets a deployment
  target of 10.12, but the libraries copied from Homebrew and MacPorts were compiled
  for the macOS they were installed on. In the reference DMG, `libgfortran`, `libgomp`,
  FFTW and ICU require macOS 26.0 and Qt requires 14.0, so the DMG runs on
  **macOS 26 or later**. Packaging works this out itself ([step 5](#5-create-the-installer-dmg),
  point 3). It writes the value into the app's `Info.plist` and the DMG's `ReadMe.txt`,
  and prints `Minimum macOS for …` during `make package`. To see it in a built app:

  ```bash
  /usr/libexec/PlistBuddy -c "Print :LSMinimumSystemVersion" JTDX_contest.app/Contents/Info.plist
  ```

- **Gatekeeper warning.** The app is signed ad hoc, not with an Apple Developer ID,
  and not notarized, so macOS refuses it the first time. How users allow it depends
  on their macOS:
  - **macOS 15 Sequoia and later** (so every user of the Apple Silicon DMG): open the
    app once and close the warning. Then go to **System Settings → Privacy & Security**,
    click **Open Anyway** next to the JTDX\_contest message, and confirm. Right-click
    → Open no longer works around Gatekeeper since macOS 15.
  - **macOS 14 and earlier**: right-click the app, choose **Open**, and confirm.
  - Either way, this also works:
    `xattr -dr com.apple.quarantine /Applications/JTDX_contest.app`.

  `Darwin/ReadMe.txt` in the DMG explains both ways. To remove the warning you need a
  paid Apple Developer account: sign with a Developer ID certificate and hardened
  runtime, then notarize with `xcrun notarytool`.
- **Microphone permission.** macOS asks once for audio input (the prompt text comes
  from `NSMicrophoneUsageDescription` in `Darwin/Info.plist.in`). An ad-hoc signed app
  gets a new identity with each build, so macOS may ask again after an update.
- Users must also install the **shared memory** plist, as described in `ReadMe.txt`
  inside the DMG.

## 8. Intel Macs: building on macOS 10.15 Catalina

The Apple Silicon DMG does not run on Intel Macs. The Intel DMG is built on an Intel Mac,
and **the macOS it is built on becomes its minimum**. The libraries it bundles come from
MacPorts packages made for that macOS. So build on the oldest macOS you want to support.

Reference machine: **MacBook Air 13-inch, Mid 2012** (model A1466, EMC 2559, model
identifier `MacBookAir5,2`), whose newest macOS is **10.15.7 Catalina**. A DMG built on it
runs on **Intel Macs with macOS 10.15 or later**, which covers practically every Intel Mac
from 2012 onwards. It probably also runs on Apple Silicon through Rosetta 2, but those
users should take the Apple Silicon DMG.

This machine differs from the Apple Silicon build in four ways:

- **MacPorts for everything, gfortran included.** Homebrew no longer supports Catalina.
- **Only the Qt modules JTDX\_contest needs**, to save space.
- **No manual or man pages.** That saves Ruby, Python and DocBook; the manual is online.
- **The deployment target set to 10.15**, so the app's own programs declare the same
  minimum as the libraries. Packaging then records 10.15 in `Info.plist` and the
  ReadMe (unless a library needs more).

Space needed on the macOS partition: about **5 GB** (Command Line Tools 1–1.5 GB,
MacPorts about 2 GB, source and build folder about 0.6 GB, plus room to build). The Air
has a dual-core CPU, so expect the JTDX\_contest build to take roughly 30–60 minutes.

### 8.1 Prepare the Mac

1. Update to **macOS 10.15.7** (Apple menu → System Preferences → Software Update). If
   it isn't offered, fetch the installer and run it from Applications:
   ```bash
   softwareupdate --fetch-full-installer --full-installer-version 10.15.7
   ```
2. Install the Command Line Tools (C/C++ compiler, git, `codesign`):
   ```bash
   xcode-select --install
   ```
3. Install **MacPorts** with the macOS 10.15 Catalina package from
   https://www.macports.org/install.php, then update it:
   ```bash
   sudo port selfupdate
   ```
4. Install the shared memory setting (once) and reboot, as in
   [step 1](#1-install-the-prerequisites).

### 8.2 Install the packages

```bash
sudo port install gcc14 cmake pkgconfig qt5-qtbase qt5-qtmultimedia qt5-qtserialport qt5-qtwebsockets qt5-qtsvg qt5-qttools fftw-3-single +gfortran boost libusb autoconf automake libtool
```

- `gcc14` provides gfortran as `/opt/local/bin/gfortran-mp-14`. If `port install`
  reports that `gcc14` isn't available, use the newest `gccNN` it offers
  (`port search --name --glob 'gcc1*'`) and change the `-mp-14` below to match.
- `qt5-qttools` provides Qt's translation tools (LinguistTools), which the build needs.
- Hamlib is not taken from MacPorts. It's built from git so both DMGs carry the same
  Hamlib, see the next step.

Free the space MacPorts used for downloads and build leftovers:

```bash
sudo port clean --all installed && sudo port reclaim
```

### 8.3 Build Hamlib 4.7.2

The same Hamlib release as on Apple Silicon, built for macOS 10.15. Background and
option details are in [Hamlib 4.7.2: versions and options](#hamlib-472-versions-and-options).

```bash
mkdir -p ~/dev && cd ~/dev && git clone --branch 4.7.2 --depth 1 https://github.com/Hamlib/Hamlib.git
```

```bash
cd ~/dev/Hamlib && ./bootstrap && ./configure --prefix=/usr/local --enable-shared --disable-static --disable-winradio --without-readline --without-indi --without-cxx-binding CFLAGS="-g -O2 -mmacosx-version-min=10.15 -I/opt/local/include" LDFLAGS="-mmacosx-version-min=10.15" LIBUSB_LIBS="-L/opt/local/lib -lusb-1.0"
```

```bash
make -j$(sysctl -n hw.ncpu) && sudo make install
```

Check it: `rigctl --version` should report `Hamlib 4.7.2`, and
`otool -l /usr/local/lib/libhamlib.4.dylib | grep minos` should show `minos 10.15`.

`autoconf`, `automake` and `libtool` are only needed for this step. Remove them afterwards
if space is tight: `sudo port uninstall autoconf automake libtool`.

### 8.4 Get the source (and a patch kit, if there is one)

The layout matches the Apple Silicon Mac: `~/dev/jtdx-prefix/` holds `jtdx_contest` and
`build`.

```bash
mkdir -p ~/dev/jtdx-prefix && cd ~/dev/jtdx-prefix && git clone https://github.com/ce3tsk/jtdx_contest.git
```

**With a patch kit** (such as `jtdx_contest-intel-macos.zip`): the kit holds changes
that are not in the repository yet. Check out the commit its patches were made against,
named on the `Base:` line at the top of each patch, and apply them. This reads the commit
from the kit:

```bash
cd ~/dev/jtdx-prefix && unzip jtdx_contest-intel-macos.zip && git -C jtdx_contest checkout "$(sed -n 's/^Base: jtdx_contest //p' jtdx_contest-intel-macos/patches/0001-*.patch)"
```

```bash
cd ~/dev/jtdx-prefix && jtdx_contest-intel-macos/patches/apply.sh ~/dev/jtdx-prefix/jtdx_contest
```

It should end with `Applied.`. Once those changes are in the repository, skip this and
build the latest source.

Alternatively, copy the already-patched `jtdx_contest` folder over from the Apple
Silicon Mac, for example with a USB stick or AirDrop (about 70 MB).

### 8.5 Configure, build and package

```bash
mkdir -p ~/dev/jtdx-prefix/build && cd ~/dev/jtdx-prefix/build
```

```bash
cmake -DCMAKE_PREFIX_PATH=/opt/local/libexec/qt5 -DCMAKE_Fortran_COMPILER=/opt/local/bin/gfortran-mp-14 -DCMAKE_OSX_DEPLOYMENT_TARGET=10.15 -DWSJT_GENERATE_DOCS=OFF -DWSJT_SKIP_MANPAGES=ON ../jtdx_contest
```

The configure output should include `Building for for: Darwin-x86_64` and list Hamlib in
`/usr/local`.

```bash
make -j$(sysctl -n hw.ncpu)
```

```bash
make package
```

Result: **`build/jtdx-3.0.0-rc08-Darwin-x86_64.dmg`**. `make package` prints
`Minimum macOS for …: 10.15`, and the ReadMe in the DMG says "Intel Macs with macOS 10.15
or later". Signing also runs here. Intel Macs would run the app unsigned, but
a valid signature does no harm, and the install step verifies the bundle either way.

### 8.6 Check it

Do the checks from [step 6](#6-check-the-installer). Then check that nothing requires a
macOS newer than 10.15; this should print nothing:

```bash
M=$(hdiutil attach -readonly -nobrowse ~/dev/jtdx-prefix/build/jtdx-*-Darwin-x86_64.dmg | tail -1 | cut -f3-) && find "$M/JTDX_contest.app" -type f \( -perm +111 -o -name "*.dylib" \) -exec sh -c 'v=$(otool -l "$1" | awk "/minos|LC_VERSION_MIN_MACOSX/{f=1} f&&/(minos|version) /{print \$2; exit}"); case "$v" in 10.1[0-5]*|10.[0-9]|10.[0-9].*|"") ;; *) echo "$v $1";; esac' _ {} \; ; hdiutil detach "$M"
```

Then copy the DMG off the Air and delete the ~300 MB of staging files in
`build/_CPack_Packages`.

### 8.7 If something fails

| Symptom | Fix |
|---|---|
| `port install` can't find a binary package and starts compiling for hours | That package has no ready-built version for 10.15 (yet). Let it finish, or try again later; the MacPorts build servers catch up. |
| cmake: `Could not find a package configuration file provided by "Qt5LinguistTools"` | `sudo port install qt5-qttools` |
| cmake warns `RIGCTL_EXE not found` | Hamlib isn't installed in `/usr/local`, or `/usr/local/bin` isn't on `PATH`. Redo [8.3](#83-build-hamlib-472). |
| `make package`: `cannot resolve item` / `otool failed` for a gcc library | Check that the gfortran passed to cmake is the MacPorts one (`/opt/local/bin/gfortran-mp-NN`). |
| macOS says JTDX\_contest needs a newer macOS | That Mac's macOS is older than the one the DMG was built on (10.15 for the Intel DMG). |

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
- `gp_resolve_item` reports this through an optional seventh argument.

This change only touches libraries that used to stop packaging with an error.

Reading a file's `LC_RPATH`s is now one function, `gp_item_rpaths`, used both here and
by `get_item_rpaths` in `CMake/Modules/BundleUtilities.cmake`. It caches the result per
file, because packaging asks about the same libraries many times, and each answer is
an `otool` run. `BundleUtilities` clears the cache whenever it copies a file into the
bundle or rewrites one with `install_name_tool`, so a changed file is read again.

### 6. The packaged app would not run, or would fail unclearly on an older macOS

**Cause.**

- `fixup_bundle` rewrites install names with `install_name_tool`, which invalidates
  the code signatures. Apple Silicon refuses to run unsigned code.
- The app didn't say which macOS it needs. Its own programs claim 10.12/11.0, but the
  bundled libraries need the build machine's macOS. On an older system the app would
  fail while loading a library, instead of macOS saying it needs a newer version.

**Fix.** New directory `Darwin/finish_bundle`:

- `CMakeLists.txt` finds `otool`, `lipo`, `plutil` and `codesign`, and installs the two
  scripts in this order.
- `bundle_code.cmake` lists the binaries in the bundle for both scripts, and finds the
  tools again at install time if the ones found at configure time have gone.
- `min_macos.cmake.in`:
  - Reads the minimum macOS of every binary in the installed bundle
    (`LC_BUILD_VERSION` `minos`, or `LC_VERSION_MIN_MACOSX`), for each architecture in
    the main executable (`lipo -info`, which older Command Line Tools also have), and
    takes the highest per architecture.
  - Writes it into `Info.plist` as `LSMinimumSystemVersion` (the lowest per-architecture
    value), plus `LSMinimumSystemVersionByArchitecture` for a universal app.
  - Replaces `{{DMG_FOR}}` in the DMG's `ReadMe.txt`, so each DMG states its own
    requirements, for example "Apple Silicon Macs (M1 and later) with macOS 26 or later".
    If no version can be worked out, it names just the kind of Mac. A Debug build gets
    a note that it only runs on the Mac that built it.
  - Stops the install if `otool` can't read the app's own executable, or if a bundled
    binary lacks one of the app's architectures (checked with `lipo -info`). It only
    warns about a file that isn't a binary. (`otool` exits with 0 even then, so the
    script looks for load commands in its output.)
  - Counts `arm64` and `arm64e` as one kind of Mac, with the higher minimum.
- `sign_bundle.cmake.in` signs the installed bundle ad hoc, inside-out, without the
  deprecated `--deep`, and then verifies it. It runs second, because the signature seals
  `Info.plist`.
  - If the bundle is not installed, or `codesign` is missing, it warns and skips.
  - If signing or verification fails, it stops the install.

The top-level `CMakeLists.txt` adds this directory **as the very last thing in the
file**, for Debug builds too, because the ReadMe is installed for every build. Install rules run either top level first and then subdirectories in order
(policy CMP0082 OLD, today's behaviour with the 3.7.2 minimum), or all in declaration
order (CMP0082 NEW). Last is after `fixup_bundle` and after the `manpages`/`doc`
installs into the bundle either way. In an earlier version the man pages were
installed after signing and broke the seal.

### 7. `Darwin/ReadMe.txt` (the user notes in the DMG)

Updated for this fork:

- The volume path is `/Volumes/JTDX_contest`, not `/Volumes/JTDX`.
- The app, its menu and the microphone prompt are called JTDX\_contest.
- A stock JTDX no longer needs renaming, because the bundle names differ.
- Says which Mac and which macOS this DMG is for. Packaging fills that in (see 6), so
  the Apple Silicon DMG says "Apple Silicon Macs (M1 and later) with macOS 26 or
  later", and the Intel DMG says what it was built for. The file doesn't describe an
  installer that may not exist.
- Explains how to get past Gatekeeper on the first launch, with separate steps for
  macOS 15 and later and for macOS 14 and earlier.
- Fixed the plist name `com.wsjtx.sysctl.plist` to `com.jtdx.sysctl.plist`.
- Help goes to jtdx\_contest@ce3tsk.com, not to the upstream author. The notes still
  credit Arvo ES1JA, whose JTDX notes they started from.
- The shared memory check says "less than 14680064" is a problem, as larger values
  also work.

`Darwin/developer read me.txt` got the same volume path fix, and a DMG file pattern
(`jtdx-*-Darwin-*.dmg`) that matches the real file name.

### 8. "Unable to create shared memory segment" after every restart

**Cause.** JTDX\_contest needs about 13 MB of System V shared memory, and macOS allows
4 MB. The launch daemon that raises the limit at boot (`com.jtdx.sysctl.plist`) only took
effect for users who copied it into `/Library/LaunchDaemons` themselves with `sudo`. A
`sudo sysctl -w …` on its own is lost at the next restart. Its values (14 MB per segment,
70 MB in total) were also barely above what one instance needs.

**Fix.**

- `main.cpp`, macOS only: when the segment can't be created and the limits are the reason
  (`kern.sysv.shmmax` or `shmall` too small, with `shmall` counted in pages of
  `hw.pagesize`), the app asks whether to install the setting. On Yes,
  `osascript … with administrator privileges` shows macOS's password prompt, worded "JTDX\_contest
  wants to install its shared memory setting." rather than the default "osascript wants to
  make changes", and runs the install script as root. Then the app tries again. If the user cancels, or it fails, the
  error message says how to do it by hand. The texts go through `translate ("main", …)`.
- `mac_shared_memory.hpp` (new): the limit check, and the install script. The script
  writes the daemon to `/Library/LaunchDaemons` (owner `root:wheel`, mode 644) and runs
  its command at once, so no reboot is needed.
  - **Everything that runs as root is compiled into the program.** Nothing comes from
    the app bundle, which anything running as the user could change.
  - The daemon's text is compiled in from `Darwin/com.jtdx.sysctl.plist`, through the
    header `jtdx_sysctl_plist.h`, which CMake generates from
    `Darwin/jtdx_sysctl_plist.h.in`. That keeps the plist the one place to edit.
- `Darwin/com.jtdx.sysctl.plist`: 100 MB per segment and 256 MB in total, the page count
  worked out from `hw.pagesize` at boot.
  - Raise-only, so it never lowers a value another program's daemon set higher, in either
    order.
  - `set -e`, so a refused step fails the install instead of hiding behind a later one
    that worked.
  - It keeps stock JTDX's label, which it replaces with values at least as large. It
    still also sits at the top of the DMG for the manual way.
- `main.cpp`, all platforms: a segment left over by a crashed run is only reused if it is
  big enough. Stock JTDX uses the same key ("JTDX"), and its `dec_data` is smaller than
  this fork's (`nsftol` was added), so clearing it would write past its end. A smaller one
  is let go (Qt removes it when nobody else is attached), and a fresh one is created.

### Files changed

| File | Change |
|---|---|
| `CMakeLists.txt` | Fortran OpenMP flags helper, `gomp` link on macOS, `fixup_exe` from `OUTPUT_NAME`, `Darwin/finish_bundle` added last, `jtdx_sysctl_plist.h` generated from the daemon plist |
| `lib/jplsubs.f` | `SPLIT` → `JPLSPLIT` |
| `CMake/Modules/GetPrerequisites.cmake` | `@rpath` resolution through the loader's own rpaths, cached `gp_item_rpaths` |
| `CMake/Modules/BundleUtilities.cmake` | `get_item_rpaths` uses `gp_item_rpaths`, cache cleared on copy and fix-up |
| `Darwin/finish_bundle/CMakeLists.txt` | new: finds `otool`, `lipo`, `plutil` and `codesign`, installs the two scripts |
| `Darwin/finish_bundle/bundle_code.cmake` | new: the bundle's binaries and tool lookup, shared by both scripts |
| `Darwin/finish_bundle/min_macos.cmake.in` | new: minimum macOS into `Info.plist` and the DMG's ReadMe |
| `Darwin/finish_bundle/sign_bundle.cmake.in` | new: ad-hoc inside-out signing and verification |
| `Darwin/ReadMe.txt` | user notes brought up to date, requirements filled in per DMG, shared memory set up by the app |
| `main.cpp` | macOS: offers to install the shared memory setting when the limits are too small |
| `mac_shared_memory.hpp` | new: limit check and the install script run as root, all compiled in |
| `Darwin/jtdx_sysctl_plist.h.in` | new: template for the header that compiles the daemon's text into the program |
| `Darwin/com.jtdx.sysctl.plist` | 100 MB per segment, 256 MB in total (per page size), raise-only, `set -e` |
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
| `gomp` link, `Darwin/finish_bundle`, ReadMe files | macOS only. |

---

## Troubleshooting

| Symptom | Cause and fix |
|---|---|
| `execve: No such file or directory` for `jtdxjt9` when starting the app from `build/` | The helpers aren't in the build-tree bundle. See [step 4](#4-run-from-the-build-directory), or use the DMG. |
| `make package` fails: `file failed to open for writing (Permission denied): …/build/install_manifest.txt` | An earlier `sudo make install` left the file owned by root. Run `sudo chown $USER build/install_manifest.txt`. The same install also left a root-owned `/usr/local/JTDX_contest.app`; remove it with `sudo rm -rf /usr/local/JTDX_contest.app` if unwanted. |
| A pasted `configure` or `cmake` command seems to ignore its options | Typographic dashes and quotes (`–`, `”`) from a word processor or web page. Retype them as plain `-` and `"`. |
| After `brew upgrade gcc`: link errors or "directory not found" for `…/Cellar/gcc/<old version>/…` | CMake caches the Fortran compiler's library directories. Reconfigure in an empty build directory ([step 2](#2-configure)). |
| `Unable to create shared memory segment` | The shared memory setting isn't installed. The app offers to install it; if that was declined, see [step 1.3](#13-shared-memory). |
| `codesign not found: … is NOT re-signed` during install | Install the Xcode Command Line Tools. Without a signature the app won't start on Apple Silicon. |
| The app from the DMG won't open on another Mac ("damaged" / "cannot be opened") | Gatekeeper, see [step 7](#7-distributing-the-dmg). Also check the Mac is Apple Silicon with a new enough macOS. |
