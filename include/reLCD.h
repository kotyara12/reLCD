/* 
   EN: LCD1602 or LCD2004 display driver for ESP32 and ESP-IDF via I2C with russian chars
       Based on https://github.com/mrkaleArduinoLib/LiquidCrystal_I2C
   RU: Драйвер дисплея LCD1602 или LCD2004 для ESP32 и ESP-IDF через I2C с русификацией
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
#include "project_config.h"
#include "driver/i2c.h"

// EN: Flags for display entry mode
// RU: Флаги режима ввода отображения
#define LCD_ENTRYRIGHT          0x00
#define LCD_ENTRYLEFT           0x02
#define LCD_ENTRYSHIFTINCREMENT 0x01
#define LCD_ENTRYSHIFTDECREMENT 0x00

// EN: Flags for display on/off control
// RU: Флаги для управления дисплеем
#define LCD_DISPLAYON           0x04
#define LCD_DISPLAYOFF          0x00
#define LCD_CURSORON            0x02
#define LCD_CURSOROFF           0x00
#define LCD_BLINKON             0x01
#define LCD_BLINKOFF            0x00

// EN: Flags for display/cursor shift
// RU: Флаги для отображения/сдвига курсора
#define LCD_DISPLAYMOVE         0x08
#define LCD_CURSORMOVE          0x00
#define LCD_MOVERIGHT           0x04
#define LCD_MOVELEFT            0x00

// EN: Flags for function set
// RU: Флаги для установки функций
#define LCD_4BITMODE            0x00
#define LCD_8BITMODE            0x10
#define LCD_1LINE               0x00
#define LCD_2LINE               0x08
#define LCD_5x10DOTS            0x04
#define LCD_5x8DOTS             0x00

// EN: Flags for backlight control
// RU: Флаги для управления подсветкой
#define LCD_BACKLIGHT           0x08 // B00001000
#define LCD_NOBACKLIGHT         0x00 // B00000000

// EN: Values for graphtype in calls to init_bargraph and character geometry
// RU: Значения для graphtype в вызовах init_bargraph и геометрии символов
#define LCDI2C_VERTICAL_BAR_GRAPH     1
#define LCDI2C_HORIZONTAL_BAR_GRAPH   2
#define LCDI2C_HORIZONTAL_LINE_GRAPH  3
#define LCD_CHARACTER_HORIZONTAL_DOTS 5
#define LCD_CHARACTER_VERTICAL_DOTS   8

// EN: If the display has russian characters, define CONFIG_LCD_RUS_CODEPAGE 1
// RU: Если в дисплее есть русские символы, определите CONFIG_LCD_RUS_CODEPAGE 1
#if defined(CONFIG_LCD_RUS_CODEPAGE) && (CONFIG_LCD_RUS_CODEPAGE == 1)
  #define LCD_RUS_USE_CUSTOM_CHARS 0
#else
  #define LCD_RUS_USE_CUSTOM_CHARS 1
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_CUSTOM_CHARS 8

class reLCD {
  public:
    reLCD(i2c_port_t i2c_bus, uint8_t i2c_addr, uint8_t cols, uint8_t rows);
    void begin(uint8_t cols, uint8_t rows, uint8_t charsize = LCD_5x8DOTS);
    void init();
    // EN: Clear display
    // RU: Очистка дисплея
    void clear();
    void clear(uint8_t rowStart, uint8_t colStart = 0, uint8_t colCnt = 255);
    // EN: Move cursor
    // RU: Перемещение курсора
    void home();
    void setCursor(uint8_t col, uint8_t row); 
    // EN: Options
    // RU: Опции
    void setDisplay(bool enabled);
    void setBlink(bool enabled);
    void setCursorVisible(bool enabled);
    void setRightToLeft(bool enabled);
    void setBacklight(bool enabled);
    // EN: Scroll text
    // RU: Прокрутка текста
    void scrollDisplayLeft();
    void scrollDisplayRight();
    void setAutoscroll(bool enabled); 
    // EN: Send text
    // RU: Печать текста
    uint8_t write(uint8_t chr);
    uint8_t printstr(const char* text);
    uint8_t printpos(uint8_t col, uint8_t row, const char* text);
    uint8_t printf(const char* fmtstr, ...);
    uint8_t printn(uint8_t col, uint8_t row, uint8_t width, const char* fmtstr, ...);
    // EN: Custom chars
    // RU: Пользовательские символы
    void createChar(uint8_t location, uint8_t charmap[]);
    #if LCD_RUS_USE_CUSTOM_CHARS
      void resetRusCustomChars();
    #endif // LCD_RUS_USE_CUSTOM_CHARS
    // EN: Bar graphs
    // RU: Гистограммы
    uint8_t init_bargraph(uint8_t graphtype);
    void draw_horizontal_graph(uint8_t row, uint8_t column, uint8_t len, uint8_t pixel_col_end);
    void draw_vertical_graph(uint8_t row, uint8_t column, uint8_t len,  uint8_t pixel_row_end);
    void draw_horizontal_graph(uint8_t row, uint8_t column, uint8_t len, uint16_t percentage);
    void draw_horizontal_graph(uint8_t row, uint8_t column, uint8_t len, float ratio);
    void draw_vertical_graph(uint8_t row, uint8_t column, uint8_t len,  uint16_t percentage);
    void draw_vertical_graph(uint8_t row, uint8_t column, uint8_t len,  float ratio);
  private:
    i2c_port_t  _I2C_num;
    uint8_t     _I2C_addr;
    uint8_t     _displayfunction;
    uint8_t     _displaycontrol;
    uint8_t     _displaymode;
    uint8_t     _numlines;
    uint8_t     _cols;
    uint8_t     _rows;
    uint8_t     _backlightval;
    uint8_t     _graphtype;
    uint8_t     _graphstate[20];
    void send(uint8_t value, uint8_t mode);
    void command(uint8_t value);
    void expanderWrite(uint8_t data);
    void write4bits(uint8_t data);
    uint16_t writeChar(uint8_t value);
    void pulseEnable(uint8_t data);
    uint8_t graphHorizontalChars(uint8_t rowPattern);
    uint8_t graphVerticalChars(uint8_t rowPattern);
    #if LCD_RUS_USE_CUSTOM_CHARS
      uint8_t  _col;
      uint8_t  _row;
      uint8_t _buf_chars[MAX_CUSTOM_CHARS];
      uint8_t writeRus(uint8_t chr);
    #endif // LCD_RUS_USE_CUSTOM_CHARS
};


#ifdef __cplusplus
}
#endif

#endif // __RE_LCD_H__