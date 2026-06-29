# RG52 Mini RK3562 kernel source — v267 image

## Device

**AISLPC RG52 Mini** (RK3562 ARM Cortex-A55 quad-core), handheld gaming console.
ROM: LineageOS 20 / Android 13 (userdebug, API 33).

This repository contains the complete corresponding Linux kernel source for the
**v267** production image, as required by GPLv2 section 3.

## Kernel

- **Version**: Linux 5.10.226 (arm64)
- **Build config**: `.config` (included at repo root) — this is the definitive build
  configuration. The checked-in `arch/arm64/configs/rg52mini_defconfig` is the
  base defconfig; the `.config` includes additional KernelSU and SELinux options
  not in the defconfig.
- **Platform**: Rockchip RK3562 (`CONFIG_ARCH_ROCKCHIP=y`)

## Base

[github.com/bmdhacks/kernel_rk3562](https://github.com/bmdhacks/kernel_rk3562)

This repository was imported as a monolithic tree from the bmdhacks RK3562 kernel
base. It is the complete corresponding source as built, not a patch series against
upstream Linux history.

## Modifications (vs. the RK3562 base)

| File | Change |
|------|--------|
| `drivers/input/joystick/rk3562-joystick.c` | Virtual `rk3562-mouse` input device, L3+R3 chord toggle, R3-hold scroll, hat-axis D-pad navigation, `ABS_GAS`/`ABS_BRAKE` trigger remap |
| `arch/arm64/configs/rg52mini_defconfig` | `CONFIG_KSU=y`, `CONFIG_KPROBES=y`, SELinux dev/stats |
| `drivers/Kconfig` | KernelSU Kconfig wiring |
| `drivers/Makefile` | KernelSU driver dir inclusion |
| `arch/arm64/boot/dts/rockchip/rk3562-darkos.dtsi` | SDIO `sdr104→sdr50`, `sai1` disable |
| `KernelSU-Next/` | Bundled KernelSU-Next kernel module (upstream: [KernelSU-Next/KernelSU-Next](https://github.com/KernelSU-Next/KernelSU-Next), tag `v3.2.0`) |

## KernelSU-Next

KernelSU-Next v3.2.0 is bundled at `KernelSU-Next/`. It provides:

- Generic kernel-based root (su) for Android
- `avc_spoof` — SELinux AVC log spoofing (enabled by default in `kernel/extras.c`).
  **Load-bearing**: this supplies the permissive SELinux behaviour microG's
  signature spoofing depends on at the kernel level. Do not remove or disable it
  without replacing the mechanism.

  Upstream commit: `81dc3fa9 kernel: extras: avc log spoofing`
  (stock upstream feature — not a local hack).

KernelSU-Next is licensed under **GPL-3.0** (see `KernelSU-Next/LICENSE`).

## Security notes

- **SELinux is permissive**: `enforcing_enabled()` returns `false` in this build.
  This is intentional — the device ships KernelSU-Next's `avc_spoof` (enabled)
  and runs a `userdebug` build. Both weaken the Android security model.
- **KernelSU root**: the kernel includes the KernelSU-Next driver (`CONFIG_KSU=y`).
  Root access is granted to allow-listed apps only and is managed by the
  KernelSU-Next manager on-device.

## Building

1. Set up an arm64 cross-compiler (the build used `aarch64-linux-gnu-gcc` 16.1.0).
2. Use the included `.config`:
   ```sh
   cp .config arch/arm64/configs/rg52mini_defconfig    # optional, for record
   make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- olddefconfig
   make ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- -j$(nproc) Image
   ```
3. The resulting `arch/arm64/boot/Image` is the kernel binary.

An out-of-tree NDK build is also supported following the standard Android kernel
build flow (`build/build.sh` or equivalent).

## Licenses

- **Linux kernel**: GPL-2.0 (`COPYING`, `LICENSES/preferred/GPL-2.0`)
- **KernelSU-Next**: GPL-3.0 (`KernelSU-Next/LICENSE`)
- **Additional licenses**: see `LICENSES/` for SPDX identifiers used throughout the tree.

## Credits

- [LineageOS](https://lineageos.org/)
- [bmdhacks/kernel_rk3562](https://github.com/bmdhacks/kernel_rk3562) — RK3562 kernel base
- [KernelSU-Next](https://github.com/KernelSU-Next/KernelSU-Next) — kernel root + SELinux support
- [microG](https://microg.org/) — signature spoofing consumer
