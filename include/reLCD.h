/* 
   EN: LCD1602 or LCD2004 display driver for ESP32 and ESP-IDF via I2C
       Based on https://github.com/mrkaleArduinoLib/LiquidCrystal_I2C
   RU: Драйвер дисплея LCD1602 или LCD2004 для ESP32 и ESP-IDF через I2C
       Основан на https://github.com/mrkaleArduinoLib/LiquidCrystal_I2C
   --------------------------
   (с) 2023 Разживин Александр | Razzhivin Alexander
   kotyara12@yandex.ru | https://kotyara12.ru | tg: @kotyara1971
   --------------------------
   Страница проекта: https://github.com/kotyara12/reLCD
*/

#ifndef __RE_LCD_H__
#define __RE_LCD_H__

#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include <esp_err.h>

// Flags for display entry mode
#define LCD_ENTRYRIGHT          0x00
#define LCD_ENTRYLEFT           0x02
#define LCD_ENTRYSHIFTINCREMENT 0x01
#define LCD_ENTRYSHIFTDECREMENT 0x00

// Flags for display on/off control
#define LCD_DISPLAYON           0x04
#define LCD_DISPLAYOFF          0x00
#define LCD_CURSORON            0x02
#define LCD_CURSOROFF           0x00
#define LCD_BLINKON             0x01
#define LCD_BLINKOFF            0x00

// Flags for display/cursor shift
#define LCD_DISPLAYMOVE         0x08
#define LCD_CURSORMOVE          0x00
#define LCD_MOVERIGHT           0x04
#define LCD_MOVELEFT            0x00

// Flags for function set
#define LCD_4BITMODE            0x00
#define LCD_8BITMODE            0x10
#define LCD_1LINE               0x00
#define LCD_2LINE               0x08
#define LCD_5x10DOTS            0x04
#define LCD_5x8DOTS             0x00

// Flags for backlight control
#define LCD_BACKLIGHT           0x08 // B00001000
#define LCD_NOBACKLIGHT         0x00 // B00000000

// Values for graphtype in calls to init_bargraph and character geometry
#define LCDI2C_VERTICAL_BAR_GRAPH     1
#define LCDI2C_HORIZONTAL_BAR_GRAPH   2
#define LCDI2C_HORIZONTAL_LINE_GRAPH  3
#define LCD_CHARACTER_HORIZONTAL_DOTS 5
#define LCD_CHARACTER_VERTICAL_DOTS   8

#ifdef __cplusplus
extern "C" {
#endif

class reLCD {
  public:
    reLCD(uint8_t i2c_bus, uint8_t i2c_addr, uint8_t cols, uint8_t rows);
    void begin(uint8_t cols, uint8_t rows, uint8_t charsize = LCD_5x8DOTS);
    void init();
    // Clear display
    void clear();
    void clear(uint8_t rowStart, uint8_t colStart = 0, uint8_t colCnt = 255);
    // Move cursor
    void home();
    void setCursor(uint8_t col, uint8_t row); 
    // Options
    void setDisplay(bool enabled);
    void setBlink(bool enabled);
    void setCursor(bool enabled);
    void setRightToLeft(bool enabled);
    void setBacklight(bool enabled);
    void setAutoscroll(bool enabled); 
    // Scroll text
    void scrollDisplayLeft();
    void scrollDisplayRight();
    // Custom chars
    void createChar(uint8_t location, uint8_t charmap[]);
    // Send commands
    size_t write(uint8_t value);
    void command(uint8_t value);
    // Bar graphs
    uint8_t init_bargraph(uint8_t graphtype);
    void draw_horizontal_graph(uint8_t row, uint8_t column, uint8_t len, uint8_t pixel_col_end);
    void draw_vertical_graph(uint8_t row, uint8_t column, uint8_t len,  uint8_t pixel_row_end);
    void draw_horizontal_graph(uint8_t row, uint8_t column, uint8_t len, uint16_t percentage);
    void draw_horizontal_graph(uint8_t row, uint8_t column, uint8_t len, float ratio);
    void draw_vertical_graph(uint8_t row, uint8_t column, uint8_t len,  uint16_t percentage);
    void draw_vertical_graph(uint8_t row, uint8_t column, uint8_t len,  float ratio);
  private:
    uint8_t _I2C_num;
    uint8_t _I2C_addr;
    uint8_t _displayfunction;
    uint8_t _displaycontrol;
    uint8_t _displaymode;
    uint8_t _numlines;
    uint8_t _cols;
    uint8_t _rows;
    uint8_t _backlightval;
    uint8_t _graphtype;
    uint8_t _graphstate[20];

    void send(uint8_t value, uint8_t mode);
    void write4bits(uint8_t data);
    void expanderWrite(uint8_t data);
    void pulseEnable(uint8_t data);
    uint8_t graphHorizontalChars(uint8_t rowPattern);
    uint8_t graphVerticalChars(uint8_t rowPattern);
};


#ifdef __cplusplus
}
#endif

#endif // __RE_LCD_H__