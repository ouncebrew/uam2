# UAM2 - drako3d shader compiler

```
Usage: uam2 [options] file
Options:
  -o, --out=<file>   Specifies the output deko3d shader module file (.dksh)
  -r, --raw=<file>   Specifies the file to which output raw SM 8.6 bytecode
  -a, --asm=<file>   Specifies the file to which output NAK assembly
  -s, --stage=<name> Specifies the pipeline stage of the shader
                     (vert, tess_ctrl, tess_eval, geom, frag, comp)
  -v, --version      Displays version information
```

UAM2 is the shader compiler for drako3.

UAM2, unlike drako3d, does not try to maintain any backwards compatibility with UAM. UAM can
and should be installed and used alongside UAM2 for Switch 1 support.

UAM2 maintains UAM's command line, GLSL rules, and DKSH output, but generates code with
[Mesa](https://www.mesa3d.org/)'s NAK backend and emits the shader ABI that drako3d uses on the
Switch 2.

## Status

Currently supported:

- GLSL and SPIR-V input
- Vertex, tessellation control/evaluation, geometry, fragment, and compute shaders
- DKSH output (`-o`), raw SM 8.6 code (`-r`), and NAK assembly (`-a`)

## Differences with UAM

- The `DEKO3D` preprocessor symbol is defined with a value of 200 (UAM uses 100)
- In compute shaders, uniform buffers 0-4 are hardware constant buffers, while 5-15 are read through
  their buffer descriptor.
- The work group size must be a compile-time constant.

## Building

Requires meson >= 1.7, ninja, a C/C++ compiler, rustc (and cargo), bindgen (with cbindgen),
python3 with mako, flex, bison, SPIRV-Tools, zlib, zstd, libdrm, and expat.

```
meson setup build
ninja -C build
```

### libuam2rt

`libuam2rt` is a static library for homebrew to link against that compiles SPIR-V to DKSH at
runtime (see `include/uam2rt.h`). This adds ~4.5MB to the output nro, so it's best if you can
pre-compile shaders beforehand with uam2. libuam2rt exists for the scenarios when that isn't possible.

To build:
```
meson setup build-switch2 --cross-file cross/switch2.cross -Druntime=true
ninja -C build-switch2 install
```

This installs `libuam2rt.a`, `uam2rt.h`, and `uam2rt.pc` into `$DEVKITPRO/portlibs/switch2`.

It can be easily linked into applications via -luam2rt -lstdc++ linker flags or `pkg-config --libs uam2rt`
