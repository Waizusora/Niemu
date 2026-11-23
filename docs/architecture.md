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

The native core now includes a bounded guest-memory implementation and scalar memory bus. The interpreter can execute a limited set of AArch64 integer instructions, including unsigned-immediate GPR loads and stores. This remains an early execution subset; MMU translation, architectural data-abort delivery, device memory, and guest boot are not implemented.


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


## Initial interpreter

The first interpreter slice executes the decoded MOVN/MOVZ/MOVK, ADD/ADDS/SUB/SUBS immediate, B/BL, B.cond, BR/BLR, and RET instruction families. It updates the guest PC and the implemented architectural flags. Load/store instructions are decoded and the supported unsigned-immediate GPR subset executes through the guest memory bus. Architectural exception delivery remains future work. Unsupported instructions return UnsupportedInstruction and do not advance PC.

The interpreter is the semantic reference for future DBT/JIT work. This subset is not sufficient to boot Linux or claim general AArch64 compatibility.


## Guest memory and scalar bus

The initial memory layer provides a zero-initialized, bounded guest RAM region and a scalar bus for 8-, 16-, 32-, and 64-bit accesses. Scalar values are assembled and serialized explicitly in little-endian order, independent of host byte order. All ranges are validated without computing an unchecked end address; out-of-range accesses fail without exposing host pointers.

The default alignment policy rejects misaligned multi-byte scalar accesses. A memory instance may instead allow unaligned accesses. This is a configurable initial policy, not a complete implementation of AArch64 alignment behavior: SCTLR controls, memory attributes, stage-1/stage-2 translation, permission checks, and architectural data-abort exceptions are future work.

The interpreter now executes integer GPR LDR/STR unsigned-immediate instructions through an optional memory bus. Base register encoding 31 selects SP; transfer register encoding 31 selects XZR/WZR. Successful W-register loads zero-extend naturally, and PC advances only after a successful access. Missing buses, address overflow, and memory faults return distinct execution statuses and leave PC unchanged.


## Guest physical address space

The guest physical address space maps non-overlapping guest-physical ranges onto bounded RAM backing stores. Each mapping has an explicit base, size, backing offset, and read/write/execute permission mask. The bus resolves accesses through the map and enforces permissions before touching backing RAM. Accesses that are unmapped, cross a region boundary, overflow the address range, or violate permissions fail deterministically.

This is the physical mapping layer, not an MMU. It does not yet interpret AArch64 translation table descriptors, walk guest page tables, implement TCR/TTBR/MAIR controls, model stage-1 or stage-2 translation, or route MMIO to devices. The execute permission is exposed for the forthcoming instruction-fetch path; the current interpreter still receives decoded instructions from its caller.
