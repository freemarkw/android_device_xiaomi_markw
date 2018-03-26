/*
 * Copyright (C) 2008 The Android Open Source Project
 * Copyright (C) 2014 The  Linux Foundation. All rights reserved.
 * Copyright (C) 2015 The CyanogenMod Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


// #define LOG_NDEBUG 0

#include <cutils/log.h>

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <math.h>

#include <sys/types.h>

#include <hardware/lights.h>

/******************************************************************************/

static pthread_once_t g_init = PTHREAD_ONCE_INIT;
static pthread_mutex_t g_lock = PTHREAD_MUTEX_INITIALIZER;
static struct light_state_t g_notification;
static struct light_state_t g_battery;
static struct light_state_t g_attention;

char const*const RED_LED_FILE
        = "/sys/class/leds/red/brightness";

char const*const RED_BLINK_FILE
	= "/sys/class/leds/red/blink";

char const*const GREEN_BLINK_FILE
	= "/sys/class/leds/green/blink";

char const*const BLUE_BLINK_FILE
	= "/sys/class/leds/blue/blink";

char const*const GREEN_LED_FILE
        = "/sys/class/leds/green/brightness";

char const*const BLUE_LED_FILE
        = "/sys/class/leds/blue/brightness";

char const*const LCD_FILE
        = "/sys/class/leds/lcd-backlight/brightness";

char const*const RED_BREATH_FILE
        = "/sys/class/leds/red/led_time";

char const*const GREEN_BREATH_FILE
        = "/sys/class/leds/green/led_time";

char const*const BLUE_BREATH_FILE
        = "/sys/class/leds/blue/led_time";

/**
 * device methods
 */

void init_globals(void)
{
    // init the mutex
    pthread_mutex_init(&g_lock, NULL);
}

static int
write_int(char const* path, int value)
{
    int fd;
    static int already_warned = 0;

    fd = open(path, O_RDWR);
    if (fd >= 0) {
        char buffer[20];
        int bytes = snprintf(buffer, sizeof(buffer), "%d\n", value);
        ssize_t amt = write(fd, buffer, (size_t)bytes);
        close(fd);
        return amt == -1 ? -errno : 0;
    } else {
        if (already_warned == 0) {
            ALOGE("write_int failed to open %s\n", path);
            already_warned = 1;
        }
        return -errno;
    }
}

static int
write_str(char const* path, char *value)
{
    int fd;
    static int already_warned = 0;

    fd = open(path, O_RDWR);
    if (fd >= 0) {
        char buffer[20];
        ssize_t amt = write(fd, value, (size_t)strlen(value));
        close(fd);
        return amt == -1 ? -errno : 0;
    } else {
        if (already_warned == 0) {
            ALOGE("write_str failed to open %s\n", path);
            already_warned = 1;
        }
        return -errno;
    }
}

static int
is_lit(struct light_state_t const* state)
{
    return state->color & 0x00ffffff;
}

static int
rgb_to_brightness(struct light_state_t const* state)
{
    int color = state->color & 0x00ffffff;
    return ((77*((color>>16)&0x00ff))
            + (150*((color>>8)&0x00ff)) + (29*(color&0x00ff))) >> 8;
}

static int
set_light_backlight(struct light_device_t* dev,
        struct light_state_t const* state)
{
    int err = 0;
    int brightness = rgb_to_brightness(state);
    if(!dev) {
        return -1;
    }
    pthread_mutex_lock(&g_lock);
    err = write_int(LCD_FILE, brightness);
    pthread_mutex_unlock(&g_lock);
    return err;
}

static int
get_fade_index(int base, int num, int shift_fade)
{
    if (num == 0) return 0; // we can ignore zeroes
    if ((base % num) != 0) return -1; // when values is not divisible
                                      // we cannot synchronize them
    int division = base / num; 

    int max_fade = 6; // 2^6, can be setted to 7
    for (int i = 0; i <= max_fade; i++) {
        if (division == (int)pow(2, i)) // values need to be divided with 2^x
            return i + shift_fade; // shift_fade increases default fade value 
    }

    return -1;
}

static int
match_color_grid_value(int value)
{   // to do: revert array and reimplement for-cycle for it
    const int grid[] = {16, 32, 64, 128, 256}; // we can add 2 more values, but these are enough
    const float diff_rate_great = 0.5f; // round down (64 + 64*0.5 =round_to=> 64) 
    const float diff_rate_less = 0.25f; // round up (64 - 64*0.25 + 1 =round_to=> 64)
    const int   zero_like = 12; // 16 - 16 * 0.25

    if (zero_like >= value) return 0;

    for (int index = 0; index != sizeof(grid)/sizeof(int); index++) { // to do: optimize
        int diff_great = grid[index] * diff_rate_great;
        int diff_less  = grid[index] * diff_rate_less;

        if (grid[index] + diff_great >= value)
            return grid[index];
    }

    // value is more than maximum
    return grid[sizeof(grid)/sizeof(int) - 1]; // the last element
}

static int
min_not_zero(int* values, int size)
{
    int min = values[0]; // can be 0, careful

    for (int i = 0; i != size; i++) {
    	if (values[i] <= 0) continue;
    	if (min <= 0) min = values[i]; // first not null
        if (values[i] < min) 
            min = values[i];
    }

    return min;
}

static void
adapt_colors_for_blink(int* red, int* green, int* blue)
{
    int part[]  = {(*red), (*green), (*blue)}; // color components
    int round[] = {     0,        0,       0}; // rounded to grid
    int rate[]  = {     0,        0,       0}; // minimum component * rate = true component value 
    const int size = sizeof(part)/sizeof(int); // 3
    int max, min, min_round;
    float max_rate, revert_rate;

    // find max element
    max = part[0];
    for (int i = 0; i != size; i++) 
        if (max < part[i]) max = part[i];

    if (max == 0) return; // 0 0 0

    max_rate    =       256.f / (float) max; // max * max_rate = 256
    revert_rate = (float) max / 256.f;       //(max * max_rate) * revert_rate = max

    for (int i = 0; i != size; i++) {
        part[i] = (int)((float)part[i] * max_rate); // max number will be 256,
                                                    // increase elements proportionally
        round[i] = match_color_grid_value(part[i]); // round off values to the grid

        part[i] = round[i] * revert_rate; // return the original scale of values
    }

    min       = min_not_zero(part,  size);
    min_round = min_not_zero(round, size);

    if (min_round > 0) {

        for (int i = 0; i != size; i++) {
            rate[i] = round[i] / min_round; // calculate rates of components

            part[i] = min * rate[i];        // min * rate = true component value 
        }

    }

    // result output
    *red   = part[0];
    *green = part[1];
    *blue  = part[2];
}

static int
set_speaker_light_locked(struct light_device_t* dev,
        struct light_state_t const* state)
{
    int red, green, blue;
    int max;
    int red_fade, green_fade, blue_fade;
    int blink;
    int onMS, offMS;
    unsigned int colorRGB;
    char breath_pattern_red[16]   = { 0, };
    char breath_pattern_green[16] = { 0, };
    char breath_pattern_blue[16]  = { 0, };
    struct color *nearest = NULL;

    if(!dev) {
        return -1;
    }

    write_int(RED_LED_FILE, 0);
    write_int(GREEN_LED_FILE, 0);
    write_int(BLUE_LED_FILE, 0);

    if (state == NULL) {
        return 0;
    }

    switch (state->flashMode) {
        case LIGHT_FLASH_TIMED:
            onMS = state->flashOnMS;
            offMS = state->flashOffMS;
            break;
        case LIGHT_FLASH_NONE:
        default:
            onMS = 0;
            offMS = 0;
            break;
    }

    colorRGB = state->color;

    ALOGD("set_speaker_light_locked mode %d, colorRGB=%08X, onMS=%d, offMS=%d\n",
            state->flashMode, colorRGB, onMS, offMS);

    red = (colorRGB >> 16) & 0xFF;
    green = (colorRGB >> 8) & 0xFF;
    blue = colorRGB & 0xFF;

    blink = onMS > 0 && offMS > 0;

    if (blink) {

        adapt_colors_for_blink(&red, &green, &blue); 
        
        // In our case, use the settings in the driver led range values
        // to do: refactor intervals
        if (onMS < 1000)
            onMS = 1000;
        else if (onMS > 5000)
            onMS = 5000;

        if (offMS < 1000)
            offMS = 1000;
        else if (offMS > 7000)
            offMS = 7000;
        
        // Indexes [0: 0.13, 1:0.26, 2: 0.52, 3:1.04, 4: 2.08, 5: 4.16, 6: 8.32, 7: 16.64]
        // max fade grid     256     128      64      32       16       8        4   
        // to do: match max fade element to grid
        
        max = (red > green) ? ((red > blue) ? red : blue) : ((green > blue) ? green : blue);
        red_fade   = get_fade_index(max, red,   1); // rise and fall = 0.26
        green_fade = get_fade_index(max, green, 1);
        blue_fade  = get_fade_index(max, blue,  1);

        ALOGD("set_speaker_light_locked r %d, g %d, b %d, rf %d, gf %d, bf %d\n",
            red, green, blue, red_fade, green_fade, blue_fade);

        if (red_fade == -1 || green_fade == -1 || blue_fade == -1) { // to do: remove
            ALOGD("set_speaker_light_locked: LED ERROR\n");

            red   = 127;
            green = 127;
            blue  = 127;

            red_fade = green_fade = blue_fade = 2; // 0.52 => 0.26 for 127 color
        }

        sprintf(breath_pattern_red,   "%d %d %d %d", red_fade,   (int)(onMS/1000), red_fade,   (int)(offMS/1000));
        sprintf(breath_pattern_green, "%d %d %d %d", green_fade, (int)(onMS/1000), green_fade, (int)(offMS/1000));
        sprintf(breath_pattern_blue,  "%d %d %d %d", blue_fade,  (int)(onMS/1000), blue_fade,  (int)(offMS/1000));

    } else {
        blink = 0;
        sprintf(breath_pattern_red,   "2 3 2 4");
        sprintf(breath_pattern_green, "2 3 2 4");
        sprintf(breath_pattern_blue,  "2 3 2 4");
    }

    // Do everything with the lights out, then turn up the brightness
    write_str(RED_BREATH_FILE, breath_pattern_red);
    write_int(RED_BLINK_FILE, (blink && red ? 1 : 0));
    write_str(GREEN_BREATH_FILE, breath_pattern_green);
    write_int(GREEN_BLINK_FILE, (blink && green ? 1 : 0));
    write_str(BLUE_BREATH_FILE, breath_pattern_blue);
    write_int(BLUE_BLINK_FILE, (blink && blue ? 1 : 0));

    write_int(RED_LED_FILE, red);
    write_int(GREEN_LED_FILE, green);
    write_int(BLUE_LED_FILE, blue);

    return 0;
}

static void
handle_speaker_battery_locked(struct light_device_t* dev)
{
    set_speaker_light_locked(dev, NULL);
    if (is_lit(&g_attention)) {
        set_speaker_light_locked(dev, &g_attention);
    } else if (is_lit(&g_notification)) {
        set_speaker_light_locked(dev, &g_notification);
    } else {
        set_speaker_light_locked(dev, &g_battery);
    }
}

static int
set_light_battery(struct light_device_t* dev,
        struct light_state_t const* state)
{
    pthread_mutex_lock(&g_lock);
    g_battery = *state;
    handle_speaker_battery_locked(dev);
    pthread_mutex_unlock(&g_lock);
    return 0;
}

static int
set_light_notifications(struct light_device_t* dev,
        struct light_state_t const* state)
{
    pthread_mutex_lock(&g_lock);
    g_notification = *state;
    handle_speaker_battery_locked(dev);
    pthread_mutex_unlock(&g_lock);
    return 0;
}

static int
set_light_attention(struct light_device_t* dev,
        struct light_state_t const* state)
{
    pthread_mutex_lock(&g_lock);

    g_attention = *state;
    if (state->flashMode == LIGHT_FLASH_HARDWARE) {
        if (g_attention.flashOnMS > 0 && g_attention.flashOffMS == 0) {
            g_attention.flashMode = LIGHT_FLASH_NONE;
        }
    } else if (state->flashMode == LIGHT_FLASH_NONE) {
        g_attention.color = 0;
    }
    handle_speaker_battery_locked(dev);

    pthread_mutex_unlock(&g_lock);

    return 0;
}

/** Close the lights device */
static int
close_lights(struct light_device_t *dev)
{
    if (dev) {
        free(dev);
    }
    return 0;
}


/******************************************************************************/

/**
 * module methods
 */

/** Open a new instance of a lights device using name */
static int open_lights(const struct hw_module_t* module, char const* name,
        struct hw_device_t** device)
{
    int (*set_light)(struct light_device_t* dev,
            struct light_state_t const* state);

    if (0 == strcmp(LIGHT_ID_BACKLIGHT, name))
        set_light = set_light_backlight;
    else if (0 == strcmp(LIGHT_ID_BATTERY, name))
        set_light = set_light_battery;
    else if (0 == strcmp(LIGHT_ID_NOTIFICATIONS, name))
        set_light = set_light_notifications;
    else if (0 == strcmp(LIGHT_ID_ATTENTION, name))
        set_light = set_light_attention;
    else
        return -EINVAL;

    pthread_once(&g_init, init_globals);

    struct light_device_t *dev = malloc(sizeof(struct light_device_t));

    if(!dev)
        return -ENOMEM;

    memset(dev, 0, sizeof(*dev));

    dev->common.tag = HARDWARE_DEVICE_TAG;
    dev->common.version = 0;
    dev->common.module = (struct hw_module_t*)module;
    dev->common.close = (int (*)(struct hw_device_t*))close_lights;
    dev->set_light = set_light;

    *device = (struct hw_device_t*)dev;
    return 0;
}

static struct hw_module_methods_t lights_module_methods = {
    .open =  open_lights,
};

/*
 * The lights Module
 */
struct hw_module_t HAL_MODULE_INFO_SYM = {
    .tag = HARDWARE_MODULE_TAG,
    .version_major = 1,
    .version_minor = 0,
    .id = LIGHTS_HARDWARE_MODULE_ID,
    .name = "Lights Module",
    .author = "The CyanogenMod Project",
    .methods = &lights_module_methods,
};
