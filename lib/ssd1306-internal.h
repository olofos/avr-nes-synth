#ifndef SSD1306_INTERNAL_H_
#define SSD1306_INTERNAL_H_

struct ssd1306_frame_t
{
    uint8_t col_start;
    uint8_t col_end;

    uint8_t row_start;
    uint8_t row_end;

    uint8_t offset;
};

void ssd1306_setup_frame(const struct ssd1306_frame_t frame);

#endif
