<p align="center">
  <a href="https://whileone.me">
    <img src="https://whileone.me/images/caffeine-small.png" alt="Caffeine Logo" width="384" height="384">
  </a>
</p>

<p align="center">
  <img src="https://img.shields.io/badge/C-11-blue.svg?style=flat-square&logo=c" alt="C11">
  <img src="https://img.shields.io/badge/CMake-%23008FBA.svg?style=flat-square&logo=cmake&logoColor=white" alt="CMake">
  <a href="https://github.com/while-one/caffeine-sal-impl/actions/workflows/ci.yml">
    <img src="https://img.shields.io/github/actions/workflow/status/while-one/caffeine-sal-impl/ci.yml?style=flat-square&branch=main" alt="CI Status">
  </a>
  <a href="https://github.com/while-one/caffeine-sal-impl/blob/main/LICENSE">
    <img src="https://img.shields.io/github/license/while-one/caffeine-sal-impl?style=flat-square&color=blue" alt="License: MIT">
  </a>
</p>

# Caffeine-Services-Impl

This repository contains concrete implementations for the abstract middleware services defined in the `caffeine-sal` header-only library. It acts as a collection of platform-agnostic device drivers (e.g., specific sensors, memory devices, displays) and utility modules (e.g., specific file systems, networking stacks) built strictly on top of the generic `caffeine-hal` interface.

## Repository Structure

Implementations are organized by their service category:
*   `src/devices/`:
    *   `led/`: Basic LED driver (Sink/Source logic).
    *   `button/`: GPIO-based button with debounce.
    *   `temp_sensor/`: SHT30, RMP117.
    *   `accel/`: BMA530, LIS2DH12.
    *   `combined_sensor/`: BME280 (Temperature + Humidity + Pressure).
*   `src/utilities/`:
    *   `cli/`: Command Line Interface.
    *   `collection/`: Linked List, Ring Buffer.

## Architecture

This repository follows the architectural mandates of the `caffeine-sal` layer:
*   **Generic PHY**: All implementations use the unified `cfn_sal_phy_t` for hardware mapping.
*   **Shared Context Pattern**: Combination sensors (like the BME280) use reference-counted shared contexts to manage physical hardware while exposing multiple logical interfaces.

## Development & Testing

This repository uses the unified `caffeine-build` system. To verify your implementations locally (using host mocks):

1.  **Format Code:** `./caffeine-build/scripts/build.sh tests-native caffeine-sal-impl-format`
2.  **Run Static Analysis:** `./caffeine-build/scripts/build.sh tests-native caffeine-sal-impl-analyze`
3.  **Run Unit Tests:** `./caffeine-build/scripts/build.sh tests-native all`

---

## License

This project is licensed under the **MIT License**. See the [LICENSE](LICENSE) file for details.
