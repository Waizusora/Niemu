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
