/* lcdTerminal - serial adaptor for an LCD display
 *
 * (c) 2016 David Haworth
 *
 * lcdTerminal is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * lcdTerminal is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with DhG.  If not, see <http://www.gnu.org/licenses/>.
 *
 * $Id: lcdTerminal.ino 11 2016-03-29 20:50:03Z dave $
 *
 * lcdTerminal is an Arduino sketch, written for an Arduino Nano
 *
 * It accepts characters on the serial port. The UART is connected to
 * a USB/serial adaptor on the board but can probably be wired out to
 * RS232 via a MAX232 device.
 *
 * The software accepts a very small subset of the "ANSI Terminal" (VT102)
 * command set as used by xterm, among others.
 *
 * The LCD display module has an "industry standard" controller. Communication
 * with it is by means of an 8-bit data bus, a single-bit address bus (RS)
 * and two control lines (E and R/W).
 *
 * My module has 4 lines of 20 characters. Anything written past the
 * last column is ignored (no horizontal scrolling or wrap-around).
 * A line feed on the last line causes the display to scroll up.
 *
 * Control sequences implemented:
 *	FF - cursor to top left, clear screen (Xterm: same as linefeed)
 *	CR - cursor to left column
 *	LF - cursor down one row, with scrolling
 *	VT - cursor up one row, with scrolling
 *	BS - cursor one column to left (unless already at left side)
 *
 *	ESC[r;cH - cursor to row r, column c
 *	ESC[rA   - cursor up r rows
 *	ESC[rB   - cursor down r rows
 *	ESC[cC   - cursor right c columns
 *	ESC[cD   - cursor left c columns
 *	ESC[cD   - cursor left c columns
 *	ESC[0J   - clear lines above cursor (not including cursor line)
 *	ESC[1J   - clear lines below cursor (including cursor line)
 *	ESC[2J   - clear display
 *	ESC[0K   - clear to right of cursor (including cursor column)
 *	ESC[1K   - clear to left of cursor (not including cursor column. Xterm might be different)
 *	ESC[2K   - clear line
 *
 *	The c and r parameters above are strings of ascii numerals. If empty, default to 1.
 *  Ranges: 1 <= r <= lcd_nrows and 1 <= c <= lcd_ncols+1
 *  ncols+1 is logically 1 character off screen to the right, which is where the
 *  cursor ends up if lcd_ncols (or more) printing characters are sent.
 *  Out of range values are coerced into range
*/
#include <LiquidCrystal.h>

#define led1		13		// On-board LED connected to digital pin 13

#define lcd_d0		5		// Pins for LCD display
#define lcd_d1		6
#define lcd_d2		7
#define lcd_d3		8
#define lcd_d4		9
#define lcd_d5		10
#define lcd_d6		11
#define lcd_d7		12
#define lcd_e		4
#define lcd_rw		3
#define lcd_rs		2

#define lcd_nrows	4
#define lcd_ncols	20

/* ASCII control characters recognised
*/
#define CR	'\r'
#define LF	'\n'
#define VT	'\v'
#define BS	'\b'
#define FF	'\f'
#define ESC	'\e'
#define NUL '\0'

#define ctrlseq_nfields	2		// Max. no of numeric fields (separated by ';') in a control sequence

LiquidCrystal lcd(lcd_rs, lcd_rw, lcd_e,
				  lcd_d0, lcd_d1, lcd_d2, lcd_d3,
				  lcd_d4, lcd_d5, lcd_d6, lcd_d7);

/* Timed processing
*/
unsigned long then;
char ledState;

/* LCD buffers and cursor position
*/
char *lcdRows[lcd_nrows];
char lcdBuf[lcd_nrows*lcd_ncols];
int row, col;

/* Control sequence processing
*/
int ctrlseq_p[2];
int ctrlseq_pos;

void LcdPutc(int ch);
void LcdScrollUp(void);
void LcdScrollDown(void);
void LcdBackspace(void);
void LcdClearRow(int);
void LcdHomeAndClear(void);
int LcdCtrlSeq(int ch);
void LcdCtrlSetCursor(void);
void LcdCtrlMoveCursorUp(void);
void LcdCtrlMoveCursorDown(void);
void LcdCtrlMoveCursorRight(void);
void LcdCtrlMoveCursorLeft(void);
void LcdCtrlClearLines(void);
void LcdCtrlClearCharacters(void);

/* setup() - standard Arduino "Init Task"
*/
void setup(void)
{
	int i;

	pinMode(led1, OUTPUT);				// Sets the digital pin as output
	digitalWrite(led1, LOW);			// Drive the pin low (LED off)
	ledState = 0;

	Serial.begin(9600);					// Start the serial port.
	Serial.println("Hello world!");		// ToDo : it'll need a "who are you?" response

	lcd.begin(lcd_ncols, lcd_nrows);	// Start the lcd driver
	lcd.setCursor(0, 0);				// Cursor to top left.
	lcd.print("Hello world!");

	row = 3;							// Cursor to bottom left.
	col = 0;
	lcd.setCursor(col, row);
	for ( i=0; i<lcd_nrows; i++ )		// Initialise LCD buffers
	{
		lcdRows[i] = &lcdBuf[i*lcd_ncols];
	}

	ctrlseq_pos = -1;					// Initialise control sequence processing

	then = millis();					// Initialise the time reference.
}

/* loop() - standard Arduino "Background Task"
*/
void loop(void)
{
	unsigned long now = millis();
	unsigned long elapsed = now - then;
	int ch;

	if ( ledState )
	{
		/* LED stays on for 20 ms
		*/
		if ( elapsed > 20 )
		{
			then += 20;
			ledState = 0;
			digitalWrite(led1, LOW);
		}
	}
	else
	{
		/* LED stays off for 2 secs minus 20 ms
		*/
		if ( elapsed >= 1980 )
		{
			then += 1980;
			ledState = 1;
			digitalWrite(led1, HIGH);
		}
	}

	ch = Serial.read();
	if ( ch >= 0 )
	{
		/* Offer the character to the control sequence processor first.
		*/
		if ( !LcdCtrlSeq(ch) )
		{
			LcdPutc(ch);
		}
	}
}


/* LcdHomeAndClear() - clear screen, cursor to 0,0
*/
void LcdHomeAndClear(void)
{
	int i, j;

	lcd.clear();
	for ( i=0; i<lcd_nrows; i++ )
	{
		for ( j=0; j<lcd_ncols; j++ )
		{
			lcdRows[i][j] = NUL;
		}
	}

	row = 0;
	col = 0;
	lcd.setCursor(col, row);
}

/* LcdPutc() - write a single character to buffers and to LCD module
*/
void LcdPutc(int ch)
{
	if ( col < lcd_ncols )
	{
#if 0	/*	Apparently not working on lcd module. Need to set F bit? */
		/* Use characters with proper descenders.
		*/
		if ( ch == 'g' || ch == 'j' || ch == 'p' || ch == 'q' || ch == 'y' )
		{
			ch += 128;
		}
#endif
		lcdRows[row][col] = ch;
		lcd.write(ch);
		col++;
	}
	else
	{
		/* Ignore everything to the right of column lcd_ncols
		*/
	}
}

/* LcdScrollUp() - scroll the display up, blank line at bottom, cursor unchanged
*/
void LcdScrollUp(void)
{
	lcd.clear();				// I hate this!
	char *tmp = lcdRows[0];
	int i, j;

	for ( i = 0; i < (lcd_nrows-1); i++ )
	{
		lcdRows[i] = lcdRows[i+1];
		lcd.setCursor(0, i);
		for ( j=0; j<lcd_ncols && lcdRows[i][j] != NUL; j++ )
		{
			lcd.write(lcdRows[i][j]);
		}
	}

	for ( j=0; j<lcd_ncols; j++ )
	{
		tmp[j] = NUL;
	}

	lcdRows[lcd_nrows-1] = tmp;
}

/* LcdScrollDown() - scroll the display down, blank line at top, cursor unchanged
*/
void LcdScrollDown(void)
{
	lcd.clear();				// I hate this!
	char *tmp = lcdRows[lcd_nrows-1];
	int i, j;

	for ( i = (lcd_nrows-1); i > 0; i-- )
	{
		lcdRows[i] = lcdRows[i-1];
		lcd.setCursor(0, i);
		for ( j=0; j<lcd_ncols && lcdRows[i][j] != NUL; j++ )
		{
			lcd.write(lcdRows[i][j]);
		}
	}

	for ( j=0; j<lcd_ncols; j++ )
	{
		tmp[j] = NUL;
	}

	lcdRows[0] = tmp;
}

/* LcdBackspace() - set the cursor one column to the left.
*/
void LcdBackspace(void)
{
	col = col - 1;
	if ( col < 0 )				col = 0;				// Default to first column.

	lcd.setCursor(col, row);
}

/* LcdClearRow() - clear a row
 *
 * The cursor position is save and restored by the caller.
*/
void LcdClearRow(int r)
{
	int i;

	col = 0;
	row = r;
	lcd.setCursor(col, row);

	for (i = 0; i < lcd_ncols; i++ )
	{
		LcdPutc(' ');
	}
}

/* LcdCtrlSeq() - handle control sequences
*/
int LcdCtrlSeq(int ch)
{
	int eaten = 0;
	if ( ctrlseq_pos < 0 )
	{
		/* Not in a control sequence at the moment
		*/
		if ( ch < ' ' )
		{
			/* A control character ...
			*/
			eaten = 1;				// We ate the character

			if ( ch == ESC )
			{
				ctrlseq_pos = 0;	// Start of a control sequence
			}
			else
			if ( ch == BS )
			{
				LcdBackspace();
			}
			else
			if ( ch == CR )
			{
				col = 0;
				lcd.setCursor(col, row);
			}
			else
			if ( ch == LF )
			{
				if ( row >= 3 )
				{
					row = 3;		// Defensive
					LcdScrollUp();
				}
				else
				{
					row++;
				}
				lcd.setCursor(col, row);
			}
			else
			if ( ch == VT )
			{
				if ( row <= 0 )
				{
					row = 0;		// Defensive
					LcdScrollDown();
				}
				else
				{
					row--;
				}
				lcd.setCursor(col, row);
			}
			else
			if ( ch == FF )
			{
				LcdHomeAndClear();
			}
			else
			{
				/* Other control characters are passed through
				*/
				eaten = 0;
			}
		}
	}
	else
	if ( ctrlseq_pos == 0 )
	{
		/* First character.
		 * If it's '[' it could be one of the recognised  control sequences
		*/
		if ( ch == '[' )
		{
			ctrlseq_pos = 1;
			ctrlseq_p[0] = -1;
			ctrlseq_p[1] = -1;
			eaten = 1;			// We ate the character
		}
		else
		{
			/* Just use the escaped character as a character. (eatern = 0)
			*/
			ctrlseq_pos = -1;
		}
	}
	else
	if ( ctrlseq_pos <= ctrlseq_nfields )
	{
		if ( ch >= '0' && ch <= '9' )
		{
			/* In a numeric field.
			*/
			int d = ch - '0';
			if ( ctrlseq_p[ctrlseq_pos-1] < 0 )
			{
				ctrlseq_p[ctrlseq_pos-1] = d;
			}
			else
			{
				ctrlseq_p[ctrlseq_pos-1] = ctrlseq_p[ctrlseq_pos-1] * 10 + d;
			}
		}
		else
		if ( ch == ';' )
		{
			/* Move to next field.
			*/
			ctrlseq_pos++;
		}
		else
		if ( ch == 'H' || ch == 'f' )
		{
			LcdCtrlSetCursor();
			ctrlseq_pos = -1;		// ... and that's it
		}
		else
		if ( ch == 'A' )
		{
			LcdCtrlMoveCursorUp();
			ctrlseq_pos = -1;		// ... and that's it
		}
		else
		if ( ch == 'B' )
		{
			LcdCtrlMoveCursorDown();
			ctrlseq_pos = -1;		// ... and that's it
		}
		else
		if ( ch == 'C' )
		{
			LcdCtrlMoveCursorRight();
			ctrlseq_pos = -1;		// ... and that's it
		}
		else
		if ( ch == 'D' )
		{
			LcdCtrlMoveCursorLeft();
			ctrlseq_pos = -1;		// ... and that's it
		}
		else
		if ( ch == 'J' )
		{
			LcdCtrlClearLines();
			ctrlseq_pos = -1;		// ... and that's it
		}
		else
		if ( ch == 'K' )
		{
			LcdCtrlClearCharacters();
			ctrlseq_pos = -1;		// ... and that's it
		}
		else
		{
			/* Unknown control sequence - just ignore it.
			*/
			ctrlseq_pos = -1;
		}
		eaten = 1;			// We ate the character
	}
	else
	{
		/* Control sequence is too long. Drop it and treat character as raw inputs (eaten = 0).
		*/
		ctrlseq_pos = -1;
	}
	return eaten;
}

/* LcdCtrlSetCursor() - set the cursor position. Sequence "ESC [ row ; col H" or "ESC [ row ; col f"
*/
void LcdCtrlSetCursor(void)
{
	row = ctrlseq_p[0] - 1;
	row = ( row < 0 ) ? 0 : row;						// Default to first row.
	row = ( row >= lcd_nrows ) ? lcd_nrows-1 : row;		// Force to last row.

	col = ctrlseq_p[1] - 1;
	col = ( col < 0 ) ? 0 : col;						// Default to first column.
	col = ( col > lcd_ncols ) ? lcd_ncols : col;		// Force to "just off screen".

	lcd.setCursor(col, row);
}

/* LcdCtrlMoveCursorUp() - move the cursor r rows up.
*/
void LcdCtrlMoveCursorUp(void)
{
	int r = ctrlseq_p[0];
	if ( r < 0 )	r = 1;

	row = row - r;
	if ( row < 0 )	row = 0;

	lcd.setCursor(col, row);
}

/* LcdCtrlMoveCursorDown() - move the cursor r rows down.
*/
void LcdCtrlMoveCursorDown(void)
{
	int r = ctrlseq_p[0];
	if ( r < 0 )				r = 1;

	row = row + r;
	if ( row >= lcd_nrows )		row = lcd_nrows-1;

	lcd.setCursor(col, row);
}

/* LcdCtrlMoveCursorRight() - move the cursor c colums right.
*/
void LcdCtrlMoveCursorRight(void)
{
	int c = ctrlseq_p[0];
	if ( c < 0 )				c = 1;

	col = col + c;
	if ( col > lcd_ncols )		col = lcd_ncols;

	lcd.setCursor(col, row);
}

/* LcdCtrlMoveCursorLeft() - move the cursor c colums left.
*/
void LcdCtrlMoveCursorLeft(void)
{
	int c = ctrlseq_p[0];
	if ( c < 0 )		c = 1;

	col = col - c;
	if ( col < 0 )		col = 0;

	lcd.setCursor(col, row);
}

/* LcdCtrlClearLines() - clear lines of the display
 *	ESC[0J   - clear lines above cursor (not including cursor line)
 *	ESC[1J   - clear lines below cursor (including cursor line)
 *	ESC[2J   - clear display
*/
void LcdCtrlClearLines(void)
{
	int i, start = 0, end = lcd_nrows;
	int saverow, savecol;

	saverow = row;
	savecol = col;

	if ( ctrlseq_p[0] <= 0 )
	{
		end = row;
	}
	else
	if ( ctrlseq_p[0] == 1 )
	{
		start = row;
	}
	else
	if ( ctrlseq_p[0] == 2 )
	{
		/* No change
		*/
	}
	else
	{
		return;		// Bomb out - invalid code.
	}

	for ( i = start; i < end; i++ )
	{
		LcdClearRow(i);
	}

	/* Put the cursor back where it was.
	*/
	row = saverow;
	col = savecol;
	lcd.setCursor(col, row);
}

/* LcdCtrlClearCharacters() - clear characters from the current line.
 *	ESC[0K   - clear to right of cursor (including cursor column)
 *	ESC[1K   - clear to left of cursor (not including cursor column. Xterm might be different)
 *	ESC[2K   - clear line
*/
void LcdCtrlClearCharacters(void)
{
	int i, count;
	int saverow, savecol;

	saverow = row;
	savecol = col;

	if ( ctrlseq_p[0] <= 0 )
	{
		/* Clear to EOL. Cursor already correct.
		*/
		count = lcd_ncols - col;
	}
	else
	if ( ctrlseq_p[0] == 1 )
	{
		/* Clear from start of line.
		*/
		count = col;
		col = 0;
		lcd.setCursor(col, row);
	}
	else
	if ( ctrlseq_p[0] == 2 )
	{
		/* Clear entire line.
		*/
		count = lcd_ncols;
		col = 0;
		lcd.setCursor(col, row);
	}
	else
	{
		return;		// Bomb out - invalid code.
	}

	for ( i = 0; i < count; i++ )
	{
		LcdPutc(' ');
	}

	/* Put the cursor back where it was.
	*/
	row = saverow;
	col = savecol;
	lcd.setCursor(col, row);
}
