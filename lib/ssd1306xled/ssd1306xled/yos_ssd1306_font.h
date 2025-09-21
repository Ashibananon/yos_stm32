#ifndef _YOS_SSD1306_FONT_H_
#define _YOS_SSD1306_FONT_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define YOS_SSD1306_SCREEN_WIDTH				128
#define YOS_SSD1306_SCREEN_HEIGHT				64
#define YOS_SSD1306_SCREEN_MEM_UNIT_HEIGHT		8
struct yos_ssd1306_font {
	uint8_t width;
	uint8_t height;
	uint8_t row_take;
	char start_char;
	uint16_t font_data_len;
	uint8_t (*get_font_byte)(uint16_t index);
};
extern struct yos_ssd1306_font *YOS_SSD1306_FONT_6X8;
extern struct yos_ssd1306_font *YOS_SSD1306_FONT_8X16;


uint8_t yos_ssd1306_char_col_to_pixel(struct yos_ssd1306_font *font, uint8_t col);
void yos_ssd1306_puts(struct yos_ssd1306_font *font, uint8_t x, uint8_t y, char *str);

#ifdef __cplusplus
}
#endif
#endif