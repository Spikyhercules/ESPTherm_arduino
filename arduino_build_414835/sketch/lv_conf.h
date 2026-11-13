#line 1 "C:\\Projects\\esp-thermal-arduino-ide\\esp_thermal\\lv_conf.h"
/**
 * lv_conf.h — LVGL v8 config for ESP32-S3 + ILI9341 (320×240)
 *
 * PLACEMENT: Copy this file to:
 *   E:\arduino_1_8_19_portable\portable\sketchbook\libraries\lvgl\src\lv_conf.h
 *
 * i.e. inside lvgl\src\ — NOT the lvgl root, NOT the sketch folder.
 *
 * WHY: The sketch folder contains build_opt.h which defines
 * -DLV_CONF_INCLUDE_SIMPLE. Arduino IDE passes that flag to ALL
 * compilation units including the lvgl library itself. With that define
 * active LVGL does #include "lv_conf.h", and since lvgl\src is always
 * on the compiler -I include path, the file is found there.
 *
 * Both this file (in lvgl\src\) AND build_opt.h (in the sketch folder)
 * must be present — either alone is not sufficient.
 */

#if 1 /* Set this to 1 to enable the content below */

#ifndef LV_CONF_H
#define LV_CONF_H

#include <stdint.h>

/*====================
   COLOR SETTINGS
====================*/
#define LV_COLOR_DEPTH     16
#define LV_COLOR_16_SWAP   0
#define LV_USE_METER        1
#define LV_USE_THEME_DEFAULT 1
/*====================
   MEMORY SETTINGS
====================*/
/* Route ALL LVGL allocations (lv_obj_t, styles, animations, etc.) to PSRAM.
   This frees ~48 KB of internal SRAM and removes the fixed pool limit.
   heap_caps_malloc/realloc/free are part of esp-idf, always available. */
#define LV_MEM_CUSTOM      1
#define LV_MEM_CUSTOM_INCLUDE   <esp_heap_caps.h>
#define LV_MEM_CUSTOM_ALLOC(size)          heap_caps_malloc(size,   MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)
#define LV_MEM_CUSTOM_REALLOC(ptr,newsize) heap_caps_realloc(ptr, newsize, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)
#define LV_MEM_CUSTOM_FREE(ptr)            heap_caps_free(ptr)

/*====================
   DISPLAY BUFFER
====================*/
/* 1/10th of screen — 320*24 pixels */
#define LV_DISP_DEF_REFR_PERIOD   16      /* ~60 fps target */

/*====================
   INPUT DEVICES
====================*/
#define LV_INDEV_DEF_READ_PERIOD  30

/*====================
   FEATURES
====================*/
#define LV_USE_ANIMATION      1
#define LV_USE_SHADOW         1
#define LV_SHADOW_CACHE_SIZE  0
#define LV_USE_BLEND_MODES    1
#define LV_USE_OPA_SCALE      1
#define LV_USE_IMG_TRANSFORM  1
#define LV_USE_GROUP          1
#define LV_USE_GPU            0      /* No GPU on this config */
#define LV_USE_FILESYSTEM     0
#define LV_USE_USER_DATA      1

/*====================
   WIDGETS — enable only what we use (saves RAM)
====================*/
#define LV_USE_ARC         1
#define LV_USE_BAR         1
#define LV_USE_BTN         1
#define LV_USE_BTNMATRIX   1
#define LV_USE_CANVAS      1
#define LV_USE_CHECKBOX    0
#define LV_USE_DROPDOWN    0
#define LV_USE_IMG         1
#define LV_USE_LABEL       1
#define LV_USE_LINE        1
#define LV_USE_ROLLER      0
#define LV_USE_SLIDER      0
#define LV_USE_SWITCH      0
#define LV_USE_TEXTAREA    0
#define LV_USE_TABLE       0
#define LV_USE_TABVIEW     1
#define LV_USE_MSGBOX      0
#define LV_USE_SPINBOX     0
#define LV_USE_SPINNER     1
#define LV_USE_TILEVIEW    1
#define LV_USE_WIN         0
#define LV_USE_SPAN        0

/* Extra widgets — explicitly disable those with unmet dependencies */
#define LV_USE_KEYBOARD    0   /* requires LV_USE_BTNMATRIX + LV_USE_TEXTAREA */
#define LV_USE_LIST        0
#define LV_USE_MENU        0
#define LV_USE_COLORWHEEL  0
#define LV_USE_IMGBTN      0
#define LV_USE_LED         0
#define LV_USE_CALENDAR    0   /* requires LV_USE_BTNMATRIX */
#define LV_USE_CHART       0
#define LV_USE_METER       1   /* used — no dependencies */

/*====================
   FONTS
====================*/
/* Built-in fonts — enable the sizes we use */
#define LV_FONT_MONTSERRAT_8   0
#define LV_FONT_MONTSERRAT_10  0
#define LV_FONT_MONTSERRAT_12  1
#define LV_FONT_MONTSERRAT_14  1
#define LV_FONT_MONTSERRAT_16  1
#define LV_FONT_MONTSERRAT_18  0
#define LV_FONT_MONTSERRAT_20  1
#define LV_FONT_MONTSERRAT_22  0
#define LV_FONT_MONTSERRAT_24  1
#define LV_FONT_MONTSERRAT_26  0
#define LV_FONT_MONTSERRAT_28  0
#define LV_FONT_MONTSERRAT_30  0
#define LV_FONT_MONTSERRAT_32  1
#define LV_FONT_MONTSERRAT_34  0
#define LV_FONT_MONTSERRAT_36  0
#define LV_FONT_MONTSERRAT_38  0
#define LV_FONT_MONTSERRAT_40  0
#define LV_FONT_MONTSERRAT_42  0
#define LV_FONT_MONTSERRAT_44  0
#define LV_FONT_MONTSERRAT_46  0
#define LV_FONT_MONTSERRAT_48  1

#define LV_FONT_DEFAULT        &lv_font_montserrat_14

#define LV_USE_FONT_SUBPX      1
#define LV_FONT_SUBPX_BGR      0

/*====================
   LOGGING
====================*/
#define LV_USE_LOG             0    /* Disable for production */

/*====================
   ASSERT / DEBUG
====================*/
#define LV_USE_ASSERT_NULL          1
#define LV_USE_ASSERT_MALLOC        1
#define LV_USE_ASSERT_STYLE         0
#define LV_USE_ASSERT_MEM_INTEGRITY 0
#define LV_USE_ASSERT_OBJ           0

/*====================
   MISC
====================*/
#define LV_USE_PERF_MONITOR    0
#define LV_USE_MEM_MONITOR     0
#define LV_USE_REFR_DEBUG      0
#define LV_SPRINTF_CUSTOM      0
#define LV_USE_LARGE_COORD     0

#endif /* LV_CONF_H */
#endif /* End of "Set this to 1 ..." */
