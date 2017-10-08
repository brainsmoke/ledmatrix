
#include <SPI.h>
#include <DMAChannel.h>

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
	0x80,0x01,0x55,0x55, 0x55,0x55,0x55,0x55, 0x55,0x55,0x55,0x55, 0x55,0x55,0x01,
	0x7f,0x03,0xaa,0xaa, 0xaa,0xaa,0xaa,0xaa, 0xaa,0xaa,0xaa,0xaa, 0xaa,0xaa,0xfe,
	0x80,0x07,0x55,0x55, 0x55,0x55,0x55,0x55, 0x55,0x55,0x55,0x55, 0x55,0x55,0x01,
	0x7f,0x0f,0xaa,0xaa, 0xaa,0xaa,0xaa,0xaa, 0xaa,0xaa,0xaa,0xaa, 0xaa,0xaa,0xfe,

	0x80,0x1f,0x55,0x55, 0x55,0x55,0x55,0x55, 0x55,0x55,0x55,0x55, 0x55,0x55,0x01,
	0x7f,0x3f,0xaa,0xaa, 0xaa,0xaa,0xaa,0xaa, 0xaa,0xaa,0xaa,0xaa, 0xaa,0xaa,0xfe,
	0x80,0x7f,0x55,0x55, 0x55,0x55,0x55,0x55, 0x55,0x55,0x55,0x55, 0x55,0x55,0x01,
	0x7f,0xff,0xaa,0xaa, 0xaa,0xaa,0xaa,0xaa, 0xaa,0xaa,0xaa,0xaa, 0xaa,0xaa,0xfe,

};

uint32_t tx_buf0[LINE_BYTE_WIDTH];
uint32_t tx_buf1[LINE_BYTE_WIDTH];

static DMAChannel spi_dma;

/* https://github.com/osresearch/vst/blob/master/teensyv/teensyv.ino */
void spi_dma_setup(void)
{
	spi_dma.disable();
	spi_dma.destination( (volatile uint32_t &) SPI0_PUSHR);
	spi_dma.disableOnCompletion();
	spi_dma.triggerAtHardwareEvent(DMAMUX_SOURCE_SPI0_TX);
	spi_dma.sourceBuffer((uint32_t*)tx_buf0, LINE_BYTE_WIDTH*4);

	SPI.begin();
	SPI.setClockDivider(SPI_CLOCK_DIV2);
	SPI.beginTransaction(SPISettings(2400000, MSBFIRST, SPI_MODE0));

	attachInterruptVector(IRQ_SPI0, next_line_isr);
	NVIC_ENABLE_IRQ(IRQ_SPI0);

	SPI0_RSER = SPI_RSER_EOQF_RE | /* SPI_RSER_RFDF_RE | SPI_RSER_RFDF_DIRS |*/ SPI_RSER_TFFF_RE | SPI_RSER_TFFF_DIRS;
	SPI0_SR = 0xFF0F0000; /* yolo-paste */
}

int line;

void prepare_line(int line)
{
	uint8_t *src = &frame[line*LINE_BYTE_WIDTH];
	uint32_t *dest = (line&1)?tx_buf1:tx_buf0;
	int i;
	for(i=0; i<LINE_BYTE_WIDTH-1; i++)
	{
		dest[i] = src[i] | SPI_PUSHR_CONT;
	}
	dest[LINE_BYTE_WIDTH-1] = src[LINE_BYTE_WIDTH-1] | SPI_PUSHR_EOQ;
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

void next_line_isr(void)
{
	spi_dma.TCD->SADDR = (line&1)?tx_buf1:tx_buf0;
	activate_line(line);
	SPI0_SR = 0xFF0F0000; /* yolo-paste */
	spi_dma.enable();

	line++;
	line &= 7;
	prepare_line(line);
	digitalWrite(LATCH_PIN, HIGH);
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
	Serial.begin(9600);
}

void loop()
{
	line = 0;
	prepare_line(line);
	next_line_isr();
	for(;;);
}

