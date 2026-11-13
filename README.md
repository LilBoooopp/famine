# Famine

A 42 project: a self-replicating ELF64 infector. `Famine` injects a signature stub into x86_64 executables using the **PT_NOTE → PT_LOAD** technique, **without changing the host program's behaviour**. Infected binaries propagate the stub to other binaries on execution. Confined to `/tmp/test` and `/tmp/test2` — this is a controlled, educational exercise in ELF internals.

## How it works

### Infection

1. Validate the file: 64-bit, x86_64, `ET_EXEC` or `ET_DYN`.
2. Skip if already infected (signature check via `memmem`).
3. Copy the binary into a buffer padded to a page boundary, append the stub.
4. Compute the stub's virtual address from the binary's layout — top of the highest `PT_LOAD`, page-rounded — so it never collides with existing mappings.
5. Repurpose the `PT_NOTE` program header into a `PT_LOAD` segment (`R+X`) mapping the stub, redirect `e_entry`.
6. Patch the original `e_entry` and `PT_PHDR` vaddr into the stub (for load-base recovery).
7. Rewrite the target file in place, preserving original permission bits.

### Runtime

The stub runs first, walks `auxv` (`AT_PHDR`) to recover the load base (PIE-safe), performs its own infection pass on `/tmp/test` and `/tmp/test2`, then jumps to the original entry point. The host program runs normally — no visible change in behavior or output.

### Self-propagation

The stub compiled into infected binaries carries the full infection logic. When an infected binary executes, it infects other ELF64 binaries in the target directories before running the original program. This creates generational spread: binary A infects binary B, running B infects binary C, and so on.

## Layout

```
include/famine.h    types (t_famine), tunables, macros, prototypes
include/m_std.h     NOSTD libc replacements + syscall macros
include/m_syscall.h syscall dispatch macros (variadic)
src/main.c          entry points: host binary + stub _start/my_start
src/elf.c           ELF validation, PT_NOTE/PT_PHDR discovery
src/inject.c        stub copy, sentinel patching, PT_NOTE hijack
src/infect.c        per-file infection pipeline
src/walk.c          directory traversal (recursive with -r flag)
src/std.c           NOSTD syscall wrappers + string primitives
src/x64-syscall.c   raw x86_64 syscall0–6 (inline asm)
src/stub_data.c     xxd-generated stub blob (stub_bin[] / stub_bin_len)
```

The stub is compiled as a freestanding PIE (`-nostdlib -ffreestanding`), linked with a custom `linker.ld` that enforces a single section (no `.data`/`.bss`), extracted to a raw binary via `objcopy`, and turned into a C header via `xxd -i`.

## Build

```
make            # release build -> ./Famine (silent)
make DEBUG=1 re # verbose: stderr logging
make re         # clean rebuild
make fclean     # remove everything
```

`DEBUG` drives the `LOG` macro; the default build is completely silent — no stdout, no stderr, no output even on error.

## Usage

```bash
./Famine                        # infect /tmp/test and /tmp/test2
./Famine <file>                 # infect a specific file
./Famine <dir>/                 # infect binaries in a directory
./Famine -r <dir>/ [more...]    # recursive infection
```

When given explicit targets, the default directories are not processed.

## Test

```bash
./test.sh           # full regression suite
./test.sh --rebuild # force make re first
```

Covers: infection correctness, idempotency, self-propagation, generational spread, permission preservation, rejection of non-targets (non-ELF, 32-bit, truncated, empty), containment (symlinks, sibling dirs), argument parsing, recursive flag, robustness (missing dirs, malformed headers), and memory management (valgrind).

## Features

- [x] ELF64 infection (PIE + non-PIE)
- [x] PT_NOTE → PT_LOAD injection with entry-point redirection
- [x] Stub vaddr computed from binary layout (no fixed constant)
- [x] PIE-safe load-base recovery via `auxv`
- [x] In-place infection, preserving permissions
- [x] Signature guard — skip already-infected binaries
- [x] Self-propagation — infected binaries infect on execution
- [x] Generational spread (A→B→C)
- [x] Total silence — no stdout/stderr output
- [x] Directory traversal with recursive flag (`-r`)
- [x] Explicit file/directory targeting via arguments
- [x] Robust against malformed inputs (no crash, no output)

### Bonus
- [x] Recursive infection from OS root
