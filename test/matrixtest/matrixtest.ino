/*
 * Copyright (c) 2017 Erik Bosman <erik@minemu.org>
 *
 * Permission  is  hereby  granted,  free  of  charge,  to  any  person
 * obtaining  a copy  of  this  software  and  associated documentation
 * files (the "Software"),  to deal in the Software without restriction,
 * including  without  limitation  the  rights  to  use,  copy,  modify,
 * merge, publish, distribute, sublicense, and/or sell copies of the
 * Software,  and to permit persons to whom the Software is furnished to
 * do so, subject to the following conditions:
 *
 * The  above  copyright  notice  and this  permission  notice  shall be
 * included  in  all  copies  or  substantial portions  of the Software.
 *
 * THE SOFTWARE  IS  PROVIDED  "AS IS", WITHOUT WARRANTY  OF ANY KIND,
 * EXPRESS OR IMPLIED,  INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY,  FITNESS  FOR  A  PARTICULAR  PURPOSE  AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM,  DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT,  TORT OR OTHERWISE,  ARISING FROM, OUT OF OR IN
 * CONNECTION  WITH THE SOFTWARE  OR THE USE  OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * (http://opensource.org/licenses/mit-license.html)
 */

#include <SPI.h>

#define MATRIX_WIDTH (15*8)
#define LINE_BYTE_WIDTH ( (MATRIX_WIDTH+7)/8 )
#define MATRIX_HEIGHT (8) /* logical height */

/* PORTC [0-7] */
#define NOT_ENABLE_PIN (15)        /* U5 Blue   */
#define ADDR0_PIN (22)             /* U5 White  */
#define ADDR1_PIN (23)             /* U5 Orange */
#define ADDR2_PIN (9)              /* U5 Purple */
#define NOT_595_OUTPUT_ENABLE (10) /* U6 Blue   */
#define SHIFT_CLOCK (13)           /* U6 Green  */
#define SHIFT_DATA  (11)           /* U6 Yellow */
#define LATCH_PIN (12)             /* U6 Purple */


uint8_t frame[LINE_BYTE_WIDTH*MATRIX_HEIGHT] =
{
	0x80,0x55,0x55,0x55, 0x55,0x55,0x55,0x55, 0x55,0x55,0x55,0x55, 0x55,0x55,0x01,
	0x7f,0xaa,0xaa,0xaa, 0xaa,0xaa,0xaa,0xaa, 0xaa,0xaa,0xaa,0xaa, 0xaa,0xaa,0xfe,
	0x80,0x55,0x55,0x55, 0x55,0x55,0x55,0x55, 0x55,0x55,0x55,0x55, 0x55,0x55,0x01,
	0x7f,0xaa,0xaa,0xaa, 0xaa,0xaa,0xaa,0xaa, 0xaa,0xaa,0xaa,0xaa, 0xaa,0xaa,0xfe,

	0x80,0x55,0x55,0x55, 0x55,0x55,0x55,0x55, 0x55,0x55,0x55,0x55, 0x55,0x55,0x01,
	0x7f,0xaa,0xaa,0xaa, 0xaa,0xaa,0xaa,0xaa, 0xaa,0xaa,0xaa,0xaa, 0xaa,0xaa,0xfe,
	0x80,0x55,0x55,0x55, 0x55,0x55,0x55,0x55, 0x55,0x55,0x55,0x55, 0x55,0x55,0x01,
	0x7f,0xaa,0xaa,0xaa, 0xaa,0xaa,0xaa,0xaa, 0xaa,0xaa,0xaa,0xaa, 0xaa,0xaa,0xfe,

};

void setup()
{
	pinMode(NOT_ENABLE_PIN, OUTPUT);
	digitalWrite(NOT_ENABLE_PIN, HIGH);

	pinMode(ADDR0_PIN, OUTPUT);
	pinMode(ADDR1_PIN, OUTPUT);
	pinMode(ADDR2_PIN, OUTPUT);
	pinMode(NOT_595_OUTPUT_ENABLE, OUTPUT);
	pinMode(LATCH_PIN, OUTPUT);

	pinMode(SHIFT_CLOCK, OUTPUT);
	pinMode(SHIFT_DATA, OUTPUT);

	digitalWrite(ADDR0_PIN, LOW);
	digitalWrite(ADDR1_PIN, LOW);
	digitalWrite(ADDR2_PIN, LOW);
	digitalWrite(NOT_595_OUTPUT_ENABLE, LOW);
	digitalWrite(LATCH_PIN, LOW);

	SPI.begin();
	SPI.beginTransaction(SPISettings(2000000, MSBFIRST, SPI_MODE0));
	CORE_PIN12_CONFIG = 0; /* don't mux latch pin to spi in */
	pinMode(LATCH_PIN, OUTPUT);
	Serial.begin(9600);
}

void write_line(uint8_t *line, int length)
{
	SPI.transfer(line, NULL, length);
}

void activate_line(int line)
{
	digitalWrite(NOT_ENABLE_PIN, HIGH);
	digitalWrite(LATCH_PIN, HIGH);
	digitalWrite(ADDR0_PIN, (line & 1) );
	digitalWrite(ADDR1_PIN, (line & 2) );
	digitalWrite(ADDR2_PIN, (line & 4) );
	digitalWrite(LATCH_PIN, LOW);
	digitalWrite(NOT_ENABLE_PIN, LOW);
}

void loop()
{
	for(;;)
	{
		int line;
		uint8_t *p=frame;
		for (line = 0; line < 8; line++)
		{
			write_line(p, LINE_BYTE_WIDTH);
			activate_line(line);
			p += LINE_BYTE_WIDTH;
		}
	}
}
