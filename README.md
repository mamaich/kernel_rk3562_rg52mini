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

## Modifications in this fork

Everything below is confined to the board DTS and the AIC8800 driver; no other
kernel source is touched. Verified on hardware (board revision B, `hw=14`).

### Device tree — `arch/arm64/boot/dts/rockchip/rk3562-rg52mini.dts`

| Change | Why |
|--------|-----|
| HUSB311 Type-C controller on `i2c2` (addr `0x4e`), `usb-role-switch` on `usbdrd_dwc3` instead of the phy `extcon` | The USB-C receptacle has **no ID pin** — the data role comes from the CC lines. Without the controller dwc3 waits on an extcon signal that never arrives and the port stays a peripheral forever: no root hub, no OTG at all. `CONFIG_TYPEC_HUSB311` was already enabled, only the node was missing. |
| `spk-mute-delay-ms = <100>` on `rk817_codec` | The codec driver gates the external amplifier from `rk817_digital_mute_dac()`. Without a delay the GPIO switches right up against the DAC transition, and every playback start/stop is an audible click. `hp-mute-delay-ms` already existed for the headphone path; the speaker path had been overlooked. |
| `snps,loa-filter-en-quirk` on `usbdrd_dwc3` | Poor cables and ESD can fake a USB2 babble condition in the idle window between high-speed EOF2 and the next microframe SOF; xHCI then disables the root hub port outright and the device drops off with `usb usb1-port1: disabled by hub (EMI?), re-enabling...`. `GUCTL1.LOA_FILTER_EN` makes the controller require three consecutive babble detections before killing the port. Pairs with the dwc3 driver change below. |
| `spk-ctl-gpios` moved from `rk817_sound` to `rk817_codec` | Recovered from the shipped DTB. The amplifier has to be gated by the codec driver, which knows when the DAC mutes. |
| `vfront-porch` 20 → 31 | Recovered from the shipped DTB. |

The last two were already present in the binary DTB of the shipped image but
missing from the published sources; they are restored here so a build from this
tree matches the shipped device tree.

### AIC8800 Wi-Fi/Bluetooth driver

| Change | Why |
|--------|-----|
| `CONFIG_SDIO_BT=y` in both `aic8800_bsp` and `aic8800_fdrv` Makefiles | Bluetooth over SDIO is off by default, so the combo chip only ever did Wi-Fi. |
| `CONFIG_BLUEDROID 0` in `aic_btsdio.h` — the BlueZ path, not the Android one | The Android branch declares its own copies of BlueZ types and does not compile against a kernel with `CONFIG_BT=y` (`redefinition of 'struct bt_skb_cb'`, `redeclaration of 'HCI_UP'`). The BlueZ path registers a normal `hci0`; the driver then loads the combo firmware `fmacfwbt_8800d80_h_u02.bin` by itself. |
| `hci_dev_get` renamed to `btsdio_hci_dev_get` in `btsdio.c` | Clashes with the in-kernel symbol of the same name once `CONFIG_BT=y`. |
| Debug output off in `aic_btsdio.h` (`AICBT_DBG_FLAG 0`, `AICBT_INFO` → `no_printk`) | Per-packet HCI tracing floods the log. |
| `aicwf_dbg_level` / `aicwf_dbg_level_bsp` default to `LOGERROR` | The stock default is `LOGERROR\|LOGINFO\|LOGDEBUG\|LOGTRACE\|LOGFW`, which writes to the kernel ring buffer every three seconds and pushes everything else out of it. The module parameter still allows raising it at runtime. |

Note that `CONFIG_SDIO_BT=y` never compiled in the published tree — an unused
`bt_char_dev_registered` tripped `-Werror=unused-variable`. With the BlueZ path
that file is not built, so no change was needed for it.

### USB dwc3 driver

| Change | Why |
|--------|-----|
| LOA babble filter quirk (`drivers/usb/dwc3/core.{c,h}`) | Backport of `usb: dwc3: core: Add LOA babble filter quirk on Rockchip` by William Wu (rockchip-linux/kernel `develop-5.10`, commit `40bd356d9e23`), which landed after the snapshot this tree is based on. Adds `snps,loa-filter-en-quirk` parsing and sets `GUCTL1.LOA_FILTER_EN`. Without it the device tree property above does nothing. |

The quirk was picked up while diffing this tree against the current Rockchip
`develop-5.10` branch. It is the only change in that branch, across display,
Mali, audio, mmc and Wi-Fi, that applies to this board — everything else
targets other SoCs or hardware this device does not have.

### Kernel configuration

| Change | Why |
|--------|-----|
| `CONFIG_LOCALVERSION="-rg52mini"`, `CONFIG_LOCALVERSION_AUTO` off | Gives this fork a stable, identifiable version string (`5.10.226-rg52mini`) without tying it to the git hash. `LOCALVERSION_AUTO` is a trap in this tree: build artifacts are committed and have no `.gitignore`, so the working tree is dirty after any build and `vermagic` would pick up a `-dirty` suffix that changes from build to build. |
| `CONFIG_ZRAM_WRITEBACK=y` | Lets zram push idle and incompressible pages out to a backing device. Also what the stock `/vendor/etc/fstab_ext*.cfg` templates need — `zram_backingdev_size` there is a no-op without it. |
| `arch/arm64/configs/rg52mini_defconfig` regenerated | The old one did not build: it was missing `CONFIG_AUDIT=y`, which `SECURITY_SELINUX` depends on, so Kconfig silently dropped SELinux and the build then died in KernelSU (`CONFIG_SECURITY_SELINUX_SID2STR_CACHE_SIZE is not defined`). The new one is `make savedefconfig` output from the working configuration and reproduces `.config` exactly. |

### Kernel version string

Changing `CONFIG_LOCALVERSION` changes `vermagic`, so **every** module has to be
rebuilt and shipped together with the Image — `aic8800_bsp`, `aic8800_fdrv` and
`rk915`. The last one matters on board revision A, where the RK915 is the actual
Wi-Fi chip; a stale module there means no Wi-Fi at all.

Note the tree also carries an empty `.scmversion`, which short-circuits
`scripts/setlocalversion` before it ever reaches git. It is a fragile safeguard —
it disappears on a clean checkout of some trees — so the config does not rely
on it.

### Firmware

Bluetooth needs the combo firmware blob `fmacfwbt_8800d80_h_u02.bin` alongside
the Wi-Fi ones in `CONFIG_AIC_FW_PATH`. The driver selects it on its own when
`CONFIG_SDIO_BT=y`; it is not part of this repository.

### Build

Built and tested with **Arm GNU Toolchain 11.3** rather than the 16.1.0 used for
the shipped image. Module symbol CRCs still match, since `genksyms` hashes
preprocessed declarations and not compiler output.

    make ARCH=arm64 CROSS_COMPILE=aarch64-none-linux-gnu- olddefconfig
    make ARCH=arm64 CROSS_COMPILE=aarch64-none-linux-gnu- -j$(nproc) Image modules

Clear `CONFIG_LOCALVERSION_AUTO` first if the tree is a git clone, otherwise
`vermagic` gains a `-g<hash>` suffix and the prebuilt modules stop loading.

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
