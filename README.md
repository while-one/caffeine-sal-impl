<p align="center">
  <a href="https://whileone.me">
    <img src="https://whileone.me/images/caffeine-small.png" alt="Caffeine Logo" width="384" height="384">
  </a>
</p>

<p align="center">
  <img src="https://img.shields.io/badge/C-11-blue.svg?style=flat-square&logo=c" alt="C11">
  <img src="https://img.shields.io/badge/CMake-%23008FBA.svg?style=flat-square&logo=cmake&logoColor=white" alt="CMake">
  <a href="https://github.com/while-one/caffeine-services-impl/actions/workflows/ci.yml">
    <img src="https://img.shields.io/github/actions/workflow/status/while-one/caffeine-services-impl/ci.yml?style=flat-square&branch=main" alt="CI Status">
  </a>
  <a href="https://github.com/while-one/caffeine-services-impl/blob/main/LICENSE">
    <img src="https://img.shields.io/github/license/while-one/caffeine-services-impl?style=flat-square&color=blue" alt="License: MIT">
  </a>
</p>

# Caffeine-Services-Impl

This repository contains concrete implementations for the abstract middleware services defined in the `caffeine-services` header-only library. It acts as a collection of platform-agnostic device drivers (e.g., specific sensors, memory devices, displays) and utility modules (e.g., specific file systems, networking stacks) built strictly on top of the generic `caffeine-hal` interface.

## Repository Structure

Implementations are organized by their service category:
*   `src/devices/`:
    *   `led/`: Basic LED driver (Sink/Source logic).
    *   `button/`: GPIO-based button with debounce.
    *   `temp_sensor/`: SHT30, SHT40, RMP117.
    *   `accel/`: BMA530, LIS2DH12.
*   `src/utilities/`:
    *   `cli/`: Command Line Interface.
    *   `collection/`: Linked List, Ring Buffer.

## Development & Testing

This repository uses the unified `caffeine-build` system. To verify your implementations locally (using host mocks):

1.  **Format Code:** `./caffeine-build/scripts/build.sh tests-native caffeine-services-impl-format`
2.  **Run Static Analysis:** `./caffeine-build/scripts/build.sh tests-native caffeine-services-impl-analyze`
3.  **Run Unit Tests:** `./caffeine-build/scripts/build.sh tests-native all`

---

## License

This project is licensed under the **MIT License**. See the [LICENSE](LICENSE) file for details.
