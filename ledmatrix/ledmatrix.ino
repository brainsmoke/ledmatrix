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
#include <DMAChannel.h>
#include <usb_dev.h>

#define SPI_WRITE_BITS (8) /* changing this requires changing code */
#define MATRIX_WIDTH (15*8)
#define LINE_WRITE_COUNT ( (MATRIX_WIDTH+SPI_WRITE_BITS-1)/SPI_WRITE_BITS )
#define MATRIX_HEIGHT (8) /* logical height */

#define MATRIX_SIZE (MATRIX_WIDTH*MATRIX_HEIGHT)
#define MATRIX_BUFSIZE ( MATRIX_SIZE + LINE_WRITE_COUNT*SPI_WRITE_BITS-MATRIX_WIDTH )

/* PORTC [0-7] */
#define NOT_ENABLE_PIN (15)        /* U5 Blue   */
#define ADDR0_PIN (22)             /* U5 White  */
#define ADDR1_PIN (23)             /* U5 Orange */
#define ADDR2_PIN (9)              /* U5 Purple */
#define NOT_595_OUTPUT_ENABLE (10) /* U6 Blue   */
#define SHIFT_CLOCK (13)           /* U6 Green  */
#define SHIFT_DATA  (11)           /* U6 Yellow */
#define LATCH_PIN (12)             /* U6 Purple */

uint8_t rx_buf0[MATRIX_BUFSIZE];
uint8_t rx_buf1[MATRIX_BUFSIZE];
uint8_t rx_buf2[MATRIX_BUFSIZE];
uint8_t td_buf [MATRIX_BUFSIZE];
uint8_t *rx_draw, *rx_cur, *rx_next, *rx_p, *td_p;
uint32_t line;

uint32_t tx_buf0[LINE_WRITE_COUNT];
uint32_t tx_buf1[LINE_WRITE_COUNT];

/*

def calc_gamma(gamma=2.5, cut_off=0x2, inv=False):
    _max = 0x80
    factor = _max / (float(0x7f)**gamma)
    gamma_values = [ (x**gamma * factor) for x in range(0x80) ]
    for i,v in enumerate(gamma_values):
        if v < cut_off/2.:
            gamma_values[i] = 0
        elif v < cut_off:
            gamma_values[i] = cut_off
        if v > _max - cut_off/2.:
            gamma_values[i] = _max
        elif v > _max - cut_off:
            gamma_values[i] = _max-cut_off;
        if inv:
            gamma_values[i] = int(128-gamma_values[i])
        else:
            gamma_values[i] = int(gamma_values[i])
    return gamma_values

calc_gamma(inv=True)

 */

uint8_t gamma_map[0x80] = {
128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 128, 126, 126, 126, 126, 126, 126, 125, 125, 125, 125, 124, 124, 124, 123, 123, 123, 122, 122, 122, 121, 121, 120, 120, 119, 119, 118, 118, 117, 117, 116, 116, 115, 114, 114, 113, 112, 112, 111, 110, 109, 109, 108, 107, 106, 105, 104, 104, 103, 102, 101, 100, 99, 98, 97, 95, 94, 93, 92, 91, 90, 88, 87, 86, 85, 83, 82, 81, 79, 78, 76, 75, 73, 72, 70, 69, 67, 66, 64, 62, 61, 59, 57, 55, 54, 52, 50, 48, 46, 44, 42, 40, 38, 36, 34, 32, 30, 28, 25, 23, 21, 19, 16, 14, 12, 9, 7, 4, 2, 0
};

static DMAChannel spi_dma;

/* https://github.com/osresearch/vst/blob/master/teensyv/teensyv.ino */
void spi_dma_setup(void)
{
	spi_dma.disable();
	spi_dma.destination( (volatile uint32_t &) SPI0_PUSHR);
	spi_dma.disableOnCompletion();
	spi_dma.triggerAtHardwareEvent(DMAMUX_SOURCE_SPI0_TX);
	spi_dma.sourceBuffer((uint32_t*)tx_buf0, LINE_WRITE_COUNT*4);

	SPI.begin();
	SPI.setClockDivider(SPI_CLOCK_DIV2);
	SPI.beginTransaction(SPISettings(4000000, MSBFIRST, SPI_MODE0));

	attachInterruptVector(IRQ_SPI0, next_line_isr);
	NVIC_ENABLE_IRQ(IRQ_SPI0);

	SPI0_RSER = SPI_RSER_EOQF_RE | /* SPI_RSER_RFDF_RE | SPI_RSER_RFDF_DIRS |*/ SPI_RSER_TFFF_RE | SPI_RSER_TFFF_DIRS;
	SPI0_SR = 0xFF0F0000; /* yolo-paste */
}

void prepare_line()
{
	uint32_t *dest = (line&1)?tx_buf1:tx_buf0;
	uint32_t v,o,off=0;
	for(off=0; off<LINE_WRITE_COUNT*SPI_WRITE_BITS; off+=SPI_WRITE_BITS)
	{
		uint8_t *src_p = &rx_p[off];
		uint8_t *dith_p = &td_p[off];

		o = SPI_PUSHR_CONT;

		v = src_p[0] + dith_p[0];
		if (v > 0x7f) o |= 0x80;
		v &= 0x7f;
		dith_p[0] = v;

		v = src_p[1] + dith_p[1];
		if (v > 0x7f) o |= 0x40;
		v &= 0x7f;
		dith_p[1] = v;

		v = src_p[2] + dith_p[2];
		if (v > 0x7f) o |= 0x20;
		v &= 0x7f;
		dith_p[2] = v;

		v = src_p[3] + dith_p[3];
		if (v > 0x7f) o |= 0x10;
		v &= 0x7f;
		dith_p[3] = v;

		v = src_p[4] + dith_p[4];
		if (v > 0x7f) o |= 0x08;
		v &= 0x7f;
		dith_p[4] = v;

		v = src_p[5] + dith_p[5];
		if (v > 0x7f) o |= 0x04;
		v &= 0x7f;
		dith_p[5] = v;

		v = src_p[6] + dith_p[6];
		if (v > 0x7f) o |= 0x02;
		v &= 0x7f;
		dith_p[6] = v;

		v = src_p[7] + dith_p[7];
		if (v > 0x7f) o |= 0x01;
		v &= 0x7f;
		dith_p[7] = v;

		*dest++ = o;
	}
	dest--;
	*dest ^= SPI_PUSHR_CONT | SPI_PUSHR_EOQ;
}

void activate_line(int line)
{
	digitalWriteFast(NOT_ENABLE_PIN, HIGH);
	digitalWriteFast(ADDR0_PIN, (line & 1) );
	digitalWriteFast(ADDR1_PIN, (line & 2) );
	digitalWriteFast(ADDR2_PIN, (line & 4) );
	digitalWriteFast(LATCH_PIN, HIGH);
	digitalWriteFast(NOT_ENABLE_PIN, LOW);
	digitalWriteFast(LATCH_PIN, LOW);
}

void next_line_isr(void)
{
	spi_dma.TCD->SADDR = (line&1)?tx_buf1:tx_buf0;
	activate_line(line);
	SPI0_SR = 0xFF0F0000; /* yolo-paste */
	spi_dma.enable();

	line++;
	line &= 7;

	rx_p += MATRIX_WIDTH;
	td_p += MATRIX_WIDTH;

	if (line == 7)
	{
		rx_p = rx_draw;
		td_p = td_buf;
	}
	prepare_line();
	//digitalWrite(LATCH_PIN, HIGH); /* debug to measure how long this takes */
}

void setup(void)
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

	spi_dma_setup();
	CORE_PIN12_CONFIG = 0; /* don't mux latch pin to spi in */

	pinMode(LATCH_PIN, OUTPUT);
	usb_init();

	int i;
	for (i=1; i<MATRIX_BUFSIZE; i++)
		td_buf[i] = (153+td_buf[i-1])&0x7f;

	memset(rx_buf0, 0x80, MATRIX_BUFSIZE);
	memset(rx_buf1, 0x80, MATRIX_BUFSIZE);
	memset(rx_buf2, 0x80, MATRIX_BUFSIZE);

	rx_draw = rx_buf0;
	rx_cur  = rx_buf1;
	rx_next = rx_buf2;
	line = 0;

	rx_p = &rx_draw[MATRIX_WIDTH];
	td_p = &td_buf[MATRIX_WIDTH];

	prepare_line();
}

void swap()
{
	uint8_t *tmp = rx_draw;
	rx_draw = rx_cur;
	rx_cur = rx_next;
	rx_next = tmp;
}

void loop()
{
	next_line_isr();

	usb_packet_t *rx_packet;

	uint32_t buf_ix = 0;

	for(;;)
	{
		while ( ! (rx_packet = usb_rx(CDC_RX_ENDPOINT) ) );

		for (int i=rx_packet->index; i<rx_packet->len; i++)
		{
			int v = rx_packet->buf[i];
			if (buf_ix < MATRIX_SIZE)
				rx_cur[buf_ix++] = gamma_map[v&0x7f];
			if (v&0x80)
			{
				buf_ix = 0;
				swap();
			}
		}

		usb_free(rx_packet);
	}
}

