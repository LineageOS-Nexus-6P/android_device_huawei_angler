#!/vendor/bin/sh

# Angler's stock OPM7 / LineageOS 17.1 CPU bring-up sequence.  The boot
# command line intentionally starts only the A53 cluster; this script brings
# the A57 cluster online after thermal and BCL hotplug have been quiesced.

write() {
    echo -n "$2" > "$1"
}

copy() {
    cat "$1" > "$2"
}

get_set_for_all() {
    for node in $1; do
        cat "$node"
        write "$node" "$2"
    done
}

# Avoid a thermal/BCL hotplug race while CPU4 creates the second cpufreq
# policy and its interactive governor nodes.
write /sys/module/msm_thermal/core_control/enabled 0
get_set_for_all '/sys/devices/soc.0/qcom,bcl.*/mode' disable
bcl_hotplug_mask="$(get_set_for_all '/sys/devices/soc.0/qcom,bcl.*/hotplug_mask' 0)"
bcl_hotplug_soc_mask="$(get_set_for_all '/sys/devices/soc.0/qcom,bcl.*/hotplug_soc_mask' 0)"
get_set_for_all '/sys/devices/soc.0/qcom,bcl.*/mode' enable

restorecon -R /sys/devices/system/cpu

# Keep at most one A57 available until its frequency policy is configured.
write /sys/devices/system/cpu/cpu5/online 0
write /sys/devices/system/cpu/cpu6/online 0
write /sys/devices/system/cpu/cpu7/online 0
write /sys/devices/system/cpu/cpu4/cpufreq/scaling_max_freq 960000
write /sys/module/msm_performance/parameters/cpu_max_freq \
    '4:960000 5:960000 6:960000 7:960000'

# Cortex-A53 cluster.
write /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor interactive
restorecon -R /sys/devices/system/cpu
write /sys/devices/system/cpu/cpu0/cpufreq/interactive/above_hispeed_delay 19000
write /sys/devices/system/cpu/cpu0/cpufreq/interactive/go_hispeed_load 99
write /sys/devices/system/cpu/cpu0/cpufreq/interactive/timer_rate 10000
write /sys/devices/system/cpu/cpu0/cpufreq/interactive/hispeed_freq 960000
write /sys/devices/system/cpu/cpu0/cpufreq/interactive/io_is_busy 1
write /sys/devices/system/cpu/cpu0/cpufreq/interactive/target_loads \
    '65 460000:75 960000:80'
write /sys/devices/system/cpu/cpu0/cpufreq/interactive/min_sample_time 40000
write /sys/devices/system/cpu/cpu0/cpufreq/interactive/max_freq_hysteresis 80000
write /sys/devices/system/cpu/cpu0/cpufreq/scaling_min_freq 384000

# CPU4 creates the shared Cortex-A57 cpufreq policy.
write /sys/devices/system/cpu/cpu4/online 1
restorecon -R /sys/devices/system/cpu

write /sys/devices/system/cpu/cpu4/cpufreq/scaling_governor interactive
restorecon -R /sys/devices/system/cpu
write /sys/devices/system/cpu/cpu4/cpufreq/interactive/above_hispeed_delay 19000
write /sys/devices/system/cpu/cpu4/cpufreq/interactive/go_hispeed_load 99
write /sys/devices/system/cpu/cpu4/cpufreq/interactive/timer_rate 10000
write /sys/devices/system/cpu/cpu4/cpufreq/interactive/hispeed_freq 1248000
write /sys/devices/system/cpu/cpu4/cpufreq/interactive/io_is_busy 1
write /sys/devices/system/cpu/cpu4/cpufreq/interactive/target_loads \
    '70 960000:80 1248000:85'
write /sys/devices/system/cpu/cpu4/cpufreq/interactive/min_sample_time 40000
write /sys/devices/system/cpu/cpu4/cpufreq/interactive/max_freq_hysteresis 80000
write /sys/devices/system/cpu/cpu4/cpufreq/scaling_min_freq 633600
copy /sys/devices/system/cpu/cpu4/cpufreq/cpuinfo_max_freq \
    /sys/devices/system/cpu/cpu4/cpufreq/scaling_max_freq

# Finish initializing the A57 cluster.  Thermal control may offline cores
# afterwards, but every core has now been booted and can be brought back.
write /sys/devices/system/cpu/cpu5/online 1
write /sys/devices/system/cpu/cpu6/online 1
write /sys/devices/system/cpu/cpu7/online 1
write /sys/module/msm_performance/parameters/cpu_max_freq \
    '4:4294967295 5:4294967295 6:4294967295 7:4294967295'

write /sys/module/cpu_boost/parameters/input_boost_freq '0:1344000'
write /sys/module/cpu_boost/parameters/input_boost_ms 40

write /proc/sys/kernel/sched_migration_fixup 1
write /proc/sys/kernel/sched_upmigrate 95
write /proc/sys/kernel/sched_downmigrate 85
write /proc/sys/kernel/sched_freq_inc_notify 400000
write /proc/sys/kernel/sched_freq_dec_notify 400000
write /proc/sys/kernel/sched_upmigrate_min_nice 9

write /proc/sys/kernel/sched_boost 0

# Restore the stock thermal and battery-current protection after both
# clusters have valid policies.
write /sys/module/msm_thermal/core_control/enabled 1
get_set_for_all '/sys/devices/soc.0/qcom,bcl.*/mode' disable
get_set_for_all '/sys/devices/soc.0/qcom,bcl.*/hotplug_mask' "$bcl_hotplug_mask"
get_set_for_all '/sys/devices/soc.0/qcom,bcl.*/hotplug_soc_mask' \
    "$bcl_hotplug_soc_mask"
get_set_for_all '/sys/devices/soc.0/qcom,bcl.*/mode' enable
