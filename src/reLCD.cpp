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
  uint8_t  rastr[8]; // Symbol bitmap
  uint16_t unicode;  // Character code in unicode
} image_char_t;

// Russian symbols
const image_char_t symbol_images[] = {
  {{0b11111, 0b10000, 0b10000, 0b11110, 0b10001, 0b10001, 0b11110, 0b00000}, 1041}, // Б
  {{0b11111, 0b10000, 0b10000, 0b10000, 0b10000, 0b10000, 0b10000, 0b00000}, 1043}, // Г
  {{0b00110, 0b01010, 0b01010, 0b01010, 0b01010, 0b01010, 0b11111, 0b10001}, 1044}, // Д
  {{0b10101, 0b10101, 0b10101, 0b01110, 0b10101, 0b10101, 0b10101, 0b00000}, 1046}, // Ж
  {{0b01110, 0b10001, 0b00001, 0b00110, 0b00001, 0b10001, 0b01110, 0b00000}, 1047}, // З
  {{0b10001, 0b10001, 0b10001, 0b10011, 0b10101, 0b11001, 0b10001, 0b00000}, 1048}, // И
  {{0b10101, 0b10001, 0b10001, 0b10011, 0b10101, 0b11001, 0b10001, 0b00000}, 1049}, // Й 
  {{0b00111, 0b01001, 0b01001, 0b01001, 0b01001, 0b01001, 0b10001, 0b00000}, 1051}, // Л
  {{0b11111, 0b10001, 0b10001, 0b10001, 0b10001, 0b10001, 0b10001, 0b00000}, 1055}, // П
  {{0b10001, 0b10001, 0b10001, 0b01111, 0b00001, 0b10001, 0b01110, 0b00000}, 1059}, // У
  {{0b00100, 0b01110, 0b10101, 0b10101, 0b10101, 0b01110, 0b00100, 0b00000}, 1060}, // Ф
  {{0b10010, 0b10010, 0b10010, 0b10010, 0b10010, 0b10010, 0b11111, 0b00001}, 1062}, // Ц
  {{0b10001, 0b10001, 0b10001, 0b01111, 0b00001, 0b00001, 0b00001, 0b00000}, 1063}, // Ч
  {{0b10001, 0b10001, 0b10001, 0b10101, 0b10101, 0b10101, 0b11111, 0b00000}, 1064}, // Ш
  {{0b10001, 0b10001, 0b10001, 0b10101, 0b10101, 0b10101, 0b11111, 0b00001}, 1065}, // Щ
  {{0b11000, 0b01000, 0b01000, 0b01110, 0b01001, 0b01001, 0b01110, 0b00000}, 1066}, // Ъ
  {{0b10001, 0b10001, 0b10001, 0b11101, 0b10011, 0b10011, 0b11101, 0b00000}, 1067}, // Ы 
  {{0b10000, 0b10000, 0b10000, 0b11110, 0b10001, 0b10001, 0b11110, 0b00000}, 1068}, // Ь
  {{0b01110, 0b10001, 0b00001, 0b00111, 0b00001, 0b10001, 0b01110, 0b00000}, 1069}, // Э
  {{0b10010, 0b10101, 0b10101, 0b11101, 0b10101, 0b10101, 0b10010, 0b00000}, 1070}, // Ю 
  {{0b01111, 0b10001, 0b10001, 0b01111, 0b00101, 0b01001, 0b10001, 0b00000}, 1071}, // Я
  {{0b00011, 0b01100, 0b10000, 0b11110, 0b10001, 0b10001, 0b01110, 0b00000}, 1073}, // б
  {{0b00000, 0b00000, 0b11110, 0b10001, 0b11110, 0b10001, 0b11110, 0b00000}, 1074}, // в
  {{0b00000, 0b00000, 0b11110, 0b10000, 0b10000, 0b10000, 0b10000, 0b00000}, 1075}, // г
  {{0b00000, 0b00000, 0b00110, 0b01010, 0b01010, 0b01010, 0b11111, 0b10001}, 1076}, // д
  {{0b01010, 0b00000, 0b01110, 0b10001, 0b11111, 0b10000, 0b01111, 0b00000}, 1105}, // ё
  {{0b00000, 0b00000, 0b10101, 0b10101, 0b01110, 0b10101, 0b10101, 0b00000}, 1078}, // ж
  {{0b00000, 0b00000, 0b01110, 0b10001, 0b00110, 0b10001, 0b01110, 0b00000}, 1079}, // з
  {{0b00000, 0b00000, 0b10001, 0b10011, 0b10101, 0b11001, 0b10001, 0b00000}, 1080}, // и
  {{0b01010, 0b00100, 0b10001, 0b10011, 0b10101, 0b11001, 0b10001, 0b00000}, 1081}, // й
  {{0b00000, 0b00000, 0b10010, 0b10100, 0b11000, 0b10100, 0b10010, 0b00000}, 1082}, // к
  {{0b00000, 0b00000, 0b00111, 0b01001, 0b01001, 0b01001, 0b10001, 0b00000}, 1083}, // л
  {{0b00000, 0b00000, 0b10001, 0b11011, 0b10101, 0b10001, 0b10001, 0b00000}, 1084}, // м
  {{0b00000, 0b00000, 0b10001, 0b10001, 0b11111, 0b10001, 0b10001, 0b00000}, 1085}, // н 
  {{0b00000, 0b00000, 0b11111, 0b10001, 0b10001, 0b10001, 0b10001, 0b00000}, 1087}, // п
  {{0b00000, 0b00000, 0b11111, 0b00100, 0b00100, 0b00100, 0b00100, 0b00000}, 1090}, // т
  {{0b00000, 0b00000, 0b00100, 0b01110, 0b10101, 0b01110, 0b00100, 0b00000}, 1092}, // ф
  {{0b00000, 0b00000, 0b10010, 0b10010, 0b10010, 0b10010, 0b11111, 0b00001}, 1094}, // ц
  {{0b00000, 0b00000, 0b10001, 0b10001, 0b01111, 0b00001, 0b00001, 0b00000}, 1095}, // ч
  {{0b00000, 0b00000, 0b10101, 0b10101, 0b10101, 0b10101, 0b11111, 0b00000}, 1096}, // ш
  {{0b00000, 0b00000, 0b10101, 0b10101, 0b10101, 0b10101, 0b11111, 0b00001}, 1097}, // щ
  {{0b00000, 0b00000, 0b11000, 0b01000, 0b01110, 0b01001, 0b01110, 0b00000}, 1098}, // ъ
  {{0b00000, 0b00000, 0b10001, 0b10001, 0b11101, 0b10011, 0b11101, 0b00000}, 1099}, // ы
  {{0b00000, 0b00000, 0b10000, 0b10000, 0b11110, 0b10001, 0b11110, 0b00000}, 1100}, // ь
  {{0b00000, 0b00000, 0b01110, 0b10001, 0b00111, 0b10001, 0b01110, 0b00000}, 1101}, // э
  {{0b00000, 0b00000, 0b10010, 0b10101, 0b11101, 0b10101, 0b10010, 0b00000}, 1102}, // ю
  {{0b00000, 0b00000, 0b01111, 0b10001, 0b01111, 0b00101, 0b01001, 0b00000}, 1103}  // я
};
const uint8_t count_images = sizeof(symbol_images) / sizeof(image_char_t);

#endif // LCD_RUS_USE_CUSTOM_CHARS

reLCD::reLCD(uint8_t i2c_bus, uint8_t i2c_addr, uint8_t cols, uint8_t rows)
{
  _I2C_num = i2c_bus;
  _I2C_addr = i2c_addr;
  _cols = cols;
  _rows = rows;
  _backlightval = LCD_NOBACKLIGHT;
  #if LCD_RUS_USE_CUSTOM_CHARS
    for (uint8_t i = 0; i < MAX_CUSTOM_CHARS; i++) {
      _buf_chars[i].unicode = 0;
      _buf_chars[i].count = 0;
    };
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

uint16_t reLCD::write(uint8_t value) 
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

uint16_t reLCD::writerc(uint16_t chr)
{
  for (uint8_t i = 0; i < MAX_CUSTOM_CHARS; i++) {
    // Scan buffer
    if (_buf_chars[i].unicode == chr) {
      _buf_chars[i].count++;
      return write(i);
    };
    // Copy symbol in buffer
    for (uint8_t j = 0; j < count_images; j++) {
      if (symbol_images[j].unicode == chr) {
        // Searching for a blank character
        uint8_t index = 255;
        for (uint8_t i = 0; i < MAX_CUSTOM_CHARS; i++) {
          if (_buf_chars[i].unicode == 0) {
            index = i;
            break;
          };
        };
        // All cells are busy
        if (index == 255) {
          index = 0;
          uint32_t min_count = _buf_chars[0].count;
          for (uint8_t i = 1; i < MAX_CUSTOM_CHARS; i++) {
            if (_buf_chars[i].count < min_count) {
              index = i;
              min_count = _buf_chars[i].count;
            };
          };
        };
        // Find symbol in images
        for (uint8_t i = 0; i < count_images; i++) {
          if (symbol_images[i].unicode == chr) {
            uint8_t col = _col;
            uint8_t row = _row;
            // Put custom char
            createChar(index, (uint8_t*)(symbol_images[i].rastr));
            _buf_chars[index].unicode = chr;
            _buf_chars[index].count = 1;
            // Print custom char
            setCursor(col, row);
            return write(index);
          };
        };
      };
    };
  };
  // Unknown char
  return write('?');
}

uint16_t reLCD::writewc(wchar_t chr)
{
  // English alphabet without change
  if (chr < 128) {
    return write((uint8_t)chr);
  } else {
    // Russian alphabet using the same characters as the English alphabet
    switch (chr) {
      case 1040:    return write('A');
      case 1042:    return write('B');
      case 1045:    return write('E');
      case 1025:    return write('E');
      case 1050:    return write('K');
      case 1052:    return write('M');
      case 1053:    return write('H');
      case 1054:    return write('O');
      case 1056:    return write('P');
      case 1057:    return write('C');
      case 1058:    return write('T');
      case 1061:    return write('X');
      case 1072:    return write('a');
      case 1077:    return write('e');
      case 1086:    return write('o');
      case 1088:    return write('p');
      case 1089:    return write('c');
      case 1091:    return write('y');
      case 1093:    return write('x');
      case 0x00B0:  return write(223);
      default:      return writerc((uint16_t)chr);
    };
  };
  return 0;
}

#else

uint16_t reLCD::writewc(wchar_t chr)
{
  // todo: It may not work, but there is nothing to check on
  return write((uint8_t)chr);
}

#endif // LCD_RUS_USE_CUSTOM_CHARS

uint16_t reLCD::printstr(const char* text)
{
  uint16_t len = strlen(text);
  if (len > 0) {
    uint16_t pos = 0;
    wchar_t buf;
    while (pos < len) {
      pos += mbtowc(&buf, (char*)text + pos, 2);
      writewc(buf);
    };
  };
  return len;
}

uint16_t reLCD::printpos(uint8_t col, uint8_t row, const char* text)
{
  setCursor(col, row);
  return printstr(text);
}

uint16_t reLCD::printf(const char* fmtstr, ...)
{
  va_list args;
  va_start(args, fmtstr);
  uint16_t len = vsnprintf(nullptr, 0, fmtstr, args);
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
  writeI2C(_I2C_num, _I2C_addr, &buf, 1, nullptr, 0, 1000);                                 
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

