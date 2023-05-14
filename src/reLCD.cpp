#include "reLCD.h"
#include "reI2C.h"
#include <rom/ets_sys.h>
#include "math.h"

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

reLCD::reLCD(uint8_t i2c_bus, uint8_t i2c_addr, uint8_t cols, uint8_t rows)
{
  _I2C_num = i2c_bus;
  _I2C_addr = i2c_addr;
  _cols = cols;
  _rows = rows;
  _backlightval = LCD_NOBACKLIGHT;
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
}

void reLCD::setCursor(uint8_t col, uint8_t row)
{
	int row_offsets[] = { 0x00, 0x40, 0x14, 0x54 };
	if ( row > _numlines ) {
		row = _numlines-1;    // we count rows starting w/0
	}
	command(LCD_SETDDRAMADDR | (col + row_offsets[row]));
}

// Turn the display on/off (quickly)
void reLCD::setDisplay(bool enabled) 
{
  enabled ? _displaycontrol |= LCD_DISPLAYON : _displaycontrol &= ~LCD_DISPLAYON;
	command(LCD_DISPLAYCONTROL | _displaycontrol);
}

// Turns the underline cursor on/off
void reLCD::setCursor(bool enabled) 
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

inline void reLCD::command(uint8_t value) {
	send(value, 0);
}

inline size_t reLCD::write(uint8_t value) {
	send(value, Rs);
	return 1; // Number of processed bytes
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
	ets_delay_us(50);		       // commands need > 37us to settle
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

