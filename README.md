# Niemu — Nice Emulator

Niemu is an independent Android-hosted, AArch64 full-system emulator. The project is being developed from original source code and is not derived from QEMU TCG or Dynarmic.

## Project status

**Early foundation / not yet bootable.** The current milestone establishes the native C++20 build, a minimal core library, unit-test entry point, and continuous integration. CPU execution, system emulation, storage formats, Android UI, and guest boot are planned milestones—not implemented capabilities.

## Design goals

- Host platform: Android (initial target: arm64-v8a; x86_64 for development/testing).
- Guest architecture: AArch64.
- Native implementation: C++20, with Kotlin/JNI for Android integration.
- Execution engines: interpreter first, then an independently implemented DBT pipeline and AArch64 JIT backend.
- Machine model: a defined ARM Generic Virtual Platform-compatible profile, with explicitly documented device models.
- Storage: RAW first, then a native QCOW2 v3 implementation based on the public format specification.
- Rendering/input: SDL3 integration, scalable guest framebuffer, virtual mouse and keyboard.
- License target: BSD-3-Clause for Niemu-authored code. Third-party dependencies retain their own licenses and will be audited separately.

## Planned repository layout

```text
app/                 Android application, Kotlin UI and JNI bridge
core/
  cpu/               Guest architectural state
  decoder/           AArch64 instruction decoding
  interpreter/       Reference execution engine
  ir/                DBT intermediate representation
  dbt/               Translation blocks and code cache
  optimizer/         IR optimization passes
  jit/aarch64/       Host AArch64 code generation
  memory/            Guest physical memory
  mmu/               Guest address translation
  exception/         Guest exception and privilege model
machine/
  arm_gvp/           Selected virtual machine profile
  gic/               Interrupt controller model
  timer/             Generic timer model
  uart/              Serial device model
  virtio/            VirtIO device framework
storage/
  block/             Block-device abstraction
  raw/               RAW disk backend
  qcow2/             Native QCOW2 v3 backend
frontend/sdl3/       SDL3 display and input frontend
platform/android/    Android platform integration
firmware/            Firmware and boot assets
tests/               Native tests and fixtures
docs/                Architecture and format documentation
```

The layout will be introduced incrementally as each subsystem has a buildable implementation.

## Build native core

Requirements: CMake 3.20+, and a C++20 compiler.

```sh
cmake -S . -B build -DNIEMU_BUILD_TESTS=ON
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

## Engineering constraints

- Guest privileged instructions are modeled by the emulator; they are never executed directly as host privileged instructions.
- The interpreter is the semantic reference for future DBT/JIT implementations.
- Guest virtual CPU state, exception levels, MMU, interrupts, timers, and devices remain explicit emulator state.
- Storage backends are independent of the CPU execution engine.
- No third-party source is copied into Niemu without license and provenance review.

## Documentation

Architecture notes and implementation decisions live in [docs/](docs/).

## License

Niemu-authored project code is intended to be distributed under the BSD 3-Clause License. See [LICENSE](LICENSE).