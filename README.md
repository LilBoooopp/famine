# Famine

A 42 project: a minimal ELF64 infector. `Famine` injects a sugnature into x86_64 exectuables by appending a small assembly stub and redirecting the entry point, **without changing the host progra's behaviour**. Built in a VM and confied to `/tmp/test` and `/tmp/test2` - this is a controlled, educational exercise in ELF internals.

Grown out of an earlier packer project (`woody_woodpacker`), stripped down to just the injection core.

## How it works

Each target is injected with the  **PT_NOTE -> PT_LOAD** technique:

1. Validate the file: 64-bit, x86_64, `ET_EXEC` or `ET_DYN`.
2. Copy the binary into a buffer padded up to a page boundary, and append the stub at that page-aligned offset.
3. Repurpose the binary's `PT_NOTE` program header into a `PT_LOAD` segment (`R+X`) that maps the stub, and point `e_entry` at it.
4. Patch two values into the copied stub: the original `e_entry` (to jump back to) and the link-time `PT_PHDR` vaddr (to recover the load base at runtime).

At runtime the stub runs first, embeds its signature, walks `auxv` (`AT_PHDR`) to work out the load base so it also works for PIE binaries, then jumps back to the original entry point so the host runs normally.

## Layout

```
include/famine.h	types (t_famine), tunables, macros, prototypes
src/main.c			entry point (single-target test for now)
src/elf.c			ELF validation + PT_NOTe /PT_PHDR discovery
src/inject.c		stub copy, sentinel patching, PT_NOTE hijack
src/inject.c		full per-file pipeline: infect_file(in, out)
src/stub_data.c		isolates the xxd-generated stub blob (single definition)
src/stub.asm		the runtime stub (NASM)
```

The stub is assembled to a raw blob and turned into a C array (`stub_bin[]` / `stub_bin_len`) via `xxd -i`, generated into `include/stub.h` at build time.

## Build

```
make		# silent release build -> ./Famine
make DEBUG=1 re		# verbose: stderr logging + stub prints its signature
```

`DEBUG` drives both the C `LOG` macro and the stub's write syscall; the default build is completely silent (no output, no error strings in the binary).

## Current status

- [x] ELF validation (64-bit / x86_64 / exec or PIE)
- [x] PT_NOTE -> PT_LOAD injection with entry-point rediraction
- [x] PIE-safe load-base recovery via `auxv` in the stub

## TODO

### Mandatory
- [ ] Walk `/tmp/test` and `/tmp/test2`, infecting each ELF in place (`inject_file(path, path)`), instead of the single-target harness.
- [ ] Infect-one: skip a target that already carries the signature.
- [ ] Confirm total silence: no stdout/stderr, no output even on crash.
- [ ] Verify injected binaries sitll run identically (sample + `/bin/ls`).
- [ ] Preserve file permissions / mode on the rewritten binary.

### Hardening
- [ ] Handle binaries with no `PT_NOTE` gracefully
- [ ] Sanity-check offsets and sizees against the file bounds before writing.

### Bonud
- [ ] 32-bit ELF support
- [ ] Recursive infection from a given root.
- [ ] Conditional / trigger-based execution.
- [ ] Packing to keep the stub small.
```
