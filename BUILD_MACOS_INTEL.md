# JTDX\_contest for Intel Macs: build instructions

These steps build **`jtdx-3.0.0-rc08-Darwin-x86_64.dmg`**, the JTDX\_contest installer for
Intel Macs, on an Intel Mac, straight from the source on GitHub. No patches are needed:
all the macOS work is in the repository, since commit `ad1c358` ("macOS fixes for installer
and shared memory issue handling").

- **Build machine:** MacBook Air 13-inch, Mid 2012 (A1466, EMC 2559, `MacBookAir5,2`) on
  its last macOS, 10.15.7 Catalina.
- **Result:** a DMG that runs on **Intel Macs with macOS 10.15 or later**, which covers
  practically every Intel Mac from 2012 onwards.

## What's in this zip

| File | What it is |
|---|---|
| `README-INTEL.md` | this description: the steps for the Air, in order |
| `BUILD_MACOS.md` | the full macOS build guide from the repository, to read before cloning; section 8 is the Intel build. Section 8.4 is newer here than in `ad1c358`: it no longer mentions a patch kit. |

## What the source already does for Intel

Everything below is in the repository; it is listed here so you know what to expect.

- **Builds with MacPorts' gfortran:** the Fortran OpenMP fix (without it `decoder.f90`
  fails with "Type mismatch in argument 'ncores'" on Intel too).
- **Self-contained DMG:** packaging copies Qt, gfortran, FFTW, Hamlib and libusb into the
  app and signs it.
- **States its own requirements:** packaging works out the minimum macOS (10.15 here) and
  writes it into the app's `Info.plist` and into the DMG's ReadMe, which then reads "This
  installer is for Intel Macs with macOS 10.15 or later". An Intel Mac on an older macOS
  gets a clear "needs a newer macOS" message, not a crash.
- **Sets up shared memory itself:** whenever JTDX\_contest starts and finds macOS's shared
  memory limit too small, it offers to raise it. Answer Yes, then enter the password in the
  dialog "JTDX\_contest wants to install its shared memory setting.", and the setting stays
  (100 MB per segment, 512 MB in total).

## Steps on the MacBook Air

About **5 GB** free space is needed on the macOS partition. Expect a few hours in total,
most of it MacPorts installing, and 30–60 minutes for the JTDX\_contest build itself. Type
or copy the commands with plain ASCII `-` and `"`: a command pasted through a word
processor gets typographic characters, and its options are then silently ignored.

Section 8 of `BUILD_MACOS.md` explains each step; the commands below are the same.

**1. macOS 10.15.7.** Update through System Preferences → Software Update. If it isn't
offered, fetch the installer and run it from Applications:

```bash
softwareupdate --fetch-full-installer --full-installer-version 10.15.7
```

**2. Command Line Tools** (compiler, git, codesign):

```bash
xcode-select --install
```

**3. MacPorts.** Install the macOS 10.15 Catalina package from
https://www.macports.org/install.php, then:

```bash
sudo port selfupdate
```

**4. Packages:**

```bash
sudo port install gcc14 cmake pkgconfig qt5-qtbase qt5-qtmultimedia qt5-qtserialport qt5-qtwebsockets qt5-qtsvg qt5-qttools fftw-3-single +gfortran boost libusb autoconf automake libtool
```

```bash
sudo port clean --all installed && sudo port reclaim
```

If `gcc14` isn't available, run `port search --name --glob 'gcc1*'`, take the newest
`gccNN`, and change `gfortran-mp-14` in step 7 to match.

**5. Hamlib 4.7.2 from git,** built for macOS 10.15:

```bash
mkdir -p ~/dev && cd ~/dev && git clone --branch 4.7.2 --depth 1 https://github.com/Hamlib/Hamlib.git
```

```bash
cd ~/dev/Hamlib && ./bootstrap && ./configure --prefix=/usr/local --enable-shared --disable-static --disable-winradio --without-readline --without-indi --without-cxx-binding CFLAGS="-g -O2 -mmacosx-version-min=10.15 -I/opt/local/include" LDFLAGS="-mmacosx-version-min=10.15" LIBUSB_LIBS="-L/opt/local/lib -lusb-1.0"
```

```bash
make -j$(sysctl -n hw.ncpu) && sudo make install
```

Check: `rigctl --version` shows `Hamlib 4.7.2`.

**6. JTDX\_contest source,** the latest from GitHub:

```bash
mkdir -p ~/dev/jtdx-prefix && cd ~/dev/jtdx-prefix && git clone https://github.com/ce3tsk/jtdx_contest.git
```

`git -C ~/dev/jtdx-prefix/jtdx_contest log --oneline -1` should show `ad1c358` or a later
commit.

**7. Configure, build, package:**

```bash
mkdir -p ~/dev/jtdx-prefix/build && cd ~/dev/jtdx-prefix/build
```

Export `SDKROOT` in the same shell where you run `cmake` and `make`. Without
it, the Command Line Tools compiler does not add `-isysroot`, so headers that
live in the SDK (`<stdio.h>`, the `OpenGL.framework` headers Qt5Gui needs, and
so on) are reported as missing:

```bash
export SDKROOT=$(xcrun --show-sdk-path)
```

```bash
cmake -DCMAKE_PREFIX_PATH=/opt/local/libexec/qt5 -DCMAKE_Fortran_COMPILER=/opt/local/bin/gfortran-mp-14 -DCMAKE_OSX_DEPLOYMENT_TARGET=10.15 -DCMAKE_OSX_SYSROOT=$SDKROOT -DCMAKE_BUILD_RPATH=/opt/local/lib/libgcc -DCMAKE_INSTALL_RPATH=/opt/local/lib/libgcc -DWSJT_GENERATE_DOCS=OFF -DWSJT_SKIP_MANPAGES=ON ../jtdx_contest
```

`-DCMAKE_OSX_SYSROOT=$SDKROOT` is what lets CMake redirect Qt5Gui's hardcoded
`/System/Library/Frameworks/OpenGL.framework/Headers` lookup into the SDK,
where those headers actually live on 10.14 and later.

`-DCMAKE_BUILD_RPATH=/opt/local/lib/libgcc` and
`-DCMAKE_INSTALL_RPATH=/opt/local/lib/libgcc` together add a runtime search
path so every binary can find `libgomp.1.dylib` (gfortran-mp-14's OpenMP
runtime, linked with install\_name `@rpath/libgomp.1.dylib`). Both are needed:
the project sets `CMAKE_BUILD_WITH_INSTALL_RPATH FALSE`, so CMake strips the
build rpath during install and uses the install rpath instead. Without the
install rpath the *staged* binary that CPack hands to `fixup_bundle` has no
rpath, and packaging fails with "otool can't open file:
@rpath/libgomp.1.dylib" because the bundle-fixup step can't resolve the
dependency to copy it into the `.app`.

```bash
make -j$(sysctl -n hw.ncpu) && make package
```

`make package` should print `Minimum macOS for …: 10.15` and end with
`package: …/jtdx-3.0.0-rc08-Darwin-x86_64.dmg generated.`

**8. Check the DMG.** Section 8.6 of `BUILD_MACOS.md` has the full check. Quick version:

```bash
M=$(hdiutil attach -readonly -nobrowse ~/dev/jtdx-prefix/build/jtdx-*-Darwin-x86_64.dmg | tail -1 | cut -f3-) && codesign --verify --deep --strict "$M/JTDX_contest.app" && /usr/libexec/PlistBuddy -c "Print :LSMinimumSystemVersion" "$M/JTDX_contest.app/Contents/Info.plist" && head -12 "$M/ReadMe.txt" | tail -2; hdiutil detach "$M"
```

This should print `10.15`, and the ReadMe line "This installer is for Intel Macs with
macOS 10.15 or later".

**9. Try it on the Air.** Open the DMG, drag JTDX\_contest to Applications and start it. On
the first start macOS may refuse the unsigned app: right-click it, choose Open, and confirm
(macOS 10.15 still offers that). When JTDX\_contest asks about shared memory, answer Yes and
enter your password.

**10. Finish.** Copy the DMG off the Air. Then delete `~/dev/jtdx-prefix/build/_CPack_Packages`
(about 300 MB) and, if space is tight, the tools needed only for Hamlib:
`sudo port uninstall autoconf automake libtool`.

## What has and hasn't been tested

- **Tested on the Apple Silicon Mac,** from GitHub commit `ad1c358`: the source builds
  and packages there. The DMG verifies, is self-contained, records its minimum macOS (26.0)
  in `Info.plist` and its ReadMe, carries the 512 MB shared memory setting, and has the new
  texts in all translations.
- **Tested on the Intel Mac** (2026-09-26, macOS 10.15): the source builds and packages
  with the four cmake flags in step 7 (`SDKROOT` in the environment, plus
  `CMAKE_OSX_SYSROOT`, `CMAKE_BUILD_RPATH`, `CMAKE_INSTALL_RPATH` on the command line). The
  DMG installs, launches, decodes from the microphone and transmits.
- **libgcc\_s reexport patch, in `Darwin/finish_bundle/sign_bundle.cmake.in`:**
  `fixup_bundle` copies `libgcc_s.1.dylib` and `libgcc_s.1.1.dylib` into `Contents/MacOS`
  but does not rewrite the reexport reference `@rpath/libgcc_s.1.1.dylib` inside
  `libgcc_s.1.dylib`, and it has already stripped the rpaths from the binaries. The loader
  then cannot find `libgcc_s.1.1.dylib` and the decoder aborts with "Reason: image not
  found" - JTDX\_contest shows "decoder could not be run". The patch rewrites the reference
  to `@loader_path/libgcc_s.1.1.dylib` before ad hoc re-signing.
- **Worth re-checking on Apple Silicon:** the patch above runs there too (the
  `sign_bundle.cmake` script has no arch guard, and its `EXISTS` guards make it a no-op if
  the two files are not in the bundle). It is likely needed - MacPorts gcc14 ships the
  same `libgcc_s.1.dylib` reexport stub on arm64 - but the Apple Silicon test on `ad1c358`
  may not have exercised the decoder path.
