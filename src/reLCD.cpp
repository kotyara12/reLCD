#include "reLCD.h"
#include "reI2C.h"
#include "reEsp32.h"
#include <rom/ets_sys.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include "project_config.h"

// Commands
#define LCD_CLEARDISPLAY        0x01
#define LCD_RETURNHOME          0x02
#define LCD_ENTRYMODESET        0x04
#define LCD_DISPLAYCONTROL      0x08
#define LCD_CURSORSHIFT         0x10
#define LCD_FUNCTIONSET         0x20
#define LCD_SETCGRAMADDR        0x40
#define LCD_SETDDRAMADDR        0x80

#define En BIT2  // B00000100 Enable bit
#define Rw BIT1  // B00000010 Read/Write bit
#define Rs BIT0  // B00000001 Register select bit

#define B00000 0x00
#define B00001 0x01
#define B11111 0x1F

#define constrainb(amt,low,high) ((amt)<(low)?(low):((amt)>(high)?(high):(amt)))
#define constrainh(amt,high) ((amt)>(high)?(high):(amt))

#if LCD_RUS_USE_CUSTOM_CHARS

typedef struct {
  uint8_t rastr[8];   // Symbol bitmap
  uint8_t charcode;   // Character code in unicode
} image_char_t;

// Russian symbols
const image_char_t rus_chars[] = {
  {{0b11111, 0b10000, 0b10000, 0b11110, 0b10001, 0b10001, 0b11110, 0b00000}, 193}, // 0x0411, 0xD091, 193 :: Б
  {{0b11111, 0b10000, 0b10000, 0b10000, 0b10000, 0b10000, 0b10000, 0b00000}, 195}, // 0x0413, 0xD093, 195 :: Г
  {{0b00110, 0b01010, 0b01010, 0b01010, 0b01010, 0b01010, 0b11111, 0b10001}, 196}, // 0x0414, 0xD094, 196 :: Д
  {{0b10101, 0b10101, 0b10101, 0b01110, 0b10101, 0b10101, 0b10101, 0b00000}, 198}, // 0x0416, 0xD096, 198 :: Ж
  {{0b01110, 0b10001, 0b00001, 0b00110, 0b00001, 0b10001, 0b01110, 0b00000}, 199}, // 0x0417, 0xD097, 199 :: З
  {{0b10001, 0b10001, 0b10001, 0b10011, 0b10101, 0b11001, 0b10001, 0b00000}, 200}, // 0x0418, 0xD098, 200 :: И
  {{0b10101, 0b10001, 0b10001, 0b10011, 0b10101, 0b11001, 0b10001, 0b00000}, 201}, // 0x0419, 0xD099, 201 :: Й
  {{0b00111, 0b01001, 0b01001, 0b01001, 0b01001, 0b01001, 0b10001, 0b00000}, 203}, // 0x041B, 0xD09B, 203 :: Л
  {{0b11111, 0b10001, 0b10001, 0b10001, 0b10001, 0b10001, 0b10001, 0b00000}, 207}, // 0x041F, 0xD09F, 207 :: П
  {{0b10001, 0b10001, 0b10001, 0b01111, 0b00001, 0b10001, 0b01110, 0b00000}, 211}, // 0x0423, 0xD0A3, 211 :: У
  {{0b00100, 0b01110, 0b10101, 0b10101, 0b10101, 0b01110, 0b00100, 0b00000}, 212}, // 0x0424, 0xD0A4, 212 :: Ф
  {{0b10010, 0b10010, 0b10010, 0b10010, 0b10010, 0b10010, 0b11111, 0b00001}, 214}, // 0x0426, 0xD0A6, 214 :: Ц
  {{0b10001, 0b10001, 0b10001, 0b01111, 0b00001, 0b00001, 0b00001, 0b00000}, 215}, // 0x0427, 0xD0A7, 215 :: Ч
  {{0b10001, 0b10001, 0b10001, 0b10101, 0b10101, 0b10101, 0b11111, 0b00000}, 216}, // 0x0428, 0xD0A8, 216 :: Ш
  {{0b10001, 0b10001, 0b10001, 0b10101, 0b10101, 0b10101, 0b11111, 0b00001}, 217}, // 0x0429, 0xD0A9, 217 :: Щ
  {{0b11000, 0b01000, 0b01000, 0b01110, 0b01001, 0b01001, 0b01110, 0b00000}, 218}, // 0x042A, 0xD0AA, 218 :: Ъ
  {{0b10001, 0b10001, 0b10001, 0b11101, 0b10011, 0b10011, 0b11101, 0b00000}, 219}, // 0x042B, 0xD0AB, 219 :: Ы
  {{0b10000, 0b10000, 0b10000, 0b11110, 0b10001, 0b10001, 0b11110, 0b00000}, 220}, // 0x042C, 0xD0AC, 220 :: Ь
  {{0b01110, 0b10001, 0b00001, 0b00111, 0b00001, 0b10001, 0b01110, 0b00000}, 221}, // 0x042D, 0xD0AD, 221 :: Э
  {{0b10010, 0b10101, 0b10101, 0b11101, 0b10101, 0b10101, 0b10010, 0b00000}, 222}, // 0x042E, 0xD0AE, 222 :: Ю
  {{0b01111, 0b10001, 0b10001, 0b01111, 0b00101, 0b01001, 0b10001, 0b00000}, 223}, // 0x042F, 0xD0AF, 223 :: Я
  {{0b00011, 0b01100, 0b10000, 0b11110, 0b10001, 0b10001, 0b01110, 0b00000}, 225}, // 0x0431, 0xD0B1, 225 :: б
  {{0b00000, 0b00000, 0b11110, 0b10001, 0b11110, 0b10001, 0b11110, 0b00000}, 226}, // 0x0432, 0xD0B2, 226 :: в
  {{0b00000, 0b00000, 0b11110, 0b10000, 0b10000, 0b10000, 0b10000, 0b00000}, 227}, // 0x0433, 0xD0B3, 227 :: г
  {{0b00000, 0b00000, 0b00110, 0b01010, 0b01010, 0b01010, 0b11111, 0b10001}, 228}, // 0x0434, 0xD0B4, 228 :: д
  {{0b01010, 0b00000, 0b01110, 0b10001, 0b11111, 0b10000, 0b01111, 0b00000}, 184}, // 0x0451, 0xD191, 184 :: ё
  {{0b00000, 0b00000, 0b10101, 0b10101, 0b01110, 0b10101, 0b10101, 0b00000}, 230}, // 0x0436, 0xD0B6, 230 :: ж
  {{0b00000, 0b00000, 0b01110, 0b10001, 0b00110, 0b10001, 0b01110, 0b00000}, 231}, // 0x0437, 0xD0B7, 231 :: з
  {{0b00000, 0b00000, 0b10001, 0b10011, 0b10101, 0b11001, 0b10001, 0b00000}, 232}, // 0x0438, 0xD0B8, 232 :: и
  {{0b01010, 0b00100, 0b10001, 0b10011, 0b10101, 0b11001, 0b10001, 0b00000}, 233}, // 0x0439, 0xD0B9, 233 :: й
  {{0b00000, 0b00000, 0b10010, 0b10100, 0b11000, 0b10100, 0b10010, 0b00000}, 234}, // 0x043A, 0xD0BA, 234 :: к
  {{0b00000, 0b00000, 0b00111, 0b01001, 0b01001, 0b01001, 0b10001, 0b00000}, 235}, // 0x043B, 0xD0BB, 235 :: л
  {{0b00000, 0b00000, 0b10001, 0b11011, 0b10101, 0b10001, 0b10001, 0b00000}, 236}, // 0x043C, 0xD0BC, 236 :: м
  {{0b00000, 0b00000, 0b10001, 0b10001, 0b11111, 0b10001, 0b10001, 0b00000}, 237}, // 0x043D, 0xD0BD, 237 :: н
  {{0b00000, 0b00000, 0b11111, 0b10001, 0b10001, 0b10001, 0b10001, 0b00000}, 239}, // 0x043F, 0xD0BF, 239 :: п
  {{0b00000, 0b00000, 0b11111, 0b00100, 0b00100, 0b00100, 0b00100, 0b00000}, 242}, // 0x0442, 0xD182, 242 :: т
  {{0b00000, 0b00000, 0b00100, 0b01110, 0b10101, 0b01110, 0b00100, 0b00000}, 244}, // 0x0444, 0xD184, 244 :: ф
  {{0b00000, 0b00000, 0b10010, 0b10010, 0b10010, 0b10010, 0b11111, 0b00001}, 246}, // 0x0446, 0xD186, 246 :: ц
  {{0b00000, 0b00000, 0b10001, 0b10001, 0b01111, 0b00001, 0b00001, 0b00000}, 247}, // 0x0447, 0xD187, 247 :: ч
  {{0b00000, 0b00000, 0b10101, 0b10101, 0b10101, 0b10101, 0b11111, 0b00000}, 248}, // 0x0448, 0xD188, 248 :: ш
  {{0b00000, 0b00000, 0b10101, 0b10101, 0b10101, 0b10101, 0b11111, 0b00001}, 249}, // 0x0449, 0xD189, 249 :: щ
  {{0b00000, 0b00000, 0b11000, 0b01000, 0b01110, 0b01001, 0b01110, 0b00000}, 250}, // 0x044A, 0xD18A, 250 :: ъ
  {{0b00000, 0b00000, 0b10001, 0b10001, 0b11101, 0b10011, 0b11101, 0b00000}, 251}, // 0x044B, 0xD18B, 251 :: ы
  {{0b00000, 0b00000, 0b10000, 0b10000, 0b11110, 0b10001, 0b11110, 0b00000}, 252}, // 0x044C, 0xD18C, 252 :: ь
  {{0b00000, 0b00000, 0b01110, 0b10001, 0b00111, 0b10001, 0b01110, 0b00000}, 253}, // 0x044D, 0xD18D, 253 :: э
  {{0b00000, 0b00000, 0b10010, 0b10101, 0b11101, 0b10101, 0b10010, 0b00000}, 254}, // 0x044E, 0xD18E, 254 :: ю
  {{0b00000, 0b00000, 0b01111, 0b10001, 0b01111, 0b00101, 0b01001, 0b00000}, 255}  // 0x044F, 0xD18F, 255 :: я
};
const uint8_t count_images = sizeof(rus_chars) / sizeof(image_char_t);

#endif // LCD_RUS_USE_CUSTOM_CHARS

reLCD::reLCD(i2c_port_t i2c_bus, uint8_t i2c_addr, uint8_t cols, uint8_t rows)
{
  _I2C_num = i2c_bus;
  _I2C_addr = i2c_addr;
  _cols = cols;
  _rows = rows;
  _backlightval = LCD_NOBACKLIGHT;
  #if LCD_RUS_USE_CUSTOM_CHARS
    resetRusCustomChars();
  #endif // LCD_RUS_USE_CUSTOM_CHARS
}

void reLCD::init()
{
	_displayfunction = LCD_4BITMODE | LCD_1LINE | LCD_5x8DOTS;
	begin(_cols, _rows, LCD_5x8DOTS);  
}

void reLCD::begin(uint8_t cols, uint8_t lines, uint8_t charsize) 
{
	if (lines > 1) {
		_displayfunction |= LCD_2LINE;
	}
	_numlines = lines;

	// For some 1 line displays you can select a 10 pixel high font
	if ((charsize != 0) && (lines == 1)) {
		_displayfunction |= LCD_5x10DOTS;
	}

	// SEE PAGE 45/46 FOR INITIALIZATION SPECIFICATION!
	// according to datasheet, we need at least 40ms after power rises above 2.7V before sending commands. 
  ets_delay_us(50000);
  
	// Now we pull both RS and R/W low to begin commands
	expanderWrite(_backlightval);	// reset expanderand turn backlight off (Bit 8 =1)
  vTaskDelay(pdMS_TO_TICKS(100));

  // put the LCD into 4 bit mode
	// this is according to the hitachi HD44780 datasheet (figure 24, pg 46)
	
  // we start in 8bit mode, try to set 4 bit mode
	write4bits(0x30);
  vTaskDelay(pdMS_TO_TICKS(5));
	ets_delay_us(4500); // wait min 4.1ms
	
	// second try
	write4bits(0x30);
	ets_delay_us(4500); // wait min 4.1ms
	
	// third go!
	write4bits(0x30); 
	ets_delay_us(150);
	
	// finally, set to 4-bit interface
	write4bits(0x20); 

	// set # lines, font size, etc.
	command(LCD_FUNCTIONSET | _displayfunction);  
	
	// turn the display on with no cursor or blinking default
	_displaycontrol = LCD_DISPLAYON | LCD_CURSOROFF | LCD_BLINKOFF;
	setDisplay(true);
	
	// clear it off
	clear();
	
	// Initialize to default text direction (for roman languages)
	_displaymode = LCD_ENTRYLEFT | LCD_ENTRYSHIFTDECREMENT;
	
	// set the entry mode
	command(LCD_ENTRYMODESET | _displaymode);
	
	home();
}

void reLCD::clear()
{
  // clear display, set cursor position to zero
	command(LCD_CLEARDISPLAY);  
  // this command takes a long time!
	ets_delay_us(2000);
  // reset cursor position
  #if LCD_RUS_USE_CUSTOM_CHARS
    _col = 0; _row = 0;
    resetRusCustomChars();
  #endif // LCD_RUS_USE_CUSTOM_CHARS
}

// Clear particular segment of a row
void reLCD::clear(uint8_t rowStart, uint8_t colStart, uint8_t colCnt) 
{
  // Maintain input parameters
  rowStart = constrainh(rowStart, _rows - 1);
  colStart = constrainh(colStart, _cols - 1);
  colCnt   = constrainh(colCnt,   _cols - colStart);
  // Clear segment
  setCursor(colStart, rowStart);
  for (uint8_t i = 0; i < colCnt; i++) write(' ');
  // Go to segment start
  setCursor(colStart, rowStart);
}

void reLCD::home()
{
  // set cursor position to zero
	command(LCD_RETURNHOME);  
  // this command takes a long time!
	ets_delay_us(2000);
  // reset cursor position
  #if LCD_RUS_USE_CUSTOM_CHARS
    _col = 0; _row = 0;
  #endif // LCD_RUS_USE_CUSTOM_CHARS
}

void reLCD::setCursor(uint8_t col, uint8_t row)
{
	int row_offsets[] = { 0x00, 0x40, 0x14, 0x54 };
	if ( row > _numlines ) {
		row = _numlines-1;    // we count rows starting w/0
	}
  #if LCD_RUS_USE_CUSTOM_CHARS
  _col = col; _row = row;
  #endif // LCD_RUS_USE_CUSTOM_CHARS
	command(LCD_SETDDRAMADDR | (col + row_offsets[row]));
}

// Turn the display on/off (quickly)
void reLCD::setDisplay(bool enabled) 
{
  enabled ? _displaycontrol |= LCD_DISPLAYON : _displaycontrol &= ~LCD_DISPLAYON;
	command(LCD_DISPLAYCONTROL | _displaycontrol);
}

// Turns the underline cursor on/off
void reLCD::setCursorVisible(bool enabled) 
{
  enabled ? _displaycontrol |= LCD_CURSORON : _displaycontrol &= ~LCD_CURSORON;
	command(LCD_DISPLAYCONTROL | _displaycontrol);
}

// Turn on and off the blinking cursor
void reLCD::setBlink(bool enabled) 
{
  enabled ? _displaycontrol |= LCD_BLINKON : _displaycontrol &= ~LCD_BLINKON;
	command(LCD_DISPLAYCONTROL | _displaycontrol);
}

// Turn the (optional) backlight off/on
void reLCD::setBacklight(bool enabled) {
	enabled ? _backlightval = LCD_BACKLIGHT : _backlightval = LCD_NOBACKLIGHT;
	expanderWrite(0);
}

// This is for text that flows Left to Right
void reLCD::setRightToLeft(bool enabled)
{
  enabled ? _displaymode &= ~LCD_ENTRYLEFT : _displaymode |= LCD_ENTRYLEFT;
	command(LCD_ENTRYMODESET | _displaymode);
}

// This will 'right justify' text from the cursor
void reLCD::setAutoscroll(bool enabled) 
{
  enabled ? _displaymode |= LCD_ENTRYSHIFTINCREMENT : _displaymode &= ~LCD_ENTRYSHIFTINCREMENT;
	command(LCD_ENTRYMODESET | _displaymode);
}

// These commands scroll the display without changing the RAM
void reLCD::scrollDisplayLeft(void) 
{
	command(LCD_CURSORSHIFT | LCD_DISPLAYMOVE | LCD_MOVELEFT);
}

void reLCD::scrollDisplayRight(void) 
{
	command(LCD_CURSORSHIFT | LCD_DISPLAYMOVE | LCD_MOVERIGHT);
}

// Allows us to fill the first 8 CGRAM locations with custom characters
void reLCD::createChar(uint8_t location, uint8_t charmap[]) 
{
	location &= 0x7; // we only have 8 locations 0-7
	command(LCD_SETCGRAMADDR | (location << 3));
	for (int i=0; i<8; i++) {
		write(charmap[i]);
	}
}

/*********** mid level commands, for sending data/cmds ***********/

inline void reLCD::command(uint8_t value) 
{
	send(value, 0);
}

uint16_t reLCD::writeChar(uint8_t value) 
{
	send(value, Rs);
  #if LCD_RUS_USE_CUSTOM_CHARS
    _col++;
    if (_col >= _cols) {
      _col =0;
      _row++;
      if (_row >= _rows) {
        _row = 0;
      };
    };
  #endif // LCD_RUS_USE_CUSTOM_CHARS
	return 1;
}

#if LCD_RUS_USE_CUSTOM_CHARS

void reLCD::resetRusCustomChars()
{
  for (uint8_t j = 0; j < MAX_CUSTOM_CHARS; j++) {
    _buf_chars[j] = 0;
  };
}

uint8_t reLCD::writeRus(uint8_t chr)
{
  // Scan in buffer
  for (uint8_t i = 0; i < MAX_CUSTOM_CHARS; i++) {
    if (_buf_chars[i] == chr) {
      return writeChar(i);
    };
  };

  // Find symbol in images
  for (uint8_t i = 0; i < count_images; i++) {
    if (rus_chars[i].charcode == chr) {
      uint8_t col = _col;
      uint8_t row = _row;
      // Searching for a blank character
      uint8_t index = 255;
      for (uint8_t j = 0; j < MAX_CUSTOM_CHARS; j++) {
        if (_buf_chars[j] == 0) {
          index = j;
          break;
        };
      };
      // All buffers are busy - reset all
      if (index == 255) {
        index = 0;
        resetRusCustomChars();
      };
      // Create new custom char
      _buf_chars[index] = chr;
      createChar(index, (uint8_t*)(rus_chars[i].rastr));
      // Print custom char
      setCursor(col, row);
      return writeChar(index);
    };
  };

  // Unknown char
  return writeChar(chr);
}

uint8_t reLCD::write(uint8_t chr)
{
  // English alphabet without change
  if (chr < 128) {
    return writeChar(chr);
  } else {
    // Russian alphabet using the same characters as the English alphabet
    switch (chr) {
      case 192: return writeChar('A');
      case 194: return writeChar('B');
      case 197: return writeChar('E');
      case 168: return writeChar('E');
      case 202: return writeChar('K');
      case 204: return writeChar('M');
      case 205: return writeChar('H');
      case 206: return writeChar('O');
      case 208: return writeChar('P');
      case 209: return writeChar('C');
      case 210: return writeChar('T');
      case 213: return writeChar('X');
      case 224: return writeChar('a');
      case 229: return writeChar('e');
      case 184: return writeChar('e');
      case 238: return writeChar('o');
      case 240: return writeChar('p');
      case 241: return writeChar('c');
      case 243: return writeChar('y');
      case 245: return writeChar('x');
      case 223: return writeChar(223);
      default:  return writeRus(chr);
    };
  };
  return 0;
}

#else

uint16_t reLCD::write(uint8_t chr)
{
  return writeChar(chr);
}

#endif // LCD_RUS_USE_CUSTOM_CHARS

uint8_t reLCD::printstr(const char* text)
{
  uint8_t len = strlen(text);
  if (len > 0) {
    uint8_t pos = 0;
    while (pos < len) {
      // utf-8 D0 :: А..Я а..п
      if ((pos+1 < len) && (text[pos] == 0xD0) && (text[pos+1] >= 0x90) && (text[pos+1] <= 0xBF)) {
        write(text[pos+1]+0x30);
        pos += 2;
      } 
      // utf-8 D1 :: р..я
      else if ((pos+1 < len) && (text[pos] == 0xD1) && (text[pos+1] >= 0x80) && (text[pos+1] <= 0x8F)) {
        write(text[pos+1]+0x70);
        pos += 2;
      } 
      // utf-8 C2 :: °
      else if ((pos+1 < len) && (text[pos] == 0xC2) && (text[pos+1] == 0xB0)) {
        write(0xDF);
        pos += 2;
      } 
      else {
        write(text[pos]);
        pos++;
      };
    };
  };
  return len;
}

uint8_t reLCD::printpos(uint8_t col, uint8_t row, const char* text)
{
  setCursor(col, row);
  return printstr(text);
}

uint8_t reLCD::printf(const char* fmtstr, ...)
{
  va_list args;
  va_start(args, fmtstr);
  uint8_t len = vsnprintf(nullptr, 0, fmtstr, args);
  char* text = (char*)esp_calloc(1, len+1);
  if (text) {
    vsnprintf(text, len+1, fmtstr, args);
  };
  va_end(args);
  if (text) {
    len = printstr((const char*)text);
    free(text);
    return len;
  };
  return 0;
}

uint8_t reLCD::printn(uint8_t col, uint8_t row, uint8_t width, const char* fmtstr, ...)
{
  va_list args;
  va_start(args, fmtstr);
  uint8_t len = vsnprintf(nullptr, 0, fmtstr, args);
  char* text = (char*)esp_calloc(1, len+1);
  if (text) {
    vsnprintf(text, len+1, fmtstr, args);
  };
  va_end(args);
  if (text) {
    setCursor(col, row);
    int8_t shift = width - len;
    // If the result of formatting is shorter than the specified width, add spaces in front
    if (shift > 0) {
      for (size_t i = 0; i < shift; i++) {
        writeChar(' ');
      };
    };
    // If the result of formatting is longer than the specified width, truncate the beginning of the string
    if (shift < 0) {
      len = printstr((const char*)text - shift) + shift;
    } else {
      len = printstr((const char*)text) + shift;
    };
    free(text);
    return len;
  };
  return 0;
}

/*********** low level data pushing commands ***********/

// write either command or data
void reLCD::send(uint8_t value, uint8_t mode) 
{
	uint8_t highnib = value & 0xF0;
	uint8_t lownib = value << 4;
	write4bits((highnib)|mode);
	write4bits((lownib)|mode);
}

void reLCD::write4bits(uint8_t value) 
{
	expanderWrite(value);
	pulseEnable(value);
}

void reLCD::expanderWrite(uint8_t data)
{       
  uint8_t buf = (int)(data) | _backlightval;
  esp_err_t err = writeI2C(_I2C_num, _I2C_addr, &buf, 1, nullptr, 0, 5000);                                 
  if (err != ESP_OK) {
    err = writeI2C(_I2C_num, _I2C_addr, &buf, 1, nullptr, 0, 5000);                                 
  };
}

void reLCD::pulseEnable(uint8_t data)
{
	expanderWrite(data | En);	 // En high
	ets_delay_us(1);		       // enable pulse must be >450ns
	
	expanderWrite(data & ~En); // En low
	ets_delay_us(40);		       // commands need > 37us to settle
}

// Create custom characters for horizontal graphs
uint8_t reLCD::graphHorizontalChars(uint8_t rowPattern) 
{
  uint8_t cc[LCD_CHARACTER_VERTICAL_DOTS];
  for (uint8_t idxCol = 0; idxCol < LCD_CHARACTER_HORIZONTAL_DOTS; idxCol++) {
    for (uint8_t idxRow = 0; idxRow < LCD_CHARACTER_VERTICAL_DOTS; idxRow++) {
      cc[idxRow] = rowPattern << (LCD_CHARACTER_HORIZONTAL_DOTS - 1 - idxCol);
    }
    createChar(idxCol, cc);
  }
  return LCD_CHARACTER_HORIZONTAL_DOTS;
}

// Create custom characters for vertical graphs
uint8_t reLCD::graphVerticalChars(uint8_t rowPattern) 
{
  uint8_t cc[LCD_CHARACTER_VERTICAL_DOTS];
  for (uint8_t idxChr = 0; idxChr < LCD_CHARACTER_VERTICAL_DOTS; idxChr++) {
    for (uint8_t idxRow = 0; idxRow < LCD_CHARACTER_VERTICAL_DOTS; idxRow++) {
      cc[LCD_CHARACTER_VERTICAL_DOTS - idxRow - 1] = idxRow > idxChr ? B00000 : rowPattern;
    }
    createChar(idxChr, cc);
  }
  return LCD_CHARACTER_VERTICAL_DOTS;
}

// Initializes custom characters for input graph type
uint8_t reLCD::init_bargraph(uint8_t graphtype) 
{
  // Initialize row state vector
  for(uint8_t i = 0; i < _rows; i++) {
    _graphstate[i] = 255;
  }
	switch (graphtype) {
		case LCDI2C_VERTICAL_BAR_GRAPH:
      graphVerticalChars(B11111);
      // Initialize column state vector
      for(uint8_t i = _rows; i < _cols; i++) {
        _graphstate[i] = 255;
      }
			break;
		case LCDI2C_HORIZONTAL_BAR_GRAPH:
      graphHorizontalChars(B11111);
			break;
		case LCDI2C_HORIZONTAL_LINE_GRAPH:
      graphHorizontalChars(B00001);
			break;
		default:
			return 1;
	}
  _graphtype = graphtype;
	return 0;
}

// Display horizontal graph from desired cursor position with input value
void reLCD::draw_horizontal_graph(uint8_t row, uint8_t column, uint8_t len, uint8_t pixel_col_end)
{
  // Maintain input parameters
  row = constrainh(row, _rows - 1);
  column = constrainh(column, _cols - 1);
  len = constrainh(len, _cols - column);
  pixel_col_end = constrainh(pixel_col_end, (len * LCD_CHARACTER_HORIZONTAL_DOTS) - 1);
  _graphstate[row] = constrainb(_graphstate[row], column, column + len - 1);
  // Display graph
  switch (_graphtype) {
    case LCDI2C_HORIZONTAL_BAR_GRAPH:
      setCursor(column, row);
      // Display full characters
      for (uint8_t i = 0; i < pixel_col_end / LCD_CHARACTER_HORIZONTAL_DOTS; i++) {
        write(LCD_CHARACTER_HORIZONTAL_DOTS - 1);
        column++;
      }
      // Display last character
      write(pixel_col_end % LCD_CHARACTER_HORIZONTAL_DOTS);
      // Clear remaining chars in segment
      for (uint8_t i = column; i < _graphstate[row]; i++) write(' ');
      // Last drawn column as graph state
      _graphstate[row] = column;
      break;
    case LCDI2C_HORIZONTAL_LINE_GRAPH:
      // Drawn column as graph state
      column += pixel_col_end / LCD_CHARACTER_HORIZONTAL_DOTS;
      // Clear previous drawn character if differs from new one
      if (_graphstate[row] != column) {
        setCursor(_graphstate[row], row);
        write(' ');
        _graphstate[row] = column;
      }
      // Display graph character
      setCursor(column, row);
      write(pixel_col_end % LCD_CHARACTER_HORIZONTAL_DOTS);
      break;
		default:
			return;
  }
}

// Display horizontal graph from desired cursor position with input value
void reLCD::draw_vertical_graph(uint8_t row, uint8_t column, uint8_t len,  uint8_t pixel_row_end) 
{
  // Maintain input parameters
  row = constrainh(row, _rows - 1);
  column = constrainh(column, _cols - 1);
  len = constrainh(len, row + 1);
  pixel_row_end = constrainh(pixel_row_end, (len * LCD_CHARACTER_VERTICAL_DOTS) - 1);
  _graphstate[column] = constrainb(_graphstate[column], row - len + 1, row);
  // Display graph
	switch (_graphtype) {
    case LCDI2C_VERTICAL_BAR_GRAPH:
      // Display full characters
      for (uint8_t i = 0; i < pixel_row_end / LCD_CHARACTER_VERTICAL_DOTS; i++) {
        setCursor(column, row--);
        write(LCD_CHARACTER_VERTICAL_DOTS - 1);
      }
      // Display the highest character
      setCursor(column, row);
      write(pixel_row_end % LCD_CHARACTER_VERTICAL_DOTS);
      // Clear remaining top chars in column
      for (uint8_t i = _graphstate[column]; i < row; i++) {
        setCursor(column, i);
        write(' ');
      }
      _graphstate[column] = row; // Last drawn row as its state
      break;
		default:
			return;
  }
}

// Overloaded methods
void reLCD::draw_horizontal_graph(uint8_t row, uint8_t column, uint8_t len, uint16_t percentage) 
{
  percentage = (percentage * len * LCD_CHARACTER_HORIZONTAL_DOTS / 100) - 1;
  draw_horizontal_graph(row, column, len, (uint8_t) percentage);
}

void reLCD::draw_horizontal_graph(uint8_t row, uint8_t column, uint8_t len, float ratio) 
{
  ratio = (ratio * len * LCD_CHARACTER_HORIZONTAL_DOTS) - 1;
  draw_horizontal_graph(row, column, len, (uint8_t) ratio);
}

void reLCD::draw_vertical_graph(uint8_t row, uint8_t column, uint8_t len,  uint16_t percentage) 
{
  percentage = (percentage * len * LCD_CHARACTER_VERTICAL_DOTS / 100) - 1;
  draw_vertical_graph(row, column, len, (uint8_t) percentage);
}

void reLCD::draw_vertical_graph(uint8_t row, uint8_t column, uint8_t len,  float ratio) 
{
  ratio = (ratio * len * LCD_CHARACTER_VERTICAL_DOTS) - 1;
  draw_vertical_graph(row, column, len, (uint8_t) ratio);
}

