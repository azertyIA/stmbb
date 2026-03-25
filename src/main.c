#include <stdint.h>

#define RCC_BASE 0x40021000
#define RCC_APB1ENR_REGISTER (*(volatile uint32_t *)(RCC_BASE + 0x1C))
#define RCC_APB1ENR_TIM2EN (1 << 0)
#define RCC_APB2ENR_REGISTER (*(volatile uint32_t *)(RCC_BASE + 0x18))
#define RCC_APB2ENR_AFIOEN (1 << 0)
#define RCC_APB2ENR_IOPAEN (1 << 2)
#define RCC_APB2ENR_IOPBEN (1 << 3)
#define RCC_APB2ENR_IOPCEN (1 << 4)
#define RCC_APB2ENR_ADC1EN (1 << 9)

#define ADC1_BASE 0x40012400
#define ADC_SR_REGISTER(x) (*(volatile uint32_t *)(x))
#define ADC_CR1_REGISTER(x) (*(volatile uint32_t *)(x + 0x04))
#define ADC_CR2_REGISTER(x) (*(volatile uint32_t *)(x + 0x08))
#define ADC_SMPR2_REGISTER(x) (*(volatile uint32_t *)(x + 0x10))
#define ADC_SQR3_REGISTER(x) (*(volatile uint32_t *)(x + 0x34))
#define ADC_DR_REGISTER(x) (*(volatile uint32_t *)(x + 0x4C))

#define GPIO_PORTA_BASE 0x40010800
#define GPIO_PORTB_BASE 0x40010C00
#define GPIO_PORTC_BASE 0x40011000

#define TIM2_BASE 0x40000000
#define TIM2_CR1 (*(volatile uint32_t *)(TIM2_BASE))
#define TIM2_CCMR1 (*(volatile uint32_t *)(TIM2_BASE + 0x18))
#define TIM2_CCER (*(volatile uint32_t *)(TIM2_BASE + 0x20))
#define TIM2_PSC (*(volatile uint32_t *)(TIM2_BASE + 0x28))
#define TIM2_ARR (*(volatile uint32_t *)(TIM2_BASE + 0x2C))
#define TIM2_CCR1 (*(volatile uint32_t *)(TIM2_BASE + 0x34))

#define GPIO_CRH_REGISTER(x) (*(volatile uint32_t *)(x + 0x4))
#define GPIO_CRH_MODE_MASK(x) (0x3 << ((x - 8) * 4))
#define GPIO_CRH_CNFO_MASK(x) (0x6 << ((x - 8) * 4))
#define GPIO_CRH_MODE_OUTPUT(x) (0x1 << ((x - 8) * 4))
#define GPIO_CRH_ALT_10MHZ(x) (0x9 << ((x - 8) * 4))

#define GPIO_CRL_REGISTER(x) (*(volatile uint32_t *)(x))
#define GPIO_CRL_MODE_MASK(x) (0xF << ((x) * 4))
#define GPIO_CRL_CNFO_MASK(x) (0xF << ((x) * 4))
#define GPIO_CRL_MODE_OUTPUT(x) (0x1 << ((x) * 4))
#define GPIO_CRL_ALT_10MHZ(x) (0x9 << ((x) * 4))

#define GPIO_ODR_REGISTER(x) (*(volatile uint32_t *)(x + 0xC))
#define GPIO_ODR_PIN(x) (0x1 << (x))

#define GPIO_BSRR_REGISTER(x) (*(volatile uint32_t *)(x + 0x10))
#define GPIO_BSRR_SET(x) (0x1 << (x))
#define GPIO_BSRR_RESET(x) (0x1 << (x + 0x10))

#define AFIO_BASE 0x40010000
#define AFIO_EVCR_REGISTER (*(volatile uint32_t *)(AFIO_BASE))
#define AFIO_MAPR_REGISTER (*(volatile uint32_t *)(AFIO_BASE + 0x04))

#define GPIO_BLINK_PORT GPIO_PORTB_BASE
#define GPIO_BLINK_NUM 11

#define GPIO_PWM_PORT GPIO_PORTA_BASE
#define GPIO_PWM_NUM 0
#define GPIO_ADC_NUM 1

#define PWM_ON 500000
#define PWM_MAX 3000
#define REST_V 800

uint32_t adc_data = 0;

int main(void) {
  RCC_APB2ENR_REGISTER |= RCC_APB2ENR_IOPAEN;

  GPIO_CRL_REGISTER(GPIO_PWM_PORT) &= ~0x000000FF; // clear 0 and 1
  GPIO_CRL_REGISTER(GPIO_PWM_PORT) |= 0x9;
  AFIO_MAPR_REGISTER &= ~0x300; // TIM2 remap

  RCC_APB1ENR_REGISTER |= RCC_APB1ENR_TIM2EN;
  RCC_APB2ENR_REGISTER |= RCC_APB2ENR_ADC1EN;

  ADC_CR2_REGISTER(ADC1_BASE) |= 0x7 << 17;
  ADC_SQR3_REGISTER(ADC1_BASE) |= 1U << 0;
  ADC_SMPR2_REGISTER(ADC1_BASE) |= 8U;
  ADC_CR2_REGISTER(ADC1_BASE) |= 1;
  for (int i = 0; i < 7; i++)
    ;

  // calibration regs
  ADC_CR2_REGISTER(ADC1_BASE) |= 8U;
  while (ADC_CR2_REGISTER(ADC1_BASE) & 8U)
    ;

  // actual calibration
  ADC_CR2_REGISTER(ADC1_BASE) |= 4U;
  while (ADC_CR2_REGISTER(ADC1_BASE) & 4U)
    ;

  TIM2_PSC = 0;
  TIM2_ARR = PWM_MAX;
  TIM2_CCMR1 |= 0x3 << 5;
  TIM2_CCER |= 1;
  TIM2_CR1 |= 1;

  TIM2_CCR1 = 500;

  while (1) {
    ADC_CR2_REGISTER(ADC1_BASE) |= 1UL;
    for (int i = 0; i < 7; i++)
      ;

    while (!(ADC_SR_REGISTER(ADC1_BASE) & 0x2))
      ;

    adc_data = ADC_DR_REGISTER(ADC1_BASE);
    if (adc_data > REST_V && TIM2_CCR1 > 0)
      TIM2_CCR1--;
    else if (adc_data < REST_V && TIM2_CCR1 < 500)
      TIM2_CCR1++;
  }
}
