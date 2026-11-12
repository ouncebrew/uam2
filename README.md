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

- SPIR-V input
- Raw SM 8.6 code (`-r`) and NAK assembly (`-a`) output

Not yet implemented: GLSL input, DKSH output, the drako3d binding ABI, and the graphics stages.

## Building

Requires meson >= 1.7, ninja, a C/C++ compiler, rustc (and cargo), bindgen (with cbindgen),
python3 with mako, flex, bison, SPIRV-Tools, zlib, zstd, libdrm, and expat.

```
meson setup build
ninja -C build
```

