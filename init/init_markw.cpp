/*
   Copyright (c) 2016, The CyanogenMod Project
   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions are
   met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above
      copyright notice, this list of conditions and the following
      disclaimer in the documentation and/or other materials provided
      with the distribution.
    * Neither the name of The Linux Foundation nor the names of its
      contributors may be used to endorse or promote products derived
      from this software without specific prior written permission.
   THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
   WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
   MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
   ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
   BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
   CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
   SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
   BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
   WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
   OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
   IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <fcntl.h>
#include <stdlib.h>
#include <sys/sysinfo.h>

#include "vendor_init.h"
#include "property_service.h"
#include "log.h"
#include "util.h"

char const *heapstartsize;
char const *heapgrowthlimit;
char const *heapsize;
char const *heapminfree;
char const *heapmaxfree;
char const *texture_cache_size;
char const *layer_cache_size;
char const *shape_cache_size;
char const *gradient_cache_size;
char const *drop_shadow_cache_size;
char const *small_cache_width;
char const *small_cache_height;
char const *large_cache_width;
char const *large_cache_height;

using android::init::property_set;

static void init_alarm_boot_properties()
{
    int boot_reason;
    FILE *fp;

    fp = fopen("/proc/sys/kernel/boot_reason", "r");
    fscanf(fp, "%d", &boot_reason);
    fclose(fp);

    /*
     * Setup ro.alarm_boot value to true when it is RTC triggered boot up
     * For existing PMIC chips, the following mapping applies
     * for the value of boot_reason:
     *
     * 0 -> unknown
     * 1 -> hard reset
     * 2 -> sudden momentary power loss (SMPL)
     * 3 -> real time clock (RTC)
     * 4 -> DC charger inserted
     * 5 -> USB charger inserted
     * 6 -> PON1 pin toggled (for secondary PMICs)
     * 7 -> CBLPWR_N pin toggled (for external power supply)
     * 8 -> KPDPWR_N pin toggled (power key pressed)
     */
     if (boot_reason == 3) {
        android::init::property_set("ro.alarm_boot", "true");
     } else {
        android::init::property_set("ro.alarm_boot", "false");
     }
}

void check_device()
{
    struct sysinfo sys;

    sysinfo(&sys);

    if (sys.totalram > 3072ull * 1024 * 1024) {
        // from - phone-xxhdpi-4096-dalvik-heap.mk
        heapstartsize = "16m";
        heapgrowthlimit = "256m";
        heapsize = "512m";
        heapminfree = "4m";
        heapmaxfree = "8m";
        texture_cache_size="88";
        layer_cache_size="58";
        shape_cache_size="4";
        gradient_cache_size="2";
        drop_shadow_cache_size="8";
        small_cache_width="2048";
        small_cache_height="2048";
        large_cache_width="4096";
        large_cache_height = "2048";
    } else if (sys.totalram > 2048ull * 1024 * 1024) {
        // from - phone-xxhdpi-3072-dalvik-heap.mk
        heapstartsize = "8m";
        heapgrowthlimit = "288m";
        heapsize = "768m";
        heapminfree = "512k";
        heapmaxfree = "8m";
        texture_cache_size="88";
        layer_cache_size="58";
        shape_cache_size="4";
        gradient_cache_size="2";
        drop_shadow_cache_size="8";
        small_cache_width="2048";
        small_cache_height="2048";
        large_cache_width="4096";
        large_cache_height = "4096";
    } else {
        // from - phone-xxhdpi-2048-dalvik-heap.mk
        heapstartsize = "16m";
        heapgrowthlimit = "192m";
        heapsize = "512m";
        heapminfree = "2m";
        heapmaxfree = "8m";
        texture_cache_size="72";
        layer_cache_size="48";
        gradient_cache_size="1";
        drop_shadow_cache_size="6";
        small_cache_width="1024";
        small_cache_height="1024";
        large_cache_width="2048";
        large_cache_height = "1024";
   }
}

void vendor_load_properties()
{
    init_alarm_boot_properties();
    check_device();

    android::init::property_set("dalvik.vm.heapstartsize", heapstartsize);
    android::init::property_set("dalvik.vm.heapgrowthlimit", heapgrowthlimit);
    android::init::property_set("dalvik.vm.heapsize", heapsize);
    android::init::property_set("dalvik.vm.heaptargetutilization", "0.75");
    android::init::property_set("dalvik.vm.heapminfree", heapminfree);
    android::init::property_set("dalvik.vm.heapmaxfree", heapmaxfree);

    android::init::property_set("ro.hwui.texture_cache_size", texture_cache_size);
    android::init::property_set("ro.hwui.layer_cache_size", layer_cache_size);
    android::init::property_set("ro.hwui.r_buffer_cache_size", "8");
    android::init::property_set("ro.hwui.path_cache_size", "32");
    android::init::property_set("ro.hwui.gradient_cache_size", gradient_cache_size);
    android::init::property_set("ro.hwui.drop_shadow_cache_size", drop_shadow_cache_size);
    android::init::property_set("ro.hwui.texture_cache_flushrate", "0.4");
    android::init::property_set("ro.hwui.text_small_cache_width", small_cache_width);
    android::init::property_set("ro.hwui.text_small_cache_height", small_cache_height);
    android::init::property_set("ro.hwui.text_large_cache_width", large_cache_width);
    android::init::property_set("ro.hwui.text_large_cache_height", large_cache_height);
}
