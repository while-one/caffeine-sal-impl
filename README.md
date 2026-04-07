<p align="center">
  <a href="https://whileone.me">
    <img src="https://raw.githubusercontent.com/while-one/caffeine-build/main/assets/logo.png" alt="Caffeine Logo" width="50%">
  </a>
<h1 align="center">The Caffeine Framework</h1>
</p>

# Caffeine-SAL-Impl

<p align="center">
  <img src="https://img.shields.io/badge/C-11-blue.svg?style=flat-square&logo=c" alt="C11">
  <img src="https://img.shields.io/badge/CMake-%23008FBA.svg?style=flat-square&logo=cmake&logoColor=white" alt="CMake">
  <a href="https://github.com/while-one/caffeine-sal-impl/tags">
    <img src="https://img.shields.io/github/v/tag/while-one/caffeine-sal-impl?style=flat-square&label=Release" alt="Latest Release">
  </a>
  <a href="https://github.com/while-one/caffeine-sal-impl/actions/workflows/ci.yml">
    <img src="https://img.shields.io/github/actions/workflow/status/while-one/caffeine-sal-impl/ci.yml?style=flat-square&branch=main" alt="CI Status">
  </a>
  <a href="https://github.com/while-one/caffeine-sal-impl/commits/main">
    <img src="https://img.shields.io/github/last-commit/while-one/caffeine-sal-impl.svg?style=flat-square" alt="Last Commit">
  </a>
  <a href="https://github.com/while-one/caffeine-sal-impl/blob/main/LICENSE">
    <img src="https://img.shields.io/github/license/while-one/caffeine-sal-impl?style=flat-square&color=blue" alt="License: MIT">
  </a>
</p>

This repository contains concrete implementations for the abstract middleware services defined in the `caffeine-sal` header-only library. It acts as a collection of platform-agnostic device drivers (e.g., specific sensors, memory devices, displays) and utility modules (e.g., specific file systems, networking stacks) built strictly on top of the generic `caffeine-hal` interface.

## Directory Structure

*   `caffeine-build/`: Submodule containing centralized build presets, toolchains, scripts, and hardware target definitions.
*   `src/devices/`:
    *   `led/`: Basic LED driver (Sink/Source logic).
    *   `button/`: GPIO-based button with debounce.
    *   `temp_sensor/`: SHT30, RMP117.
    *   `accel/`: BMA530, LIS2DH12.
    *   `combined_sensor/`: BME280 (Temperature + Humidity + Pressure).
*   `src/utilities/`:
    *   `cli/`: Command Line Interface.
    *   `collection/`: Linked List, Ring Buffer.

---

## Documentation

The API reference for this repository is generated automatically via Doxygen and hosted on GitHub Pages.

📚 **[View the Caffeine-SAL-Impl API Documentation](https://while-one.github.io/caffeine-sal-impl)**

---

## Architecture

This repository follows the architectural mandates of the `caffeine-sal` layer:
*   **Generic PHY**: All implementations use the unified `cfn_sal_phy_t` for hardware mapping.
*   **Composite Struct Pattern**: Combination sensors (like the BME280) use composite structures containing all logical interfaces and a standardized `cfn_sal_combined_state_t`. Navigation uses the `CFN_HAL_CONTAINER_OF` macro for type-safe, deterministic parent retrieval.

## Development & Testing

This repository uses the framework's centralized scripts for orchestration. To verify your implementations locally (using host mocks):

```bash
# Full Quality Gate (Format -> Analyze -> Build -> Test)
./caffeine-build/scripts/ci.sh all

# Perform a clean build and run tests for the unit-test preset
./caffeine-build/scripts/build.sh --clean unit-tests-gtest all
```

---

## Support the Gallery

While this library is no Mondrian, it deals with a different form of **abstraction art**. Hardware abstraction is a craft of its own—one that keeps your application code portable and your debugging sessions short.

Whether Caffeine is fueling an elegant embedded project or just helping you wake up your hardware, you can contribute in the following ways:

* Star & Share: If you find this project useful, give it a ⭐ on GitHub and share it with your fellow firmware engineers. It helps others find the library and grows the Caffeine community.
* Show & Tell: If you are using Caffeine in a project (personal or professional), let me know! Hearing how it's being used is a huge motivator.
* Propose Features: If the library is missing a specific "brushstroke," let's design the interface together.
* Port New Targets: Help us expand the collection by porting the HAL to new silicon or peripheral sets.
* Expand the HIL Lab: Contributions go primarily toward acquiring new development boards. These serve as dedicated Hardware-in-the-Loop test targets, ensuring every commit remains rock-solid across our entire fleet of supported hardware.

**If my projects helped you, feel free to buy me a brew. Or if it caused you an extra debugging session, open a PR!**

<a href="https://www.buymeacoffee.com/whileone" target="_blank">
  <img src="https://img.shields.io/badge/Caffeine%20me--0077ff?style=for-the-badge&logo=buy-me-a-coffee&logoColor=white" 
       height="40" 
       style="border-radius: 5px;">
</a>&nbsp;&nbsp;&nbsp;&nbsp;
<a href="https://github.com/sponsors/while-one" target="_blank">
<img src="https://img.shields.io/badge/Sponsor--ea4aaa?style=for-the-badge&logo=github-sponsors" height="40" style="border-radius: 5px;"> </a>&nbsp;&nbsp;&nbsp;
<a href="https://github.com/while-one/caffeine-sal-impl/compare" target="_blank">
<img src="https://img.shields.io/badge/Open%20a%20PR--orange?style=for-the-badge&logo=github&logoColor=white" height="40" style="border-radius: 5px;">
</a>

---

## License

This project is licensed under the **MIT License**. See the [LICENSE](LICENSE) file for details.
