<div align="center">

# LineageOS for Google Nexus 6P

Device configuration, proprietary vendor files, kernel sources, IMS service,
and platform patches for bringing **LineageOS 22.2** (Android 15) to the
**Google Nexus 6P** (`angler`).

<p>
  <img alt="LineageOS 22.2" src="https://img.shields.io/badge/LineageOS-22.2-167C80?style=flat-square&logo=lineageos&logoColor=white">
  <img alt="Android 15" src="https://img.shields.io/badge/Android-15-3DDC84?style=flat-square&logo=android&logoColor=white">
  <img alt="Qualcomm Snapdragon 810" src="https://img.shields.io/badge/SoC-Snapdragon%20810-EF2D2D?style=flat-square&logo=qualcomm&logoColor=white">
  <img alt="Linux kernel 3.10.108" src="https://img.shields.io/badge/Kernel-3.10.108-FCC624?style=flat-square&logo=linux&logoColor=black">
</p>

<img src="https://fdn2.gsmarena.com/vv/pics/huawei/huawei-nexus-6p-1.jpg" width="420" alt="Google Nexus 6P">

</div>

## Device overview

The Google Nexus 6P is a 2015 Android smartphone manufactured by Huawei. It
combines a 5.7-inch WQHD AMOLED display with Qualcomm's Snapdragon 810 v2.1
platform, front-facing stereo speakers, a rear fingerprint sensor, USB Type-C,
and a 3450 mAh battery.

## Specifications

| Category | Specification |
|:--|:--|
| Device | Google Nexus 6P |
| Manufacturer | Huawei |
| Models | H1511 / H1512 |
| Codename | `angler` |
| Released | September 2015 |
| SoC | Qualcomm Snapdragon 810 v2.1 / MSM8994, 20 nm |
| CPU | 4× Cortex-A57 at up to 2.0 GHz + 4× Cortex-A53 at up to 1.55 GHz |
| GPU | Qualcomm Adreno 430 |
| Memory | 3 GB LPDDR4 RAM |
| Storage | 32 GB, 64 GB, or 128 GB; no microSD slot |
| Display | 5.7-inch AMOLED, 2560 × 1440, 518 ppi, Gorilla Glass 4 |
| Rear camera | 12.3 MP, f/2.0, 1.55 μm pixels, laser autofocus, dual-LED flash |
| Front camera | 8 MP, f/2.4, 1.4 μm pixels |
| Video capture | Up to 4K UHD at 30 fps |
| Battery | 3450 mAh |
| Wireless | Dual-band 802.11 a/b/g/n/ac Wi-Fi, Bluetooth 4.2, NFC |
| Location | GPS, A-GPS, and GLONASS |
| Ports | USB Type-C and 3.5 mm audio jack |
| Audio | Dual front-facing stereo speakers and three microphones |
| Dimensions | 159.3 × 77.8 × 7.3 mm |
| Weight | 178 g |
| Shipped OS | Android 6.0 Marshmallow |
| Final official firmware | Android 8.1 Oreo, build `OPM7.181205.001` |

## Project details

| Item | Value |
|:--|:--|
| Target platform | LineageOS 22.2 / Android 15 |
| Build target | `lineage_angler-bp1a-userdebug` |
| Kernel | Linux 3.10.108 |
| Architecture | ARM64 |

## Source repositories

| Component | Repository | Branch |
|:--|:--|:--|
| Device configuration | [`android_device_huawei_angler`](https://github.com/LineageOS-Nexus-6P/android_device_huawei_angler/tree/lineage-22.2) | `lineage-22.2` |
| Proprietary vendor files | [`android_vendor_huawei_angler`](https://github.com/LineageOS-Nexus-6P/android_vendor_huawei_angler/tree/lineage-22.2) | `lineage-22.2` |
| Kernel source | [`android_kernel_huawei_angler`](https://github.com/LineageOS-Nexus-6P/android_kernel_huawei_angler/tree/lineage-22.2) | `lineage-22.2` |
| PhhIms for Nexus 6P | [`android_packages_apps_PhhIms`](https://github.com/LineageOS-Nexus-6P/android_packages_apps_PhhIms/tree/lineage-22.2) | `lineage-22.2` |
| Platform patches | [`nexus6p_patches`](https://github.com/LineageOS-Nexus-6P/nexus6p_patches/tree/lineage-22.2) | `lineage-22.2` |

---

> [!NOTE]
> This is an independent, unofficial community project. It is not affiliated
> with or endorsed by LineageOS, Google, Huawei, or Qualcomm.
