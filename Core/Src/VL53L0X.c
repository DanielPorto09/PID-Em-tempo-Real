// Most of the functionality of this library is based on the VL53L0X API
// provided by ST (STSW-IMG005), and some of the explanatory comments are quoted
// or paraphrased from the API source code, API user manual (UM2039), and the
// VL53L0X datasheet.

#include "VL53L0X.h"


#include <stdbool.h>
#include <stdint.h>

/*
 * ARMv7
 */
__attribute__((always_inline)) static inline void __disable_irq(void) {
	__asm volatile ("cpsid i" : : : "memory");
}

__attribute__((always_inline)) static inline void __enable_irq(void) {
	__asm volatile ("cpsie i" : : : "memory");
}

/*
 * Cortex-M3
 */
// Nested vectored interrupt controller (NVIC)
struct nvic {
	volatile uint32_t iser[1]; // Interrupt set enable register
	uint32_t reserved0[31];
	volatile uint32_t icer[1]; // Interrupt clear enable register
	uint32_t reserved1[31];
	volatile uint32_t ispr[1]; // Interrupt set pending register
	uint32_t reserved2[31];
	volatile uint32_t icpr[1]; // Interrupt clear pending register
	uint32_t reserved3[31];
	uint32_t reserved4[64];
	volatile uint32_t ip[1]; // Interrupt priority
};

#define NVIC ((struct nvic*) 0xE000E100)

typedef enum {
	IRQN_PENDSV = -2,
	IRQN_SYSTICK = -1,
	IRQN_EXTI9_5 = 23,
} IRQN;

#define NVIC_PRIO_BITS 4

void nvic_enable_irq(IRQN irqn);
void nvic_set_priority(IRQN irqn, uint32_t priority);

// System control block (SCB)
struct scb {
	volatile uint32_t cpuid; // CPUID Base Register
	volatile uint32_t icsr; // Interrupt Control and State Register
	volatile uint32_t vtor; // Vector Table Offset Register
	volatile uint32_t aircr; // Application Interrupt and Reset Control Register
	volatile uint32_t scr; // System Control Register
	volatile uint32_t ccr; // Configuration Control Register
	volatile uint8_t shp[12]; // System Handlers Priority Registers
	volatile uint32_t shcsr; // System Handler Control and State Register
	volatile uint32_t cfsr; // Configurable Fault Status Register
};

#define SCB ((struct scb*) 0xE000ED00)

#define SCB_ICSR_PENDSVSET (1 << 28)

// SysTick
struct systick {
	volatile uint32_t csr; // Control and Status Register
	volatile uint32_t rvr; // Reload Value Register
	volatile uint32_t cvr; // Current Value Register
	volatile uint32_t calib; // Calibration Register
};

#define SYSTICK ((struct systick*) 0xE000E010)

#define	SYSTICK_CSR_ENABLE (1 << 0)
#define	SYSTICK_CSR_TICKINT (1 << 1)
#define	SYSTICK_CSR_CLKSOURCE (1 << 2)

extern void systick_handler(void);
void systick_init(uint32_t ticks);

/*
 * STM32F103
 */
// Flash interface registers (FLASH)
struct flash {
	volatile uint32_t acr; // Access control register
	volatile uint32_t keyr; // Key register
	volatile uint32_t optkeyr; // Option key register
	volatile uint32_t sr; // Status register
	volatile uint32_t cr; // Control register
	volatile uint32_t ar; // Address register
	uint32_t reserved0;
	volatile uint32_t obr; // Option byte register
	volatile uint32_t wrpr; // Write protection register
};

#define FLASH ((struct flash*) 0x40022000)

#define FLASH_ACR_LATENCY(x) ((x) << 0)
#define FLASH_ACR_PRFTBE (1 << 4)

// Reset and Clock Control (RCC)
struct rcc {
	volatile uint32_t cr; // Clock control register
	volatile uint32_t cfgr; // Clock configuration register
	volatile uint32_t cir; // Clock interrupt register
	volatile uint32_t apb2rstr; // APB2 peripheral reset register
	volatile uint32_t apb1rstr; // APB1 peripheral reset register
	volatile uint32_t ahbenr; // AHB peripheral clock enable register
	volatile uint32_t apb2enr; // APB2 peripheral clock enable register
	volatile uint32_t apb1enr; // APB1 peripheral clock enable register
	volatile uint32_t bdcr; // Backup domain control register
	volatile uint32_t csr; // Control/status register
};

#define RCC ((struct rcc*) 0x40021000)

#define RCC_CR_HSION (1 << 0)
#define RCC_CR_HSITRIM(x) ((x) << 3)
#define RCC_CR_HSEON (1 << 16)
#define RCC_CR_PLLON (1 << 24)
#define RCC_CR_PLLRDY (1 << 25)

#define RCC_CFGR_SW(x) ((x) << 0)
#define RCC_CFGR_PPRE1(x) ((x) << 8)
#define RCC_CFGR_PLLSRC (1 << 16)
#define RCC_CFGR_PLLMULL(x) ((x) << 18)

#define RCC_APB1RSTR_I2C1RST (1 << 21)

#define RCC_APB2ENR_AFIOEN (1 << 0)
#define RCC_APB2ENR_IOPAEN (1 << 2)
#define RCC_APB2ENR_IOPBEN (1 << 3)
#define RCC_APB2ENR_IOPCEN (1 << 4)
#define RCC_APB2ENR_IOPEEN (1 << 6)
#define RCC_APB2ENR_USART1EN (1 << 14)

#define RCC_APB1ENR_TIM2EN (1 << 0)
#define RCC_APB1ENR_I2C1EN (1 << 21)

void rcc_init(void);
uint32_t rcc_get_clock(void);

// External interrupt event controller (EXTI)
struct exti {
	volatile uint32_t imr; // Interrupt mask register
	volatile uint32_t emr; // Event mask register
	volatile uint32_t rtsr; // Rising trigger selection register
	volatile uint32_t ftsr; // Falling trigger selection register
	volatile uint32_t swier; // Software interrupt event register
	volatile uint32_t pr; // Pending register
};

#define EXTI ((struct exti*) 0x40010400)

#define EXTI_TRIGGER_RISING (1 << 0)
#define EXTI_TRIGGER_FALLING (1 << 1)

void exti_enable(uint8_t line);
void exti_configure(uint8_t line, uint8_t trigger);
void exti_clear_pending(uint8_t line);

// General purpose input output (GPIO)
struct gpio {
	volatile uint32_t cr[2]; // Port configuration register
	volatile uint32_t idr; // Port input data register
	volatile uint32_t odr; // Port output data register
	volatile uint32_t bsrr; // Port bit set/reset register
	volatile uint32_t brr; // Port bit reset register
	volatile uint32_t lckr; // Port configuration lock register
};

#define GPIOA ((struct gpio*) 0x40010800)
#define GPIOB ((struct gpio*) 0x40010C00)
#define GPIOC ((struct gpio*) 0x40011000)

#define GPIO_CR_MODE_INPUT 0b00
#define GPIO_CR_MODE_OUTPUT_10M 0b01
#define GPIO_CR_MODE_OUTPUT_2M 0b10
#define GPIO_CR_MODE_OUTPUT_50M 0b11

#define GPIO_CR_CNF_INPUT_ANALOG 0b00
#define GPIO_CR_CNF_INPUT_FLOATING 0b01
#define GPIO_CR_CNF_INPUT_PUPD 0b10
#define GPIO_CR_CNF_OUTPUT_PUSH_PULL 0b00
#define GPIO_CR_CNF_OUTPUT_OPEN_DRAIN 0b01
#define GPIO_CR_CNF_OUTPUT_ALT_PUSH_PULL 0b10
#define GPIO_CR_CNF_OUTPUT_ALT_OPEN_DRAIN 0b11

void gpio_init(struct gpio* gpio);
void gpio_configure(struct gpio* gpio, uint8_t pin, uint8_t mode, uint8_t cnf);
void gpio_write(struct gpio* gpio, uint8_t pin, bool value);
bool gpio_read(struct gpio* gpio, uint8_t pin);

// I2C
struct i2c {
	volatile uint32_t cr1; // Control register 1
	volatile uint32_t cr2; // Control register 2
	volatile uint32_t oar1;
	volatile uint32_t oar2;
	volatile uint32_t dr;
	volatile uint32_t sr1;
	volatile uint32_t sr2;
	volatile uint32_t ccr;
	volatile uint32_t trise;
};

#define I2C1 ((struct i2c*) 0x40005400)

#define I2C_CR1_PE (1 << 0)
#define I2C_CR1_START (1 << 8)
#define I2C_CR1_STOP (1 << 9)
#define I2C_CR1_ACK (1 << 10)
#define I2C_CR1_POS (1 << 11)

#define I2C_CR2_FREQ(x) ((x) & 0b111111)

#define I2C_SR1_SB (1 << 0)
#define I2C_SR1_ADDR (1 << 1)
#define I2C_SR1_BTF (1 << 2)
#define I2C_SR1_RXNE (1 << 6)
#define I2C_SR1_TXE (1 << 7)
#define I2C_SR1_BERR (1 << 8)
#define I2C_SR1_ARLO (1 << 9)
#define I2C_SR1_AF (1 << 10)
#define I2C_SR1_OVR (1 << 11)
#define I2C_SR1_PECERR (1 << 12)
#define I2C_SR1_TIMEOUT (1 << 14)

void i2c_init(struct i2c* i2c);

void i2c_read(struct i2c* i2c, uint8_t slave_address, uint8_t* data, uint8_t size);
void i2c_write(struct i2c* i2c, uint8_t slave_address, uint8_t* data, uint8_t size);

// Timer
struct timer {
	volatile uint32_t cr1; // Control register 1
	volatile uint32_t cr2; // Control register 2
	volatile uint32_t smcr; // Slave mode control register
	volatile uint32_t dier; // DMA/interrupt enable register
	volatile uint32_t sr; // Status register
	volatile uint32_t egr; // Event generation register
	volatile uint32_t ccmr1; // Capture/compare mode register 1
	volatile uint32_t ccmr2; // Capture/compare mode register 2
	volatile uint32_t ccer; // Capture/compare enable register
	volatile uint32_t cnt; // Counter register
	volatile uint32_t psc; // Prescaler register
	volatile uint32_t arr; // Auto-reload register
	volatile uint32_t rcr; // Repetition counter register
	volatile uint32_t ccr1; // Capture/compare register 1
	volatile uint32_t ccr2; // Capture/compare register 2
	volatile uint32_t ccr3; // Capture/compare register 3
	volatile uint32_t ccr4; // Capture/compare register 4
	volatile uint32_t bdtr; // Break and dead-time register
	volatile uint32_t dcr; // DMA control register
	volatile uint32_t dmar; // DMA address for full transfer register
	volatile uint32_t or; // Option register
};

#define TIMER2 ((struct timer*) 0x40000000)

#define TIMER_CR1_CEN (1 << 0)

#define TIMER_CCMR1_OC2M_1 (2 << 12)
#define TIMER_CCMR1_OC2M_2 (4 << 12)

#define TIMER_CCER_CC2E (1 << 4)

void timer_init(struct timer* timer);

// Universal synchronous asynchronous receiver transmitter (USART)
struct usart {
	volatile uint32_t sr; // Status register
	volatile uint32_t dr; // Data register
	volatile uint32_t brr; // Baud rate register
	volatile uint32_t cr1; // Control register 1
	volatile uint32_t cr2; // Control register 2
	volatile uint32_t cr3; // Control register 3
	volatile uint32_t gtpr; // Guard time and prescaler
};

#define USART1 ((struct usart*) 0x40013800)
#define USART2 ((struct usart*) 0x40004400)
#define USART3 ((struct usart*) 0x40004800)

#define USART_SR_RXNE (1 << 5) // Read data register not empty
#define USART_SR_TXE (1 << 7) // Transmit data register empty

#define USART_CR1_RE (1 << 2) // Receiver enable
#define USART_CR1_TE (1 << 3) // Transmitter enable
#define USART_CR1_PCE (1 << 10) // Parity control enable
#define USART_CR1_M (1 << 12) // Word length
#define USART_CR1_UE (1 << 13) // USART enable

void usart_init(struct usart* usart, uint32_t brr);
void usart_write(struct usart* usart, char c);
char usart_read(struct usart* usart);


/*
 * Cortex-M3
 */
// Nested vectored interrupt controller (NVIC)
void nvic_enable_irq(IRQN irqn) {
	if (irqn >= 0) {
		NVIC->iser[irqn >> 5] = 1 << (irqn & 0x1F);
	}
}

void nvic_set_priority(IRQN irqn, uint32_t priority) {
	if (irqn >= 0) {
	} else {
		SCB->shp[(((uint32_t) irqn) & 0xF) - 4] = (uint8_t)((priority << (8 - NVIC_PRIO_BITS)) & (uint32_t) 0xFF);
	}
}

// SysTick
void systick_init(uint32_t ticks) {
	SYSTICK->rvr = ticks - 1;
	SYSTICK->cvr = 0;
	SYSTICK->csr = SYSTICK_CSR_ENABLE | SYSTICK_CSR_TICKINT | SYSTICK_CSR_CLKSOURCE;
}

/*
 * STM32F103
 */
// Reset and clock control (RCC)
void rcc_init(void) {
	// Configure the clock to 72 MHz
	RCC->cfgr = RCC_CFGR_PLLSRC | RCC_CFGR_PLLMULL(7) | RCC_CFGR_SW(0b00) | RCC_CFGR_PPRE1(0b100);
	RCC->cr = RCC_CR_HSION | RCC_CR_HSITRIM(16) | RCC_CR_HSEON | RCC_CR_PLLON;
	while (!(RCC->cr & RCC_CR_PLLRDY));
	FLASH->acr = FLASH_ACR_LATENCY(0b010) | FLASH_ACR_PRFTBE;
	RCC->cfgr = RCC_CFGR_PLLSRC | RCC_CFGR_PLLMULL(7) | RCC_CFGR_SW(0b10) | RCC_CFGR_PPRE1(0b100);
}

uint32_t rcc_get_clock(void) {
	return 72e6;
}

// External interrupt event controller (EXTI)
void exti_enable(uint8_t line) {
	EXTI->imr |= (1 << line);
}

void exti_configure(uint8_t line, uint8_t trigger) {
	if (trigger & EXTI_TRIGGER_RISING)
		EXTI->rtsr |= (1 << line);
	else
		EXTI->rtsr &= ~(1 << line);
	if (trigger & EXTI_TRIGGER_FALLING)
		EXTI->ftsr |= (1 << line);
	else
		EXTI->ftsr &= ~(1 << line);
}

void exti_clear_pending(uint8_t line) {
	EXTI->pr |= (1 << line);
}

// General purpose input output (GPIO)
void gpio_init(struct gpio* gpio) {
	switch ((uint32_t) gpio) {
		case (uint32_t) GPIOA: RCC->apb2enr |= RCC_APB2ENR_IOPAEN; break;
		case (uint32_t) GPIOB: RCC->apb2enr |= RCC_APB2ENR_IOPBEN; break;
		case (uint32_t) GPIOC: RCC->apb2enr |= RCC_APB2ENR_IOPCEN; break;
	}
}

void gpio_configure(struct gpio* gpio, uint8_t pin, uint8_t mode, uint8_t cnf) {
	uint8_t reg = pin / 8;
	uint8_t base = (pin % 8) * 4;
	gpio->cr[reg] = (gpio->cr[reg] & ~(0b1111 << base)) | (mode << base) | (cnf << base << 2);
}

void gpio_write(struct gpio* gpio, uint8_t pin, bool value) {
	if (value)
		gpio->brr |= 1 << pin;
	else
		gpio->bsrr |= 1 << pin;
}

bool gpio_read(struct gpio* gpio, uint8_t pin) {
	return (gpio->idr >> pin) != 0;
}

// I2C
void i2c_init(struct i2c* i2c) {
	switch ((uint32_t) i2c) {
		case (uint32_t) I2C1:
			RCC->apb1enr |= RCC_APB1ENR_I2C1EN;
			RCC->apb2enr |= RCC_APB2ENR_IOPBEN;
			gpio_configure(GPIOA, 15, GPIO_CR_MODE_OUTPUT_50M, GPIO_CR_CNF_OUTPUT_ALT_OPEN_DRAIN);
			gpio_configure(GPIOB, 7, GPIO_CR_MODE_OUTPUT_50M, GPIO_CR_CNF_OUTPUT_ALT_OPEN_DRAIN);
			break;
	}
	i2c->cr2 = I2C_CR2_FREQ(36);
	i2c->ccr = 180;
	i2c->trise = 37;
	i2c->cr1 |= I2C_CR1_PE;
}

void i2c_read(struct i2c* i2c, uint8_t slave_address, uint8_t* data, uint8_t size) {
	i2c->cr1 &= ~I2C_CR1_POS;
	i2c->cr1 |= I2C_CR1_START | I2C_CR1_ACK;
	while (!(i2c->sr1 & I2C_SR1_SB));
	i2c->dr = (slave_address << 1) | 1;
	while (!(i2c->sr1 & I2C_SR1_ADDR));
	i2c->sr1;
	i2c->sr2;
	uint8_t index;
	for (index = 0; index < size; index++) {
		if (index + 1 == size) {
			i2c->cr1 &= ~I2C_CR1_ACK;
			i2c->cr1 |= I2C_CR1_STOP;
		}
		while (!(i2c->sr1 & I2C_SR1_RXNE));
		data[index] = i2c->dr;
	}
}

void i2c_write(struct i2c* i2c, uint8_t slave_address, uint8_t* data, uint8_t size) {
	volatile uint16_t reg;
	i2c->cr1 &= ~I2C_CR1_POS;
	i2c->cr1 |= I2C_CR1_START;
	while (!(i2c->sr1 & I2C_SR1_SB));
	i2c->dr = (slave_address << 1) | 0;
	while (!(i2c->sr1 & I2C_SR1_ADDR));
	reg = i2c->sr1;
	reg = i2c->sr2;
	while (!(i2c->sr1 & I2C_SR1_TXE));
	for (uint8_t index = 0; index < size; index++) {
		i2c->dr = data[index];
		while (!(i2c->sr1 & I2C_SR1_TXE));
		while (!(i2c->sr1 & I2C_SR1_BTF));
		reg = i2c->sr1;
		reg = i2c->sr2;
	}
	i2c->cr1 |= I2C_CR1_STOP;
	reg = i2c->sr1;
	reg = i2c->sr2;
	(void) reg;
}

// Timer
void timer_init(struct timer* timer) {
	switch ((uint32_t) timer) {
		case (uint32_t) TIMER2: RCC->apb1enr |= RCC_APB1ENR_TIM2EN; break;
	}
}

// Universal synchronous asynchronous receiver transmitter (USART)
void usart_init(struct usart* usart, uint32_t brr) {
	switch ((uint32_t) usart) {
		case (uint32_t) USART1:
			RCC->apb2enr |= RCC_APB2ENR_IOPAEN | RCC_APB2ENR_USART1EN;
			gpio_configure(GPIOA, 9, GPIO_CR_MODE_OUTPUT_50M, GPIO_CR_CNF_OUTPUT_ALT_PUSH_PULL);
			gpio_configure(GPIOA, 10, GPIO_CR_MODE_INPUT, GPIO_CR_CNF_INPUT_FLOATING);
			break;
	}
	usart->cr1 = USART_CR1_RE | USART_CR1_TE | USART_CR1_PCE | USART_CR1_M | USART_CR1_UE;
	usart->cr2 = 0;
	usart->cr3 = 0;
	usart->gtpr = 0;
	usart->brr = brr;
}

void usart_write(struct usart* usart, char c) {
	while (!(usart->sr & USART_SR_TXE));
	usart->dr = c;
}

char usart_read(struct usart* usart) {
	while (!(usart->sr & USART_SR_RXNE));
	return usart->dr;
}

// Defines /////////////////////////////////////////////////////////////////////

// Record the current time to check an upcoming timeout against
//#define startTimeout() (timeout_start_ms = millis())

// Check if timeout is enabled (set to nonzero value) and has expired
//#define checkTimeoutExpired() (io_timeout > 0 && ((uint16_t)millis() - timeout_start_ms) > io_timeout)

// Decode VCSEL (vertical cavity surface emitting laser) pulse period in PCLKs
// from register value
// based on VL53L0X_decode_vcsel_period()
#define decodeVcselPeriod(reg_val) (((reg_val) + 1) << 1)

// Encode VCSEL pulse period register value from period in PCLKs
// based on VL53L0X_encode_vcsel_period()
#define encodeVcselPeriod(period_pclks) (((period_pclks) >> 1) - 1)

// Calculate macro period in *nanoseconds* from VCSEL period in PCLKs
// based on VL53L0X_calc_macro_period_ps()
// PLL_period_ps = 1655; macro_period_vclks = 2304
#define calcMacroPeriod(vcsel_period_pclks) ((((uint32_t)2304 * (vcsel_period_pclks) * 1655) + 500) / 1000)



void VL53L0X_setAddress(struct VL53L0X* dev, uint8_t new_addr)
{
  VL53L0X_writeReg(dev, I2C_SLAVE_DEVICE_ADDRESS, new_addr & 0x7F);
  dev->address = new_addr;
}

// Initialize sensor using sequence based on VL53L0X_DataInit(),
// VL53L0X_StaticInit(), and VL53L0X_PerformRefCalibration().
// This function does not perform reference SPAD calibration
// (VL53L0X_PerformRefSpadManagement()), since the API user manual says that it
// is performed by ST on the bare modules; it seems like that should work well
// enough unless a cover glass is added.
// If io_2v8 (optional) is true or not given, the sensor is configured for 2V8
// mode.
bool VL53L0X_init(struct VL53L0X* dev)
{
  // VL53L0X_DataInit() begin
  i2c_init(I2C1);

  // sensor uses 1V8 mode for I/O by default; switch to 2V8 mode if necessary
  if (dev->io_2v8)
  {
    VL53L0X_writeReg(dev, VHV_CONFIG_PAD_SCL_SDA__EXTSUP_HV, VL53L0X_readReg(dev, VHV_CONFIG_PAD_SCL_SDA__EXTSUP_HV) | 0x01 ); // set bit 0
  }

  // "Set I2C standard mode"
  VL53L0X_writeReg(dev, 0x88, 0x00);

  VL53L0X_writeReg(dev, 0x80, 0x01);
  VL53L0X_writeReg(dev, 0xFF, 0x01);
  VL53L0X_writeReg(dev, 0x00, 0x00);
  dev->stop_variable = VL53L0X_readReg(dev, 0x91);
  VL53L0X_writeReg(dev, 0x00, 0x01);
  VL53L0X_writeReg(dev, 0xFF, 0x00);
  VL53L0X_writeReg(dev, 0x80, 0x00);

  // disable SIGNAL_RATE_MSRC (bit 1) and SIGNAL_RATE_PRE_RANGE (bit 4) limit checks
  VL53L0X_writeReg(dev, MSRC_CONFIG_CONTROL, VL53L0X_readReg(dev,  MSRC_CONFIG_CONTROL) | 0x12);

  // set final range signal rate limit to 0.25 MCPS (million counts per second)
  VL53L0X_setSignalRateLimit(dev);

  VL53L0X_writeReg(dev, SYSTEM_SEQUENCE_CONFIG, 0xFF);

  // VL53L0X_DataInit() end

  // VL53L0X_StaticInit() begin

  uint8_t spad_count;
  bool spad_type_is_aperture;
  if (!VL53L0X_getSpadInfo(dev, &spad_count, &spad_type_is_aperture)) { return false; }

  // The SPAD map (RefGoodSpadMap) is read by VL53L0X_get_info_from_device() in
  // the API, but the same data seems to be more easily readable from
  // GLOBAL_CONFIG_SPAD_ENABLES_REF_0 through _6, so read it from there
  uint8_t ref_spad_map[6];
  VL53L0X_readMulti(dev, GLOBAL_CONFIG_SPAD_ENABLES_REF_0, ref_spad_map, 6);

  // -- VL53L0X_set_reference_spads() begin (assume NVM values are valid)

  VL53L0X_writeReg(dev, 0xFF, 0x01);
  VL53L0X_writeReg(dev, DYNAMIC_SPAD_REF_EN_START_OFFSET, 0x00);
  VL53L0X_writeReg(dev, DYNAMIC_SPAD_NUM_REQUESTED_REF_SPAD, 0x2C);
  VL53L0X_writeReg(dev, 0xFF, 0x00);
  VL53L0X_writeReg(dev, GLOBAL_CONFIG_REF_EN_START_SELECT, 0xB4);

  uint8_t first_spad_to_enable = spad_type_is_aperture ? 12 : 0; // 12 is the first aperture spad
  uint8_t spads_enabled = 0;

  for (uint8_t i = 0; i < 48; i++)
  {
    if (i < first_spad_to_enable || spads_enabled == spad_count)
    {
      // This bit is lower than the first one that should be enabled, or
      // (reference_spad_count) bits have already been enabled, so zero this bit
      ref_spad_map[i / 8] &= ~(1 << (i % 8));
    }
    else if ((ref_spad_map[i / 8] >> (i % 8)) & 0x1)
    {
      spads_enabled++;
    }
  }

  VL53L0X_writeMulti(dev, GLOBAL_CONFIG_SPAD_ENABLES_REF_0, ref_spad_map, 6);

  // -- VL53L0X_set_reference_spads() end

  // -- VL53L0X_load_tuning_settings() begin
  // DefaultTuningSettings from vl53l0x_tuning.h

  VL53L0X_writeReg(dev, 0xFF, 0x01);
  VL53L0X_writeReg(dev, 0x00, 0x00);

  VL53L0X_writeReg(dev, 0xFF, 0x00);
  VL53L0X_writeReg(dev, 0x09, 0x00);
  VL53L0X_writeReg(dev, 0x10, 0x00);
  VL53L0X_writeReg(dev, 0x11, 0x00);

  VL53L0X_writeReg(dev, 0x24, 0x01);
  VL53L0X_writeReg(dev, 0x25, 0xFF);
  VL53L0X_writeReg(dev, 0x75, 0x00);

  VL53L0X_writeReg(dev, 0xFF, 0x01);
  VL53L0X_writeReg(dev, 0x4E, 0x2C);
  VL53L0X_writeReg(dev, 0x48, 0x00);
  VL53L0X_writeReg(dev, 0x30, 0x20);

  VL53L0X_writeReg(dev, 0xFF, 0x00);
  VL53L0X_writeReg(dev, 0x30, 0x09);
  VL53L0X_writeReg(dev, 0x54, 0x00);
  VL53L0X_writeReg(dev, 0x31, 0x04);
  VL53L0X_writeReg(dev, 0x32, 0x03);
  VL53L0X_writeReg(dev, 0x40, 0x83);
  VL53L0X_writeReg(dev, 0x46, 0x25);
  VL53L0X_writeReg(dev, 0x60, 0x00);
  VL53L0X_writeReg(dev, 0x27, 0x00);
  VL53L0X_writeReg(dev, 0x50, 0x06);
  VL53L0X_writeReg(dev, 0x51, 0x00);
  VL53L0X_writeReg(dev, 0x52, 0x96);
  VL53L0X_writeReg(dev, 0x56, 0x08);
  VL53L0X_writeReg(dev, 0x57, 0x30);
  VL53L0X_writeReg(dev, 0x61, 0x00);
  VL53L0X_writeReg(dev, 0x62, 0x00);
  VL53L0X_writeReg(dev, 0x64, 0x00);
  VL53L0X_writeReg(dev, 0x65, 0x00);
  VL53L0X_writeReg(dev, 0x66, 0xA0);

  VL53L0X_writeReg(dev, 0xFF, 0x01);
  VL53L0X_writeReg(dev, 0x22, 0x32);
  VL53L0X_writeReg(dev, 0x47, 0x14);
  VL53L0X_writeReg(dev, 0x49, 0xFF);
  VL53L0X_writeReg(dev, 0x4A, 0x00);

  VL53L0X_writeReg(dev, 0xFF, 0x00);
  VL53L0X_writeReg(dev, 0x7A, 0x0A);
  VL53L0X_writeReg(dev, 0x7B, 0x00);
  VL53L0X_writeReg(dev, 0x78, 0x21);

  VL53L0X_writeReg(dev, 0xFF, 0x01);
  VL53L0X_writeReg(dev, 0x23, 0x34);
  VL53L0X_writeReg(dev, 0x42, 0x00);
  VL53L0X_writeReg(dev, 0x44, 0xFF);
  VL53L0X_writeReg(dev, 0x45, 0x26);
  VL53L0X_writeReg(dev, 0x46, 0x05);
  VL53L0X_writeReg(dev, 0x40, 0x40);
  VL53L0X_writeReg(dev, 0x0E, 0x06);
  VL53L0X_writeReg(dev, 0x20, 0x1A);
  VL53L0X_writeReg(dev, 0x43, 0x40);

  VL53L0X_writeReg(dev, 0xFF, 0x00);
  VL53L0X_writeReg(dev, 0x34, 0x03);
  VL53L0X_writeReg(dev, 0x35, 0x44);

  VL53L0X_writeReg(dev, 0xFF, 0x01);
  VL53L0X_writeReg(dev, 0x31, 0x04);
  VL53L0X_writeReg(dev, 0x4B, 0x09);
  VL53L0X_writeReg(dev, 0x4C, 0x05);
  VL53L0X_writeReg(dev, 0x4D, 0x04);

  VL53L0X_writeReg(dev, 0xFF, 0x00);
  VL53L0X_writeReg(dev, 0x44, 0x00);
  VL53L0X_writeReg(dev, 0x45, 0x20);
  VL53L0X_writeReg(dev, 0x47, 0x08);
  VL53L0X_writeReg(dev, 0x48, 0x28);
  VL53L0X_writeReg(dev, 0x67, 0x00);
  VL53L0X_writeReg(dev, 0x70, 0x04);
  VL53L0X_writeReg(dev, 0x71, 0x01);
  VL53L0X_writeReg(dev, 0x72, 0xFE);
  VL53L0X_writeReg(dev, 0x76, 0x00);
  VL53L0X_writeReg(dev, 0x77, 0x00);

  VL53L0X_writeReg(dev, 0xFF, 0x01);
  VL53L0X_writeReg(dev, 0x0D, 0x01);

  VL53L0X_writeReg(dev, 0xFF, 0x00);
  VL53L0X_writeReg(dev, 0x80, 0x01);
  VL53L0X_writeReg(dev, 0x01, 0xF8);

  VL53L0X_writeReg(dev, 0xFF, 0x01);
  VL53L0X_writeReg(dev, 0x8E, 0x01);
  VL53L0X_writeReg(dev, 0x00, 0x01);
  VL53L0X_writeReg(dev, 0xFF, 0x00);
  VL53L0X_writeReg(dev, 0x80, 0x00);

  // -- VL53L0X_load_tuning_settings() end

  // "Set interrupt config to new sample ready"
  // -- VL53L0X_SetGpioConfig() begin

  VL53L0X_writeReg(dev, SYSTEM_INTERRUPT_CONFIG_GPIO, 0x04);
  VL53L0X_writeReg(dev, GPIO_HV_MUX_ACTIVE_HIGH, VL53L0X_readReg(dev,  GPIO_HV_MUX_ACTIVE_HIGH) & ~0x10); // active low
  VL53L0X_writeReg(dev, SYSTEM_INTERRUPT_CLEAR, 0x01);

  // -- VL53L0X_SetGpioConfig() end

  dev->measurement_timing_budget_us = VL53L0X_getMeasurementTimingBudget(dev);

  // "Disable MSRC and TCC by default"
  // MSRC = Minimum Signal Rate Check
  // TCC = Target CentreCheck
  // -- VL53L0X_SetSequenceStepEnable() begin

  VL53L0X_writeReg(dev, SYSTEM_SEQUENCE_CONFIG, 0xE8);

  // -- VL53L0X_SetSequenceStepEnable() end

  // "Recalculate timing budget"
 VL53L0X_setMeasurementTimingBudget(dev, dev->measurement_timing_budget_us);

  // VL53L0X_StaticInit() end

  // VL53L0X_PerformRefCalibration() begin (VL53L0X_perform_ref_calibration())

  // -- VL53L0X_perform_vhv_calibration() begin

  VL53L0X_writeReg(dev, SYSTEM_SEQUENCE_CONFIG, 0x01);
  if (!VL53L0X_performSingleRefCalibration(dev, 0x40)) { return false; }

  // -- VL53L0X_perform_vhv_calibration() end

  // -- VL53L0X_perform_phase_calibration() begin

  VL53L0X_writeReg(dev, SYSTEM_SEQUENCE_CONFIG, 0x02);
  if (!VL53L0X_performSingleRefCalibration(dev, 0x00)) { return false; }

  // -- VL53L0X_perform_phase_calibration() end

  // "restore the previous Sequence Config"
  VL53L0X_writeReg(dev, SYSTEM_SEQUENCE_CONFIG, 0xE8);

  // VL53L0X_PerformRefCalibration() end

  return true;
}

// Write an 8-bit register
void VL53L0X_writeReg(struct VL53L0X* dev, uint8_t reg, uint8_t value)
{
	uint8_t buf[2];
	buf[0] = reg;
	buf[1] = value;
	i2c_write(I2C1, 0b0101001, buf, 2);
}

// Write a 16-bit register
void VL53L0X_writeReg16Bit(struct VL53L0X* dev, uint8_t reg, uint16_t value)
{
	uint8_t buf[3];
	buf[0] = reg;
	buf[1] = (uint8_t) (value >> 8);
	buf[2] = (uint8_t) (value & 0xFF);
	i2c_write(I2C1, 0b0101001, buf, 3);
}

// Write a 32-bit register
void VL53L0X_writeReg32Bit(struct VL53L0X* dev, uint8_t reg, uint32_t value)
{
	uint8_t buf[5];
	buf[0] = reg;
	buf[1] = (uint8_t) (value >> 24);
	buf[2] = (uint8_t) (value >> 16);
	buf[3] = (uint8_t) (value >> 8);
	buf[4] = (uint8_t) (value & 0xFF);
	i2c_write(I2C1, 0b0101001, buf, 5);
}

// Read an 8-bit register
uint8_t VL53L0X_readReg(struct VL53L0X* dev, uint8_t reg)
{
  uint8_t value;
  i2c_write(I2C1, 0b0101001, &reg, 1);
  i2c_read(I2C1, 0b0101001, &value, 1);
  return value;
}

// Read a 16-bit register
uint16_t VL53L0X_readReg16Bit(struct VL53L0X* dev, uint8_t reg)
{
  uint16_t value;
  uint8_t buf[2];
  i2c_write(I2C1, 0b0101001, &reg, 1);
  i2c_read(I2C1, 0b0101001, buf, 2);
  value = (uint16_t) (buf[0] << 8);
  value |= (uint16_t) buf[1];
  return value;
}

// Read a 32-bit register
uint32_t VL53L0X_readReg32Bit(struct VL53L0X* dev, uint8_t reg)
{
  uint32_t value;
  uint8_t buf[4];
  i2c_write(I2C1, 0b0101001, &reg, 1);
  i2c_read(I2C1, 0b0101001, buf, 4);
  value = (uint32_t) ( buf[0] << 24 );
  value |= (uint32_t) ( buf[1] << 16 );
  value |= (uint32_t) ( buf[2] << 8 );
  value |= (uint32_t) buf[3];
  return value;
}

// Write an arbitrary number of bytes from the given array to the sensor,
// starting at the given register
void VL53L0X_writeMulti(struct VL53L0X* dev, uint8_t reg, uint8_t* src, uint8_t count)
{
	i2c_write(I2C1, 0b0101001, &reg, 1);
	i2c_write(I2C1, 0b0101001, src, count);
}

// Read an arbitrary number of bytes from the sensor, starting at the given
// register, into the given array
void VL53L0X_readMulti(struct VL53L0X* dev, uint8_t reg, uint8_t * dst, uint8_t count)
{
	i2c_write(I2C1, 0b0101001, &reg, 1);
	i2c_read(I2C1, 0b0101001, dst, count);
}

// Set the return signal rate limit check value in units of MCPS (mega counts
// per second). "This represents the amplitude of the signal reflected from the
// target and detected by the device"; setting this limit presumably determines
// the minimum measurement necessary for the sensor to report a valid reading.
// Setting a lower limit increases the potential range of the sensor but also
// seems to increase the likelihood of getting an inaccurate reading because of
// unwanted reflections from objects other than the intended target.
// Defaults to 0.25 MCPS as initialized by the ST API and this library.
bool VL53L0X_setSignalRateLimit(struct VL53L0X* dev)
{
  // Q9.7 fixed point format (9 integer bits, 7 fractional bits)
  VL53L0X_writeReg16Bit(dev, FINAL_RANGE_CONFIG_MIN_COUNT_RATE_RTN_LIMIT, (1 << 7) / 4);
  return true;
}

// Get the return signal rate limit check value in MCPS
uint16_t VL53L0X_getSignalRateLimit(struct VL53L0X* dev)
{
  return VL53L0X_readReg16Bit(dev, FINAL_RANGE_CONFIG_MIN_COUNT_RATE_RTN_LIMIT) * 4 / (1 << 7);
}

// Set the measurement timing budget in microseconds, which is the time allowed
// for one measurement; the ST API and this library take care of splitting the
// timing budget among the sub-steps in the ranging sequence. A longer timing
// budget allows for more accurate measurements. Increasing the budget by a
// factor of N decreases the range measurement standard deviation by a factor of
// sqrt(N). Defaults to about 33 milliseconds; the minimum is 20 ms.
// based on VL53L0X_set_measurement_timing_budget_micro_seconds()
bool VL53L0X_setMeasurementTimingBudget(struct VL53L0X* dev, uint32_t budget_us)
{
  struct VL53L0X_SequenceStepEnables enables;
  struct VL53L0X_SequenceStepTimeouts timeouts;

  uint16_t const StartOverhead      = 1320; // note that this is different than the value in get_
  uint16_t const EndOverhead        = 960;
  uint16_t const MsrcOverhead       = 660;
  uint16_t const TccOverhead        = 590;
  uint16_t const DssOverhead        = 690;
  uint16_t const PreRangeOverhead   = 660;
  uint16_t const FinalRangeOverhead = 550;

  uint32_t const MinTimingBudget = 20000;

  if (budget_us < MinTimingBudget) { return false; }

  uint32_t used_budget_us = StartOverhead + EndOverhead;

  VL53L0X_getSequenceStepEnables(dev, &enables);
  VL53L0X_getSequenceStepTimeouts(dev, &enables, &timeouts);

  if (enables.tcc)
  {
    used_budget_us += (timeouts.msrc_dss_tcc_us + TccOverhead);
  }

  if (enables.dss)
  {
    used_budget_us += 2 * (timeouts.msrc_dss_tcc_us + DssOverhead);
  }
  else if (enables.msrc)
  {
    used_budget_us += (timeouts.msrc_dss_tcc_us + MsrcOverhead);
  }

  if (enables.pre_range)
  {
    used_budget_us += (timeouts.pre_range_us + PreRangeOverhead);
  }

  if (enables.final_range)
  {
    used_budget_us += FinalRangeOverhead;

    // "Note that the final range timeout is determined by the timing
    // budget and the sum of all other timeouts within the sequence.
    // If there is no room for the final range timeout, then an error
    // will be set. Otherwise the remaining time will be applied to
    // the final range."

    if (used_budget_us > budget_us)
    {
      // "Requested timeout too big."
      return false;
    }

    uint32_t final_range_timeout_us = budget_us - used_budget_us;

    // set_sequence_step_timeout() begin
    // (SequenceStepId == VL53L0X_SEQUENCESTEP_FINAL_RANGE)

    // "For the final range timeout, the pre-range timeout
    //  must be added. To do this both final and pre-range
    //  timeouts must be expressed in macro periods MClks
    //  because they have different vcsel periods."

    uint16_t final_range_timeout_mclks = VL53L0X_timeoutMicrosecondsToMclks(final_range_timeout_us, timeouts.final_range_vcsel_period_pclks);

    if (enables.pre_range)
    {
      final_range_timeout_mclks += timeouts.pre_range_mclks;
    }

    VL53L0X_writeReg16Bit(dev, FINAL_RANGE_CONFIG_TIMEOUT_MACROP_HI, VL53L0X_encodeTimeout(final_range_timeout_mclks));

    // set_sequence_step_timeout() end

    dev->measurement_timing_budget_us = budget_us; // store for internal reuse
  }
  return true;
}

// Get the measurement timing budget in microseconds
// based on VL53L0X_get_measurement_timing_budget_micro_seconds()
// in us
uint32_t VL53L0X_getMeasurementTimingBudget(struct VL53L0X* dev)
{
  struct VL53L0X_SequenceStepEnables enables;
  struct VL53L0X_SequenceStepTimeouts timeouts;

  uint16_t const StartOverhead     = 1910; // note that this is different than the value in set_
  uint16_t const EndOverhead        = 960;
  uint16_t const MsrcOverhead       = 660;
  uint16_t const TccOverhead        = 590;
  uint16_t const DssOverhead        = 690;
  uint16_t const PreRangeOverhead   = 660;
  uint16_t const FinalRangeOverhead = 550;

  // "Start and end overhead times always present"
  uint32_t budget_us = StartOverhead + EndOverhead;

  VL53L0X_getSequenceStepEnables(dev, &enables);
  VL53L0X_getSequenceStepTimeouts(dev, &enables, &timeouts);

  if (enables.tcc)
  {
    budget_us += (timeouts.msrc_dss_tcc_us + TccOverhead);
  }

  if (enables.dss)
  {
    budget_us += 2 * (timeouts.msrc_dss_tcc_us + DssOverhead);
  }
  else if (enables.msrc)
  {
    budget_us += (timeouts.msrc_dss_tcc_us + MsrcOverhead);
  }

  if (enables.pre_range)
  {
    budget_us += (timeouts.pre_range_us + PreRangeOverhead);
  }

  if (enables.final_range)
  {
    budget_us += (timeouts.final_range_us + FinalRangeOverhead);
  }

  dev->measurement_timing_budget_us = budget_us; // store for internal reuse
  return budget_us;
}

// Set the VCSEL (vertical cavity surface emitting laser) pulse period for the
// given period type (pre-range or final range) to the given value in PCLKs.
// Longer periods seem to increase the potential range of the sensor.
// Valid values are (even numbers only):
//  pre:  12 to 18 (initialized default: 14)
//  final: 8 to 14 (initialized default: 10)
// based on VL53L0X_set_vcsel_pulse_period()
bool VL53L0X_setVcselPulsePeriod(struct VL53L0X* dev, enum VL53L0X_vcselPeriodType type, uint8_t period_pclks)
{
  uint8_t vcsel_period_reg = encodeVcselPeriod(period_pclks);

  struct VL53L0X_SequenceStepEnables enables;
  struct VL53L0X_SequenceStepTimeouts timeouts;

  VL53L0X_getSequenceStepEnables(dev, &enables);
  VL53L0X_getSequenceStepTimeouts(dev, &enables, &timeouts);

  // "Apply specific settings for the requested clock period"
  // "Re-calculate and apply timeouts, in macro periods"

  // "When the VCSEL period for the pre or final range is changed,
  // the corresponding timeout must be read from the device using
  // the current VCSEL period, then the new VCSEL period can be
  // applied. The timeout then must be written back to the device
  // using the new VCSEL period.
  //
  // For the MSRC timeout, the same applies - this timeout being
  // dependant on the pre-range vcsel period."


  if (type == VcselPeriodPreRange)
  {
    // "Set phase check limits"
    switch (period_pclks)
    {
      case 12:
        VL53L0X_writeReg(dev, PRE_RANGE_CONFIG_VALID_PHASE_HIGH, 0x18);
        break;

      case 14:
        VL53L0X_writeReg(dev, PRE_RANGE_CONFIG_VALID_PHASE_HIGH, 0x30);
        break;

      case 16:
        VL53L0X_writeReg(dev, PRE_RANGE_CONFIG_VALID_PHASE_HIGH, 0x40);
        break;

      case 18:
        VL53L0X_writeReg(dev, PRE_RANGE_CONFIG_VALID_PHASE_HIGH, 0x50);
        break;

      default:
        // invalid period
        return false;
    }
    VL53L0X_writeReg(dev, PRE_RANGE_CONFIG_VALID_PHASE_LOW, 0x08);

    // apply new VCSEL period
    VL53L0X_writeReg(dev, PRE_RANGE_CONFIG_VCSEL_PERIOD, vcsel_period_reg);

    // update timeouts

    // set_sequence_step_timeout() begin
    // (SequenceStepId == VL53L0X_SEQUENCESTEP_PRE_RANGE)

    uint16_t new_pre_range_timeout_mclks = VL53L0X_timeoutMicrosecondsToMclks(timeouts.pre_range_us, period_pclks);

    VL53L0X_writeReg16Bit(dev, PRE_RANGE_CONFIG_TIMEOUT_MACROP_HI, VL53L0X_encodeTimeout(new_pre_range_timeout_mclks));

    // set_sequence_step_timeout() end

    // set_sequence_step_timeout() begin
    // (SequenceStepId == VL53L0X_SEQUENCESTEP_MSRC)

    uint16_t new_msrc_timeout_mclks = VL53L0X_timeoutMicrosecondsToMclks(timeouts.msrc_dss_tcc_us, period_pclks);

    VL53L0X_writeReg(dev, MSRC_CONFIG_TIMEOUT_MACROP, (new_msrc_timeout_mclks > 256) ? 255 : (new_msrc_timeout_mclks - 1));

    // set_sequence_step_timeout() end
  }
  else if (type == VcselPeriodFinalRange)
  {
    switch (period_pclks)
    {
      case 8:
        VL53L0X_writeReg(dev, FINAL_RANGE_CONFIG_VALID_PHASE_HIGH, 0x10);
        VL53L0X_writeReg(dev, FINAL_RANGE_CONFIG_VALID_PHASE_LOW,  0x08);
        VL53L0X_writeReg(dev, GLOBAL_CONFIG_VCSEL_WIDTH, 0x02);
        VL53L0X_writeReg(dev, ALGO_PHASECAL_CONFIG_TIMEOUT, 0x0C);
        VL53L0X_writeReg(dev, 0xFF, 0x01);
        VL53L0X_writeReg(dev, ALGO_PHASECAL_LIM, 0x30);
        VL53L0X_writeReg(dev, 0xFF, 0x00);
        break;

      case 10:
        VL53L0X_writeReg(dev, FINAL_RANGE_CONFIG_VALID_PHASE_HIGH, 0x28);
        VL53L0X_writeReg(dev, FINAL_RANGE_CONFIG_VALID_PHASE_LOW,  0x08);
        VL53L0X_writeReg(dev, GLOBAL_CONFIG_VCSEL_WIDTH, 0x03);
        VL53L0X_writeReg(dev, ALGO_PHASECAL_CONFIG_TIMEOUT, 0x09);
        VL53L0X_writeReg(dev, 0xFF, 0x01);
        VL53L0X_writeReg(dev, ALGO_PHASECAL_LIM, 0x20);
        VL53L0X_writeReg(dev, 0xFF, 0x00);
        break;

      case 12:
        VL53L0X_writeReg(dev, FINAL_RANGE_CONFIG_VALID_PHASE_HIGH, 0x38);
        VL53L0X_writeReg(dev, FINAL_RANGE_CONFIG_VALID_PHASE_LOW,  0x08);
        VL53L0X_writeReg(dev, GLOBAL_CONFIG_VCSEL_WIDTH, 0x03);
        VL53L0X_writeReg(dev, ALGO_PHASECAL_CONFIG_TIMEOUT, 0x08);
        VL53L0X_writeReg(dev, 0xFF, 0x01);
        VL53L0X_writeReg(dev, ALGO_PHASECAL_LIM, 0x20);
        VL53L0X_writeReg(dev, 0xFF, 0x00);
        break;

      case 14:
        VL53L0X_writeReg(dev, FINAL_RANGE_CONFIG_VALID_PHASE_HIGH, 0x48);
        VL53L0X_writeReg(dev, FINAL_RANGE_CONFIG_VALID_PHASE_LOW,  0x08);
        VL53L0X_writeReg(dev, GLOBAL_CONFIG_VCSEL_WIDTH, 0x03);
        VL53L0X_writeReg(dev, ALGO_PHASECAL_CONFIG_TIMEOUT, 0x07);
        VL53L0X_writeReg(dev, 0xFF, 0x01);
        VL53L0X_writeReg(dev, ALGO_PHASECAL_LIM, 0x20);
        VL53L0X_writeReg(dev, 0xFF, 0x00);
        break;

      default:
        // invalid period
        return false;
    }

    // apply new VCSEL period
    VL53L0X_writeReg(dev, FINAL_RANGE_CONFIG_VCSEL_PERIOD, vcsel_period_reg);

    // update timeouts

    // set_sequence_step_timeout() begin
    // (SequenceStepId == VL53L0X_SEQUENCESTEP_FINAL_RANGE)

    // "For the final range timeout, the pre-range timeout
    //  must be added. To do this both final and pre-range
    //  timeouts must be expressed in macro periods MClks
    //  because they have different vcsel periods."

    uint16_t new_final_range_timeout_mclks = VL53L0X_timeoutMicrosecondsToMclks(timeouts.final_range_us, period_pclks);

    if (enables.pre_range)
    {
      new_final_range_timeout_mclks += timeouts.pre_range_mclks;
    }

    VL53L0X_writeReg16Bit(dev, FINAL_RANGE_CONFIG_TIMEOUT_MACROP_HI, VL53L0X_encodeTimeout(new_final_range_timeout_mclks));

    // set_sequence_step_timeout end
  }
  else
  {
    // invalid type
    return false;
  }

  // "Finally, the timing budget must be re-applied"

  VL53L0X_setMeasurementTimingBudget(dev, dev->measurement_timing_budget_us);

  // "Perform the phase calibration. This is needed after changing on vcsel period."
  // VL53L0X_perform_phase_calibration() begin

  uint8_t sequence_config = VL53L0X_readReg(dev,  SYSTEM_SEQUENCE_CONFIG);
  VL53L0X_writeReg(dev, SYSTEM_SEQUENCE_CONFIG, 0x02);
  VL53L0X_performSingleRefCalibration(dev, 0x0);
  VL53L0X_writeReg(dev, SYSTEM_SEQUENCE_CONFIG, sequence_config);

  // VL53L0X_perform_phase_calibration() end

  return true;
}

// Get the VCSEL pulse period in PCLKs for the given period type.
// based on VL53L0X_get_vcsel_pulse_period()
uint8_t VL53L0X_getVcselPulsePeriod(struct VL53L0X* dev, enum VL53L0X_vcselPeriodType type)
{
  if (type == VcselPeriodPreRange)
  {
    return decodeVcselPeriod(VL53L0X_readReg(dev,  PRE_RANGE_CONFIG_VCSEL_PERIOD));
  }
  else if (type == VcselPeriodFinalRange)
  {
    return decodeVcselPeriod(VL53L0X_readReg(dev,  FINAL_RANGE_CONFIG_VCSEL_PERIOD));
  }
  else { return 255; }
}

// Start continuous ranging measurements. If period_ms (optional) is 0 or not
// given, continuous back-to-back mode is used (the sensor takes measurements as
// often as possible); otherwise, continuous timed mode is used, with the given
// inter-measurement period in milliseconds determining how often the sensor
// takes a measurement.
// based on VL53L0X_StartMeasurement()
void VL53L0X_startContinuous(struct VL53L0X* dev, uint32_t period_ms)
{
  VL53L0X_writeReg(dev, 0x80, 0x01);
  VL53L0X_writeReg(dev, 0xFF, 0x01);
  VL53L0X_writeReg(dev, 0x00, 0x00);
  VL53L0X_writeReg(dev, 0x91, dev->stop_variable);
  VL53L0X_writeReg(dev, 0x00, 0x01);
  VL53L0X_writeReg(dev, 0xFF, 0x00);
  VL53L0X_writeReg(dev, 0x80, 0x00);

  if (period_ms != 0)
  {
    // continuous timed mode

    // VL53L0X_SetInterMeasurementPeriodMilliSeconds() begin

    uint16_t osc_calibrate_val = VL53L0X_readReg16Bit(dev, OSC_CALIBRATE_VAL);

    if (osc_calibrate_val != 0)
    {
      period_ms *= osc_calibrate_val;
    }

    VL53L0X_writeReg32Bit(dev, SYSTEM_INTERMEASUREMENT_PERIOD, period_ms);

    // VL53L0X_SetInterMeasurementPeriodMilliSeconds() end

    VL53L0X_writeReg(dev, SYSRANGE_START, 0x04); // VL53L0X_REG_SYSRANGE_MODE_TIMED
  }
  else
  {
    // continuous back-to-back mode
    VL53L0X_writeReg(dev, SYSRANGE_START, 0x02); // VL53L0X_REG_SYSRANGE_MODE_BACKTOBACK
  }
}

// Stop continuous measurements
// based on VL53L0X_StopMeasurement()
void VL53L0X_stopContinuous(struct VL53L0X* dev)
{
  VL53L0X_writeReg(dev, SYSRANGE_START, 0x01); // VL53L0X_REG_SYSRANGE_MODE_SINGLESHOT

  VL53L0X_writeReg(dev, 0xFF, 0x01);
  VL53L0X_writeReg(dev, 0x00, 0x00);
  VL53L0X_writeReg(dev, 0x91, 0x00);
  VL53L0X_writeReg(dev, 0x00, 0x01);
  VL53L0X_writeReg(dev, 0xFF, 0x00);
}

// Returns a range reading in millimeters when continuous mode is active
// (readRangeSingleMillimeters() also calls this function after starting a
// single-shot range measurement)
uint16_t VL53L0X_readRangeContinuousMillimeters(struct VL53L0X* dev)
{
  // VL53L0X_startTimeout(dev);
  // while ((VL53L0X_readReg(dev,  RESULT_INTERRUPT_STATUS) & 0x07) == 0)
  // {
  //   if (VL53L0X_checkTimeoutExpired(dev))
  //   {
  //     dev->did_timeout = true;
  //     return 65535;
  //   }
  // }

  // assumptions: Linearity Corrective Gain is 1000 (default);
  // fractional ranging is not enabled
  uint16_t range = VL53L0X_readReg16Bit(dev, RESULT_RANGE_STATUS + 10);

  VL53L0X_writeReg(dev, SYSTEM_INTERRUPT_CLEAR, 0x01);

  return range;
}

// Performs a single-shot range measurement and returns the reading in
// millimeters
// based on VL53L0X_PerformSingleRangingMeasurement()
uint16_t VL53L0X_readRangeSingleMillimeters(struct VL53L0X* dev)
{
  VL53L0X_writeReg(dev, 0x80, 0x01);
  VL53L0X_writeReg(dev, 0xFF, 0x01);
  VL53L0X_writeReg(dev, 0x00, 0x00);
  VL53L0X_writeReg(dev, 0x91, dev->stop_variable);
  VL53L0X_writeReg(dev, 0x00, 0x01);
  VL53L0X_writeReg(dev, 0xFF, 0x00);
  VL53L0X_writeReg(dev, 0x80, 0x00);

  VL53L0X_writeReg(dev, SYSRANGE_START, 0x01);

  // "Wait until start bit has been cleared"
  VL53L0X_startTimeout(dev);
  while (VL53L0X_readReg(dev,  SYSRANGE_START) & 0x01)
  {
    if (VL53L0X_checkTimeoutExpired(dev))
    {
      dev->did_timeout = true;
      return 65535;
    }
  }

  return VL53L0X_readRangeContinuousMillimeters(dev);
}

// Did a timeout occur in one of the read functions since the last call to
// timeoutOccurred()?
bool VL53L0X_timeoutOccurred(struct VL53L0X* dev)
{
  bool tmp = dev->did_timeout;
  dev->did_timeout = false;
  return tmp;
}

// Get reference SPAD (single photon avalanche diode) count and type
// based on VL53L0X_get_info_from_device(),
// but only gets reference SPAD count and type
bool VL53L0X_getSpadInfo(struct VL53L0X* dev, uint8_t * count, bool * type_is_aperture)
{
  uint8_t tmp;

  VL53L0X_writeReg(dev, 0x80, 0x01);
  VL53L0X_writeReg(dev, 0xFF, 0x01);
  VL53L0X_writeReg(dev, 0x00, 0x00);

  VL53L0X_writeReg(dev, 0xFF, 0x06);
  VL53L0X_writeReg(dev, 0x83, VL53L0X_readReg(dev,  0x83) | 0x04);
  VL53L0X_writeReg(dev, 0xFF, 0x07);
  VL53L0X_writeReg(dev, 0x81, 0x01);

  VL53L0X_writeReg(dev, 0x80, 0x01);

  VL53L0X_writeReg(dev, 0x94, 0x6b);
  VL53L0X_writeReg(dev, 0x83, 0x00);
  VL53L0X_startTimeout(dev);
  while (VL53L0X_readReg(dev,  0x83) == 0x00)
  {
    if (VL53L0X_checkTimeoutExpired(dev)) { return false; }
  }
  VL53L0X_writeReg(dev, 0x83, 0x01);
  tmp = VL53L0X_readReg(dev,  0x92);

  *count = tmp & 0x7f;
  *type_is_aperture = (tmp >> 7) & 0x01;

  VL53L0X_writeReg(dev, 0x81, 0x00);
  VL53L0X_writeReg(dev, 0xFF, 0x06);
  VL53L0X_writeReg(dev, 0x83, VL53L0X_readReg(dev,  0x83)  & ~0x04);
  VL53L0X_writeReg(dev, 0xFF, 0x01);
  VL53L0X_writeReg(dev, 0x00, 0x01);

  VL53L0X_writeReg(dev, 0xFF, 0x00);
  VL53L0X_writeReg(dev, 0x80, 0x00);

  return true;
}

// Get sequence step enables
// based on VL53L0X_GetSequenceStepEnables()
void VL53L0X_getSequenceStepEnables(struct VL53L0X* dev, struct VL53L0X_SequenceStepEnables* enables)
{
  uint8_t sequence_config = VL53L0X_readReg(dev,  SYSTEM_SEQUENCE_CONFIG);

  enables->tcc          = (sequence_config >> 4) & 0x1;
  enables->dss          = (sequence_config >> 3) & 0x1;
  enables->msrc         = (sequence_config >> 2) & 0x1;
  enables->pre_range    = (sequence_config >> 6) & 0x1;
  enables->final_range  = (sequence_config >> 7) & 0x1;
}

// Get sequence step timeouts
// based on get_sequence_step_timeout(),
// but gets all timeouts instead of just the requested one, and also stores
// intermediate values
void VL53L0X_getSequenceStepTimeouts(struct VL53L0X* dev, struct VL53L0X_SequenceStepEnables* enables, struct VL53L0X_SequenceStepTimeouts* timeouts)
{
  timeouts->pre_range_vcsel_period_pclks = VL53L0X_getVcselPulsePeriod(dev, VcselPeriodPreRange);

  timeouts->msrc_dss_tcc_mclks = VL53L0X_readReg(dev,  MSRC_CONFIG_TIMEOUT_MACROP) + 1;
  timeouts->msrc_dss_tcc_us = VL53L0X_timeoutMclksToMicroseconds(timeouts->msrc_dss_tcc_mclks, timeouts->pre_range_vcsel_period_pclks);

  timeouts->pre_range_mclks = VL53L0X_decodeTimeout(VL53L0X_readReg16Bit(dev, PRE_RANGE_CONFIG_TIMEOUT_MACROP_HI));
  timeouts->pre_range_us = VL53L0X_timeoutMclksToMicroseconds(timeouts->pre_range_mclks, timeouts->pre_range_vcsel_period_pclks);

  timeouts->final_range_vcsel_period_pclks = VL53L0X_getVcselPulsePeriod(dev, VcselPeriodFinalRange);

  timeouts->final_range_mclks = VL53L0X_decodeTimeout(VL53L0X_readReg16Bit(dev, FINAL_RANGE_CONFIG_TIMEOUT_MACROP_HI));

  if (enables->pre_range)
  {
    timeouts->final_range_mclks -= timeouts->pre_range_mclks;
  }

  timeouts->final_range_us = VL53L0X_timeoutMclksToMicroseconds(timeouts->final_range_mclks, timeouts->final_range_vcsel_period_pclks);
}

// Decode sequence step timeout in MCLKs from register value
// based on VL53L0X_decode_timeout()
// Note: the original function returned a uint32_t, but the return value is
// always stored in a uint16_t.
uint16_t VL53L0X_decodeTimeout(uint16_t reg_val)
{
  // format: "(LSByte * 2^MSByte) + 1"
  return (uint16_t)((reg_val & 0x00FF) <<
         (uint16_t)((reg_val & 0xFF00) >> 8)) + 1;
}

// Encode sequence step timeout register value from timeout in MCLKs
// based on VL53L0X_encode_timeout()
// Note: the original function took a uint16_t, but the argument passed to it
// is always a uint16_t.
uint16_t VL53L0X_encodeTimeout(uint16_t timeout_mclks)
{
  // format: "(LSByte * 2^MSByte) + 1"

  uint32_t ls_byte = 0;
  uint16_t ms_byte = 0;

  if (timeout_mclks > 0)
  {
    ls_byte = timeout_mclks - 1;

    while ((ls_byte & 0xFFFFFF00) > 0)
    {
      ls_byte >>= 1;
      ms_byte++;
    }

    return (ms_byte << 8) | (ls_byte & 0xFF);
  }
  else { return 0; }
}

// Convert sequence step timeout from MCLKs to microseconds with given VCSEL period in PCLKs
// based on VL53L0X_calc_timeout_us()
uint32_t VL53L0X_timeoutMclksToMicroseconds(uint16_t timeout_period_mclks, uint8_t vcsel_period_pclks)
{
  uint32_t macro_period_ns = calcMacroPeriod(vcsel_period_pclks);

  return ((timeout_period_mclks * macro_period_ns) + (macro_period_ns / 2)) / 1000;
}

// Convert sequence step timeout from microseconds to MCLKs with given VCSEL period in PCLKs
// based on VL53L0X_calc_timeout_mclks()
uint32_t VL53L0X_timeoutMicrosecondsToMclks(uint32_t timeout_period_us, uint8_t vcsel_period_pclks)
{
  uint32_t macro_period_ns = calcMacroPeriod(vcsel_period_pclks);

  return (((timeout_period_us * 1000) + (macro_period_ns / 2)) / macro_period_ns);
}


// based on VL53L0X_perform_single_ref_calibration()
bool VL53L0X_performSingleRefCalibration(struct VL53L0X* dev, uint8_t vhv_init_byte)
{
  VL53L0X_writeReg(dev, SYSRANGE_START, 0x01 | vhv_init_byte); // VL53L0X_REG_SYSRANGE_MODE_START_STOP

  VL53L0X_startTimeout(dev);
  while ((VL53L0X_readReg(dev,  RESULT_INTERRUPT_STATUS) & 0x07) == 0)
  {
    if (VL53L0X_checkTimeoutExpired(dev)) { return false; }
  }

  VL53L0X_writeReg(dev, SYSTEM_INTERRUPT_CLEAR, 0x01);

  VL53L0X_writeReg(dev, SYSRANGE_START, 0x00);

  return true;
}


void VL53L0X_startTimeout(struct VL53L0X* dev){
	//dev->timeout_start_ms = os_current_millis();
}

bool VL53L0X_checkTimeoutExpired(struct VL53L0X* dev){
	return 0;//(dev->io_timeout > 0 && (os_current_millis() - dev->timeout_start_ms) > dev->io_timeout);
}
