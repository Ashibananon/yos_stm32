#include <stdio.h>
#include "yos_ssd1306_font.h"
#include "ssd1306xled.h"

static int _yos_ssd1306_is_out_of_range(struct yos_ssd1306_font *font, uint8_t x, uint8_t y)
{
	if (x + font->width >= YOS_SSD1306_SCREEN_WIDTH
		|| y + font->row_take > YOS_SSD1306_SCREEN_HEIGHT / YOS_SSD1306_SCREEN_MEM_UNIT_HEIGHT) {
		return 1;
	} else {
		return 0;
	}
}

uint8_t yos_ssd1306_char_col_to_pixel(struct yos_ssd1306_font *font, uint8_t col)
{
	if (font == NULL) {
		return YOS_SSD1306_SCREEN_WIDTH;
	}
	return col * font->width;
}

void yos_ssd1306_puts(struct yos_ssd1306_font *font, uint8_t x, uint8_t y, char *str)
{
	if (font == NULL || str == NULL
		|| font->width == 0 || font->height == 0 || font->get_font_byte == NULL) {
		return;
	}

	char *c = str;
	uint16_t index;
	uint16_t i, j;
	while (*c != '\0') {
		if (_yos_ssd1306_is_out_of_range(font, x, y)) {
			break;
		}

		index = (*c - font->start_char) * (font->width * font->row_take);

		j = 0;
		while (j < font->row_take) {
			i = 0;
			ssd1306_setpos(x, y + j);
			ssd1306_start_data();
			while (i < font->width) {
				if (index + i < font->font_data_len) {
					ssd1306_byte(font->get_font_byte(index + i));
				} else {
					break;
				}
				i++;
			}
			ssd1306_stop();

			index += font->width;
			j++;
		}

		x += font->width;
		c++;
	}
}
