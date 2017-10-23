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

#define SEGMENT_WIDTH (8*6)                   /* bits */
#define SEGMENT_DATA_WIDTH (SEGMENT_WIDTH+8)  /* bits, includes address line */
#define N_SEGMENTS (3*3)

#define MATRIX_WIDTH (SEGMENT_WIDTH*N_SEGMENTS)
#define MATRIX_DATA_WIDTH (SEGMENT_DATA_WIDTH*N_SEGMENTS)

#define SPI_WRITE_BITS (8) /* changing this requires changing code */
#define LINE_WRITE_COUNT ( (MATRIX_DATA_WIDTH+SPI_WRITE_BITS-1)/SPI_WRITE_BITS )
#define MATRIX_HEIGHT (8) /* logical height */

#define MATRIX_SIZE (MATRIX_WIDTH*MATRIX_HEIGHT)
#define MATRIX_BUFSIZE ( MATRIX_SIZE + LINE_WRITE_COUNT*SPI_WRITE_BITS-MATRIX_WIDTH )

/* PORTC [0-7] */
#define NOT_ENABLE_PIN (15)        /* C0 */
#define SHIFT_CLOCK (13)           /* C5 */
#define SHIFT_DATA  (11)           /* C6 */
#define LATCH_PIN (12)             /* C7 */

uint8_t rx_buf0[MATRIX_BUFSIZE];
uint8_t rx_buf1[MATRIX_BUFSIZE];
uint8_t rx_buf2[MATRIX_BUFSIZE];
uint8_t td_buf [MATRIX_BUFSIZE];
uint8_t *rx_draw, *rx_cur, *rx_next, *rx_p, *td_p;
uint32_t line, line_demux;

uint32_t tx_buf0[LINE_WRITE_COUNT];
uint32_t tx_buf1[LINE_WRITE_COUNT];

/*

def calc_gamma(gamma=2.5, cut_off=0x8, inv=False):
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

calc_gamma()

 */

uint8_t gamma_map[0x80] = {
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 9, 9, 10, 10, 11, 11, 12, 13, 13, 14, 15, 15, 16, 17, 18, 18, 19, 20, 21, 22, 23, 23, 24, 25, 26, 27, 28, 29, 30, 32, 33, 34, 35, 36, 37, 39, 40, 41, 42, 44, 45, 46, 48, 49, 51, 52, 54, 55, 57, 58, 60, 61, 63, 65, 66, 68, 70, 72, 73, 75, 77, 79, 81, 83, 85, 87, 89, 91, 93, 95, 97, 99, 102, 104, 106, 108, 111, 113, 115, 118, 120, 120, 128, 128
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

void prepare_line(uint32_t *dest)
{
	uint32_t v,o,off=0,seg=0;
	for(off=0; off<MATRIX_WIDTH; off+=8)
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

		seg++;
		if (seg == 6)
		{
			seg = 0;
			*dest++ = line_demux | SPI_PUSHR_CONT;
		}
	}
	dest--;
	*dest ^= SPI_PUSHR_CONT | SPI_PUSHR_EOQ;
}

void activate_line(int line)
{
	digitalWriteFast(LATCH_PIN, LOW);
	digitalWriteFast(NOT_ENABLE_PIN, HIGH);
	digitalWriteFast(LATCH_PIN, HIGH);
	digitalWriteFast(NOT_ENABLE_PIN, LOW);
}

int c=0;
void next_line_isr(void)
{
	spi_dma.TCD->SADDR = (c&1)?tx_buf1:tx_buf0;
	activate_line(line);
	SPI0_SR = 0xFF0F0000; /* yolo-paste */
	spi_dma.enable();

	c++;
	c&=3;

	if (c==0)
	{
		line++;
		line &= 7;
		line_demux = 0xff ^ (1<<((line+1)&7));

		rx_p += MATRIX_WIDTH;
		td_p += MATRIX_WIDTH;

		if (line == 7)
		{
			rx_p = rx_draw;
			td_p = td_buf;
		}
	}
	prepare_line(c&1?tx_buf1:tx_buf0);
}

void setup(void)
{
	pinMode(NOT_ENABLE_PIN, OUTPUT);
	digitalWrite(NOT_ENABLE_PIN, HIGH);

	pinMode(LATCH_PIN, OUTPUT);

	pinMode(SHIFT_CLOCK, OUTPUT);
	pinMode(SHIFT_DATA, OUTPUT);

	digitalWrite(LATCH_PIN, LOW);

	spi_dma_setup();
	CORE_PIN12_CONFIG = 0; /* don't mux latch pin to spi in */

	pinMode(LATCH_PIN, OUTPUT);
	usb_init();

	int i;
	for (i=1; i<MATRIX_BUFSIZE; i++)
		td_buf[i] = (153+td_buf[i-1])&0x7f;

	memset(rx_buf0, 0x00, MATRIX_BUFSIZE);
	memset(rx_buf1, 0x00, MATRIX_BUFSIZE);
	memset(rx_buf2, 0x00, MATRIX_BUFSIZE);

	rx_draw = rx_buf0;
	rx_cur  = rx_buf1;
	rx_next = rx_buf2;
	line = 0;
	line_demux = 0xfe;

	rx_p = &rx_draw[MATRIX_WIDTH];
	td_p = &td_buf[MATRIX_WIDTH];

	prepare_line(tx_buf0);
}

void swap()
{
	uint8_t *tmp = rx_draw;
	rx_draw = rx_cur;
	rx_cur = rx_next;
	rx_next = tmp;
}

static usb_packet_t *rx_packet=NULL;
static int rx_i=0, fastpath=0;

static int usb_getchar(void)
{
	int c;

	if (!rx_packet)
	{
		while ( !(rx_packet = usb_rx(CDC_RX_ENDPOINT) ) || (rx_packet->index >= rx_packet->len) );
		rx_i=rx_packet->index;
		fastpath=rx_packet->len-1;
	}

	c = (uint8_t)rx_packet->buf[rx_i++];

	if (rx_i == rx_packet->len)
	{
		usb_free(rx_packet);
		rx_packet = NULL;
	}

	return c;
}

void read_frame(void)
{
	uint32_t seg_ix = 0, row_ix=0, col_ix = 0;
	int v;

	for (row_ix = 0; row_ix<8; row_ix+=1)
		for (seg_ix = row_ix; seg_ix<MATRIX_WIDTH; seg_ix+=8)
			for (col_ix = seg_ix; col_ix<MATRIX_SIZE; col_ix+=MATRIX_WIDTH)
			{
				if (rx_i<fastpath)
					v = rx_packet->buf[rx_i++];
				else
					v = usb_getchar();
				rx_cur[col_ix] = gamma_map[v&0x7f];
				if (v&0x80)
					return;
			}

	for(;;)
	{
		if (rx_i<fastpath)
			v = rx_packet->buf[rx_i++];
		else
			v = usb_getchar();
		if (v&0x80)
			return;
	}
}

void loop()
{
	next_line_isr();

	for(;;)
	{
		read_frame();
		swap();
	}
}

