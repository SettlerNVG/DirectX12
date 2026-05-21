// stb_image.h - v2.28 - public domain image loader
// Это заглушка для компиляции. Скачайте полную версию с:
// https://github.com/nothings/stb/blob/master/stb_image.h

#ifndef STB_IMAGE_H
#define STB_IMAGE_H

#ifdef __cplusplus
extern "C" {
#endif

// Основные функции stb_image
typedef unsigned char stbi_uc;

// Загрузка изображения
stbi_uc *stbi_load(char const *filename, int *x, int *y, int *channels_in_file, int desired_channels);

// Освобождение памяти
void stbi_image_free(void *retval_from_stbi_load);

// Настройки
void stbi_set_flip_vertically_on_load(int flag_true_if_should_flip);

// Информация об изображении
int stbi_info(char const *filename, int *x, int *y, int *comp);

// Причина ошибки
const char *stbi_failure_reason(void);

#ifdef __cplusplus
}
#endif

// ============================================================================
// IMPLEMENTATION (только если определён STB_IMAGE_IMPLEMENTATION)
// ============================================================================

#ifdef STB_IMAGE_IMPLEMENTATION

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int stbi__vertically_flip_on_load = 0;

void stbi_set_flip_vertically_on_load(int flag) {
    stbi__vertically_flip_on_load = flag;
}

const char *stbi_failure_reason(void) {
    return "stb_image stub - download full version from GitHub";
}

// Заглушка для загрузки - возвращает NULL
stbi_uc *stbi_load(char const *filename, int *x, int *y, int *channels_in_file, int desired_channels) {
    // Это заглушка! Скачайте полную версию stb_image.h
    // https://raw.githubusercontent.com/nothings/stb/master/stb_image.h
    
    // Для компиляции возвращаем NULL
    if (x) *x = 0;
    if (y) *y = 0;
    if (channels_in_file) *channels_in_file = 0;
    return NULL;
}

void stbi_image_free(void *retval_from_stbi_load) {
    if (retval_from_stbi_load) {
        free(retval_from_stbi_load);
    }
}

int stbi_info(char const *filename, int *x, int *y, int *comp) {
    return 0;
}

#endif // STB_IMAGE_IMPLEMENTATION

#endif // STB_IMAGE_H

/*
ИНСТРУКЦИЯ ПО УСТАНОВКЕ ПОЛНОЙ ВЕРСИИ:

1. Скачайте stb_image.h с GitHub:
   https://raw.githubusercontent.com/nothings/stb/master/stb_image.h

2. Замените этот файл скачанной версией

3. Перекомпилируйте проект

ПОДДЕРЖИВАЕМЫЕ ФОРМАТЫ:
- JPEG baseline & progressive
- PNG 1/2/4/8/16-bit-per-channel
- TGA
- BMP non-1bpp, non-RLE
- PSD (composited view only, no extra channels)
- GIF
- HDR (radiance rgbE format)
- PIC (Softimage PIC)
- PNM (PPM and PGM binary only)

ИСПОЛЬЗОВАНИЕ:
    int width, height, channels;
    unsigned char *data = stbi_load("image.png", &width, &height, &channels, 0);
    // ... используйте data ...
    stbi_image_free(data);
*/
