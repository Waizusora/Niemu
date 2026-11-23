# Architecture Overview

## Execution boundary

Niemu models an AArch64 guest as explicit state owned by the emulator. Guest EL0–EL3 state, system registers, exception routing, translation regimes, virtual interrupts, and timer state are not aliases for host execution state. Guest privileged operations must be decoded and modeled.

## Execution engines

The interpreter is the first executable engine and the semantic reference. Later DBT work will introduce a typed intermediate representation, translation blocks, validation, optimization passes, and a code cache. The AArch64 JIT backend will emit host instructions only for operations whose semantics are implemented and tested. Unsupported instructions must take a defined exception or fallback path.

## Machine model

The initial target is a documented, bounded ARM Generic Virtual Platform-compatible machine profile. Compatibility claims will name the exact CPU features, memory map, interrupt controller, timer, UART, and VirtIO devices implemented. The project does not claim compatibility with every Arm FVP configuration.

## Storage

Block devices use a backend-neutral interface. RAW is the first backend. QCOW2 v3 is a separate native implementation based on the public format specification; it is not part of CPU execution and does not reuse QEMU implementation source.

## Android frontend

The Android application owns lifecycle, permissions, SAF document access, and settings. JNI provides a narrow native boundary. SDL3 handles guest presentation and input integration. The guest framebuffer dimensions are independent of the physical Android display dimensions.

## Current milestone

The initial repository milestone contains only the native build skeleton, version API, smoke test, and CI. No guest instruction execution or guest boot is implemented yet.


## AArch64 CPU foundation

The first CPU slice defines a host-independent guest architectural state:

- X0-X30 general-purpose registers; register encoding 31 is handled explicitly as SP or XZR by the instruction-facing accessors.
- Separate SP storage for EL0 through EL3.
- PC and current exception level.
- A compact PSTATE model for NZCV, DAIF, single-step, and illegal-execution-state flags.
- A typed system-register access layer for the implemented NZCV, DAIF, CurrentEL, and SP_ELn registers.

The system-register layer intentionally returns an unavailable result for registers that are not implemented. It does not silently fabricate values. ELR_EL1, ESR_EL1, and FAR_EL1 are reserved in the enum for later exception-model work and are not yet readable or writable.

This is an initial state model, not a complete Arm ARM implementation. In particular, exception entry/return, SPSel semantics, access permissions/traps, system-register encodings, and architectural feature discovery remain future work. Guest privileged state is modeled in software and must never be executed directly on the Android host.


## Initial AArch64 decoder

The decoder currently recognizes a bounded instruction subset:

- MOVN, MOVZ, MOVK wide-immediate encodings.
- ADD, ADDS, SUB, SUBS immediate encodings.
- B, BL, and B.cond.
- BR, BLR, and RET.
- Integer GPR LDR/STR unsigned-immediate encodings.

The decoder returns a typed record containing the raw encoding and extracted operands. It does not execute instructions. Unsupported or reserved encodings are reported as Unsupported. Execution semantics, condition evaluation, memory access faults, and architectural exception delivery belong to the interpreter and subsequent CPU model layers.
