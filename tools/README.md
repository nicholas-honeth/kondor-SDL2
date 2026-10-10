# Reconstruction tools

These scripts were used to recover the source from `KONDOR.EXE` (v1.0, 221323 bytes).
They are not needed to build or play the game.

1. `unemu87.py in.exe out.exe 0x15B2` undoes Borland's 8087-emulator fix-ups
   (`INT 34h`–`3Dh` back to x87 opcodes), so disassemblers and decompilers see
   real FPU code.
2. `KEXE=KONDOR.EXE python3 tdc.py outdir` parses the Turbo Debugger information appended
   to the EXE, using `tdtypes.py` and the demangler `kondor_apply_demangle.py`. It writes
   `types.h` and `spec.json`: types with member offsets, globals, functions, parameters
   and locals, and the source file and line range of each function. It needs the
   `construct` package and `tdinfo_structs.py` from ramikg/tdinfo-parser, found through
   `$TDINFO_PARSER` (default `~/re/tdinfo-parser`).
3. `KondorApply.java` is a headless Ghidra 12 script. It applies the spec (types,
   class namespaces, signatures, locals), fixes Ghidra's mis-wrapped
   `push cs; call near` targets, and decompiles every game function into
   per-source-file drafts:
   `analyzeHeadless <proj> kondor -process KONDOR.EXE -noanalysis -readOnly -scriptPath tools -postScript KondorApply.java spec.json outdir`
4. `annasm.py exe spec.json outdir` writes annotated assembly per source file. It
   shows local names for `bp` slots, `global.member` paths, float constants, `this->`
   members and call targets. The x87 expressions were translated from these listings,
   because the decompiler loses track of the FPU stack across calls.
5. `fuzzkeys.py seed frames` generates random key scripts for sanitizer runs
   (`KONDOR_SCRIPT`). `fuzz.sh binary gamedir first last frames [jobs]` runs a batch of
   them.

`docs/DECOMPILATION.md` walks through the whole pipeline with commands, and
`docs/TESTING.md` covers the tests.
