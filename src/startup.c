#include <stdint.h>

extern uint32_t _estack;
extern uint32_t _sdata, _edata;
extern uint32_t _sbss, _ebss;
extern uint32_t _sidata;

int main(void);
void reset_handler(void);
void default_handler(void);

void nmi_handler(void) __attribute__((weak, alias("default_handler")));
void hard_fault_handler(void) __attribute__((weak, alias("default_handler")));
void mem_manage_handler(void) __attribute__((weak, alias("default_handler")));
void bus_fault_handler(void) __attribute__((weak, alias("default_handler")));
void usage_fault_handler(void) __attribute__((weak, alias("default_handler")));
void sv_call_handler(void) __attribute__((weak, alias("default_handler")));
void debug_monitor_handler(void)
    __attribute__((weak, alias("default_handler")));
void pend_sv_handler(void) __attribute__((weak, alias("default_handler")));
void systick_handler(void) __attribute__((weak, alias("default_handler")));

__attribute__((section(".isr_vector"))) void *vector_table[76] = {
    &_estack,
    reset_handler,
    nmi_handler,
    hard_fault_handler,
    mem_manage_handler,
    bus_fault_handler,
    usage_fault_handler,
    0,
    0,
    0,
    0,
    sv_call_handler,
    debug_monitor_handler,
    0,
    pend_sv_handler,
    systick_handler,
    [16 ... 75] = default_handler};

void reset_handler(void) {
  uint32_t *src = &_sidata;
  uint32_t *dst = &_sdata;
  while (dst < &_edata)
    *dst++ = *src++;

  dst = &_sbss;
  while (dst < &_ebss)
    *dst++ = 0;

  main();

  while (1)
    ;
}

void default_handler(void) {
  while (1)
    ;
}
