#!/usr/bin/env -S PYTHONPATH=../../../tools/extract-utils python3
#
# SPDX-FileCopyrightText: 2026 The LineageOS Project
# SPDX-License-Identifier: Apache-2.0

import struct
from pathlib import Path

from extract_utils.fixups_blob import blob_fixup
from extract_utils.main import ExtractUtils, ExtractUtilsModule
LIBSTDCXX_FIXUP = (
    'replace DT_NEEDED libstdc++.so with source-built libstdc++_vendor.so'
)
LIBSTDCXX_FIXUP_PATHS = {
    'vendor/lib/libgoog_eis_armeabi-v7a.so',
    'vendor/lib/libgoog_rownr.so',
    'vendor/lib/libmmcamera_faceproc.so',
}
FACEPROC_SONAME_FIXUP_PATH = 'vendor/lib/libmmcamera_faceproc.so'
FACEPROC_SONAME_FIXUP = (
    'set SONAME to installed libmmcamera_faceproc.so; '
    'no DT_NEEDED consumers of old name'
)
QDUTILS_FIXUP = (
    'remove unused DT_NEEDED libqdutils.so; zero imported-symbol overlap'
)
QDUTILS_FIXUP_PATHS = {
    'vendor/lib/libmm-qdcm.so',
    'vendor/lib64/libmm-qdcm.so',
}
ART_COMPILER_FIXUP = (
    'remove unused DT_NEEDED libart-compiler.so and libart.so; '
    'zero imported-symbol overlap'
)
ART_COMPILER_FIXUP_PATHS = {
    'vendor/lib/lib-imsrcscmclient.so',
    'vendor/lib64/lib-imsrcscmclient.so',
}
LEGACY_LIBLOG_FIXUP_PATHS = {
    'vendor/bin/cnd',
    'vendor/bin/imsdatadaemon',
    'vendor/bin/imsqmidaemon',
    'vendor/bin/loc_launcher',
    'vendor/bin/port-bridge',
    'vendor/lib/lib-imsSDP.so',
    'vendor/lib/lib-imsdpl.so',
    'vendor/lib/lib-imss.so',
    'vendor/lib/lib-rtpdaemoninterface.so',
    'vendor/lib/libQSEEComAPI.so',
    'vendor/lib/libcne.so',
    'vendor/lib/libcneapiclient.so',
    'vendor/lib/libmmcamera2_frame_algorithm.so',
    'vendor/lib/libmmcamera2_is.so',
    'vendor/lib/libmmcamera2_q3a_core.so',
    'vendor/lib/libmmcamera2_stats_algorithm.so',
    'vendor/lib/libmmcamera_cac2_lib.so',
    'vendor/lib/libmmcamera_pdaf.so',
    'vendor/lib/libmmcamera_pdafcamif.so',
    'vendor/lib/libmmcamera_tintless_bg_pca_algo.so',
    'vendor/lib64/lib-imsSDP.so',
    'vendor/lib64/lib-imsdpl.so',
    'vendor/lib64/lib-imss.so',
    'vendor/lib64/lib-rtpdaemoninterface.so',
    'vendor/lib64/libQSEEComAPI.so',
    'vendor/lib64/lib_fpc_tac_shared.so',
    'vendor/lib64/libcne.so',
    'vendor/lib64/libcneapiclient.so',
    'vendor/lib64/libizat_core.so',
    'vendor/lib64/liblbs_core.so',
    'vendor/lib64/liblowi_client.so',
    'vendor/lib64/liblowi_wifihal.so',
    'vendor/lib64/libmmcamera2_q3a_core.so',
    'vendor/lib64/libmmcamera2_stats_algorithm.so',
    'vendor/lib64/libquipc_os_api.so',
}
LEGACY_LIBLOG_FIXUP = (
    'add direct liblog DT_NEEDED for legacy Android log imports'
)
SCHED_POLICY_FIXUP_PATHS = {
    'vendor/lib/libvoice-svc.so',
    'vendor/lib64/libvoice-svc.so',
}
SCHED_POLICY_FIXUP = (
    'add direct libprocessgroup DT_NEEDED for set_sched_policy'
)
CUTILS_STRING_SHIM_FIXUP_PATHS = {
    'vendor/bin/ATFWD-daemon',
    'vendor/lib/libcne.so',
    'vendor/lib64/libcne.so',
}
CUTILS_STRING_SHIM_FIXUP = (
    'add source-built libcutils_shim DT_NEEDED for legacy UTF conversion'
)
POWER_MANAGER_SHIM_FIXUP_PATHS = {
    'vendor/lib/libmm-abl.so',
    'vendor/lib64/libmm-abl.so',
}
POWER_MANAGER_SHIM_FIXUP = (
    'add forwarding power-manager namespace shim for legacy asInterface'
)
SURFACE_SHIM_FIXUP_PATHS = {
    'vendor/lib/libimsmedia_jni.so',
    'vendor/lib64/libimsmedia_jni.so',
}
SURFACE_SHIM_FIXUP = (
    'add forwarding Surface constructor shim with null control handle'
)
RS_FLOOR_SHIM_FIXUP_PATH = 'vendor/lib/librs.rnr_process.so'
RS_FLOOR_SHIM_FIXUP = (
    'add forwarding RenderScript SC_floorf shim'
)
RIL_AUDIO_CALLBACK_SHIM_FIXUP_PATH = 'vendor/lib64/libril-qc-qmi-1.so'
RIL_DSI_COMPAT_FIXUP_PATH = 'vendor/lib64/libril-qc-qmi-1.so'
RIL_AUDIO_CALLBACK_SHIM_FIXUP = (
    'forward AudioSystem setErrorCallback to addErrorCallback'
)
RIL_DSI_COMPAT_FIXUP = (
    'defer address-backed completion outside the stock DSI callback lock when '
    'QCRIL receives WDS_CONNECTED instead of the older NET_IS_CONN event'
)
PERIPHERAL_REFBASE_SHIM_FIXUP_PATH = 'vendor/bin/pm-service'
PERIPHERAL_REFBASE_SHIM_FIXUP = (
    'load the scoped RefBase stack-object compatibility library directly; '
    'Android init does not preserve LD_PRELOAD for this vendor service'
)
SENSOR_EVENTQUEUE_SHIM_FIXUP_PATHS = {
    'vendor/bin/slim_daemon',
    'vendor/lib/liboemcamera.so',
}
SENSOR_EVENTQUEUE_SHIM_FIXUP = (
    'forward legacy SensorManager event queue with empty attribution tag'
)
CAMERA_SENSOR_THREAD_FIXUP_PATH = 'vendor/lib/liboemcamera.so'
CAMERA_SENSOR_THREAD_FIXUP = (
    'keep the legacy camera sensor polling usable when Android 15 declines '
    'its optional SCHED_FIFO promotion or BitTube event queue'
)
CAMERA_SENSOR_NDK_BRIDGE_FIXUP_PATH = (
    'vendor/lib/libmmcamera2_stats_modules.so'
)
CAMERA_SENSOR_NDK_BRIDGE_FIXUP = (
    'route legacy camera NDK sensor calls through libsensorndkbridge'
)
CAMERA_AF_LIFETIME_FIXUP = (
    'cache AF sync_flag before asynchronous queue ownership transfer'
)
WIDEVINE_PROTOBUF_FIXUP_PATHS = {
    'vendor/lib/mediadrm/libwvdrmengine.so',
    'vendor/lib64/mediadrm/libwvdrmengine.so',
}
WIDEVINE_PROTOBUF_FIXUP = (
    'replace protobuf lite with source-built AOSP v28 compatibility library'
)
ISP_MUTEX_FIXUP_PATH = 'vendor/lib/libmmcamera2_isp_modules.so'
ISP_MUTEX_FIXUP = (
    'move CBNZ before mutex destruction for Android P FORTIFY; '
    'PixelBoot provenance 1b95fec2e5f4e5c2432e5885d5ef705e82ecb245'
)
THERMAL_CONFIG_PATH_FIXUP = (
    'read the thermal policy from /vendor/etc; Android 15 neverallows vendor '
    'daemons from reading ordinary /system files'
)
Q3A64_AUDIT_ONLY_PATH = 'vendor/lib64/libmmcamera2_q3a_core.so'
Q3A64_AUDIT_ONLY_PROVENANCE = (
    'audit-only: stock has no 64-bit libmmcamera2_is provider; '
    'runtime camera daemon is 32-bit; omit from product'
)
LIBC_PRIVATE_VERSION_FIXUP_PATHS = {
    'vendor/lib/hw/gatekeeper.msm8994.so',
    'vendor/lib/lib-dplmedia.so',
    'vendor/lib/lib-ims-rcscmjni.so',
    'vendor/lib/lib-imsSDP.so',
    'vendor/lib/lib-imsdpl.so',
    'vendor/lib/lib-imsqimf.so',
    'vendor/lib/lib-imsrcs.so',
    'vendor/lib/lib-imsrcscm.so',
    'vendor/lib/lib-imsrcscmclient.so',
    'vendor/lib/lib-imsrcscmservice.so',
    'vendor/lib/lib-imss.so',
    'vendor/lib/lib-imsxml.so',
    'vendor/lib/lib-rcsimssjni.so',
    'vendor/lib/lib-rcsjni.so',
    'vendor/lib/lib-rtpcommon.so',
    'vendor/lib/lib-rtpcore.so',
    'vendor/lib/lib-rtpdaemoninterface.so',
    'vendor/lib/lib-rtpsl.so',
    'vendor/lib/libQSEEComAPI.so',
    'vendor/lib/libadm.so',
    'vendor/lib/libadpcmdec.so',
    'vendor/lib/libadsprpc.so',
    'vendor/lib/libcneapiclient.so',
    'vendor/lib/libconfigdb.so',
    'vendor/lib/libdrmfs.so',
    'vendor/lib/libdrmtime.so',
    'vendor/lib/libdsi_netctrl.so',
    'vendor/lib/libdsutils.so',
    'vendor/lib/liblistensoundmodel2.so',
    'vendor/lib/libmdsprpc.so',
    'vendor/lib/libmm-abl.so',
    'vendor/lib/libmm-disp-apis.so',
    'vendor/lib/libmm-qdcm.so',
    'vendor/lib/libmmcamera2_frame_algorithm.so',
    'vendor/lib/libmmcamera2_is.so',
    'vendor/lib/libmmcamera2_stats_algorithm.so',
    'vendor/lib/libmmcamera_pdaf.so',
    'vendor/lib/libmmcamera_pdafcamif.so',
    'vendor/lib/libmmcamera_tintless_algo.so',
    'vendor/lib/libmmcamera_tintless_bg_pca_algo.so',
    'vendor/lib/libnetmgr.so',
    'vendor/lib/libqdi.so',
    'vendor/lib/librpmb.so',
    'vendor/lib/libscale.so',
    'vendor/lib/libssd.so',
    'vendor/lib/libtzdrmgenprov.so',
}
LIBC_PRIVATE_VERSION_FIXUP = (
    'retarget audited legacy ARM EABI imports from LIBC_PRIVATE to the '
    'ABI-identical LIBC_N aliases exported by current Bionic'
)


def _elf_hash(value: bytes):
    result = 0
    for byte in value:
        result = (result << 4) + byte
        high = result & 0xF0000000
        if high:
            result ^= high >> 24
        result &= ~high
    return result


def retarget_libc_private_version(
    _ctx,
    _file,
    file_path,
    *_args,
    **_kwargs,
):
    data = bytearray(Path(file_path).read_bytes())
    if data[:6] != b'\x7fELF\x01\x01':
        raise ValueError(f'{file_path}: expected a little-endian ELF32 file')

    section_offset = struct.unpack_from('<I', data, 32)[0]
    section_entry_size = struct.unpack_from('<H', data, 46)[0]
    section_count = struct.unpack_from('<H', data, 48)[0]
    if section_entry_size < 40:
        raise ValueError(f'{file_path}: invalid ELF32 section entry size')

    sections = [
        struct.unpack_from(
            '<IIIIIIIIII',
            data,
            section_offset + index * section_entry_size,
        )
        for index in range(section_count)
    ]
    verneed_sections = [section for section in sections if section[1] == 0x6FFFFFFE]
    if len(verneed_sections) != 1:
        raise ValueError(f'{file_path}: expected exactly one GNU verneed section')

    verneed = verneed_sections[0]
    dynstr_index = verneed[6]
    if dynstr_index >= len(sections) or sections[dynstr_index][1] != 3:
        raise ValueError(f'{file_path}: GNU verneed does not link to a string table')
    dynstr = sections[dynstr_index]
    dynstr_offset, dynstr_size = dynstr[4], dynstr[5]

    old_name = b'LIBC_PRIVATE'
    new_name = b'LIBC_N'
    old_hash = _elf_hash(old_name)
    new_hash = _elf_hash(new_name)
    patched = 0
    relative = 0
    while relative < verneed[5]:
        current = verneed[4] + relative
        _, aux_count, _, aux_relative, next_relative = struct.unpack_from(
            '<HHIII', data, current
        )
        aux = current + aux_relative
        for _ in range(aux_count):
            version_hash, _, _, name_relative, next_aux = struct.unpack_from(
                '<IHHII', data, aux
            )
            name_start = dynstr_offset + name_relative
            name_end = data.index(0, name_start, dynstr_offset + dynstr_size)
            if bytes(data[name_start:name_end]) == old_name:
                if version_hash != old_hash:
                    raise ValueError(f'{file_path}: LIBC_PRIVATE hash mismatch')
                padded_name = new_name + bytes(len(old_name) - len(new_name))
                data[name_start:name_end] = padded_name
                struct.pack_into('<I', data, aux, new_hash)
                patched += 1
            if next_aux == 0:
                break
            aux += next_aux
        if next_relative == 0:
            break
        relative += next_relative

    if patched != 1:
        raise ValueError(
            f'{file_path}: expected one LIBC_PRIVATE version need, found {patched}'
        )
    Path(file_path).write_bytes(data)


def _libc_private_fixup():
    return blob_fixup().call(
        retarget_libc_private_version,
        need_tmp_dir=False,
    )


def _legacy_liblog_fixup(path):
    fixup = (
        _libc_private_fixup()
        if path in LIBC_PRIVATE_VERSION_FIXUP_PATHS
        else blob_fixup()
    )
    fixup.add_needed('liblog.so')
    if path in CUTILS_STRING_SHIM_FIXUP_PATHS:
        fixup.add_needed('libcutils_shim.so')
    return fixup


blob_fixups = {
    'vendor/bin/thermal-engine': blob_fixup().binary_regex_replace(
        b'/system/etc/thermal-engine.conf',
        b'/vendor/etc/thermal-engine.conf',
    ),
    tuple(
        sorted(LIBSTDCXX_FIXUP_PATHS - {FACEPROC_SONAME_FIXUP_PATH})
    ): blob_fixup().replace_needed(
        'libstdc++.so',
        'libstdc++_vendor.so',
    ),
    FACEPROC_SONAME_FIXUP_PATH: (
        blob_fixup()
        .replace_needed('libstdc++.so', 'libstdc++_vendor.so')
        .fix_soname()
    ),
    'vendor/lib64/libmm-qdcm.so': blob_fixup().remove_needed('libqdutils.so'),
    'vendor/lib/libmm-qdcm.so': (
        blob_fixup().remove_needed('libqdutils.so').call(
            retarget_libc_private_version,
            need_tmp_dir=False,
        )
    ),
    'vendor/lib64/lib-imsrcscmclient.so': (
        blob_fixup()
        .remove_needed('libart-compiler.so')
        .remove_needed('libart.so')
    ),
    'vendor/lib/lib-imsrcscmclient.so': (
        blob_fixup()
        .remove_needed('libart-compiler.so')
        .remove_needed('libart.so')
        .call(retarget_libc_private_version, need_tmp_dir=False)
    ),
    **{
        path: _legacy_liblog_fixup(path)
        for path in sorted(LEGACY_LIBLOG_FIXUP_PATHS)
    },
    tuple(
        sorted(CUTILS_STRING_SHIM_FIXUP_PATHS - LEGACY_LIBLOG_FIXUP_PATHS)
    ): blob_fixup().add_needed('libcutils_shim.so'),
    'vendor/lib64/libmm-abl.so': (
        blob_fixup().add_needed('libpowermanager_legacy_shim.so')
    ),
    'vendor/lib/libmm-abl.so': (
        _libc_private_fixup().add_needed('libpowermanager_legacy_shim.so')
    ),
    tuple(sorted(SURFACE_SHIM_FIXUP_PATHS)): (
        blob_fixup().add_needed('libsurface_legacy_shim.so')
    ),
    RS_FLOOR_SHIM_FIXUP_PATH: (
        blob_fixup().add_needed('librs_legacy_math_shim.so')
    ),
    RIL_AUDIO_CALLBACK_SHIM_FIXUP_PATH: (
        blob_fixup()
        .add_needed('libaudioclient_legacy_shim.so')
        .add_needed('libangler_dsi_netctrl_compat.so')
    ),
    PERIPHERAL_REFBASE_SHIM_FIXUP_PATH: (
        blob_fixup().add_needed('libangler_peripheral_refbase_compat.so')
    ),
    tuple(
        sorted(
            SENSOR_EVENTQUEUE_SHIM_FIXUP_PATHS
            - {CAMERA_SENSOR_THREAD_FIXUP_PATH}
        )
    ): (
        blob_fixup().add_needed('libsensor_eventqueue_legacy_shim.so')
    ),
    CAMERA_SENSOR_THREAD_FIXUP_PATH: (
        blob_fixup()
        .add_needed('libsensor_eventqueue_legacy_shim.so')
        .sig_replace(
            '6F F0 25 05 13 E0',
            '00 25 00 BF 13 E0',
        )
        .sig_replace(
            (
                '30 68 14 F0 03 FA 04 46 68 42 14 F0 2F FA 1B 49'
            ),
            (
                '00 25 0E E0 14 F0 03 FA 04 46 68 42 14 F0 '
                '2F FA 1B 49'
            ),
        )
    ),
    CAMERA_SENSOR_NDK_BRIDGE_FIXUP_PATH: (
        blob_fixup()
        .replace_needed('libandroid.so', 'libsensorndkbridge.so')
        .sig_replace(
            (
                '11 F0 93 F8 01 28 09 D1 65 68 01 2D 08 D1'
            ),
            (
                '11 F0 4B FB 01 28 09 D1 3D 46 01 2D 08 D1'
            ),
        )
        .sig_replace(
            (
                'FF FF 78 47 C0 46 00 C0 9F E5 0F F0 8C E0 48 5F '
                'FD FF 00 00 00 00 00 00'
            ),
            (
                'FF FF 78 47 C0 46 00 C0 9F E5 0F F0 8C E0 48 5F '
                'FD FF 4F 68 F7 F7 3D BF'
            ),
        )
    ),
    tuple(sorted(WIDEVINE_PROTOBUF_FIXUP_PATHS)): (
        blob_fixup().replace_needed(
            'libprotobuf-cpp-lite.so',
            'libprotobuf-cpp-lite-v28.so',
        )
    ),
    tuple(sorted(SCHED_POLICY_FIXUP_PATHS)): (
        blob_fixup().add_needed('libprocessgroup.so')
    ),
    ISP_MUTEX_FIXUP_PATH: blob_fixup().sig_replace(
        (
            '06 9A 02 F5 46 3E 0E F5 EA 70 20 F0 5C FC 06 99 '
            '01 F5 46 30 00 F5 EC 70 20 F0 55 FC 06 9B 03 F5 '
            '46 3C 0C F5 EE 70 20 F0 4E FC 06 9E 06 F5 46 32 '
            '02 F5 E8 70 20 F0 4F FC 0C B9'
        ),
        (
            'EC B9 06 9A 02 F5 46 3E 0E F5 EA 70 20 F0 5B FC '
            '06 99 01 F5 46 30 00 F5 EC 70 20 F0 54 FC 06 9B '
            '03 F5 46 3C 0C F5 EE 70 20 F0 4D FC 06 9E 06 F5 '
            '46 32 02 F5 E8 70 20 F0 4E FC'
        ),
    ),
    tuple(
        sorted(
            LIBC_PRIVATE_VERSION_FIXUP_PATHS
            - {
                'vendor/lib/libmm-qdcm.so',
                'vendor/lib/lib-imsrcscmclient.so',
                'vendor/lib/libmm-abl.so',
            }
            - LEGACY_LIBLOG_FIXUP_PATHS
        )
    ): _libc_private_fixup(),
}


# Keep this baseline fixup-free. A blob may be changed only after a recorded
# Android 15 loader/symbol failure identifies one exact compatibility patch.
module = ExtractUtilsModule(
    'angler',
    'huawei',
    blob_fixups=blob_fixups,
    namespace_imports=[
        'hardware/qcom/display',
        'vendor/qcom/opensource/dataservices',
    ],
)
if __name__ == '__main__':
    ExtractUtils.device(module).run()
