```
#### SETUP :: STM32
FILE STRUCTURE:
```
project
- build
  - main.o
  - app.elf
- src
  - main.c
  - linker.ld
```

ENVIRONMENT 
```
n src
```

```
arm-none-eabi-gcc -o build/main.o -c -g -nostdlib -mcpu=cortex-m3 -mthumb src/main.c && arm-none-eabi-gcc -o build/app.elf -Wl,-Tsrc/linker.ld -nostartfiles build/main.o
```

```
gdb-multiarch build/app.elf
```

```
openocd -f interface/stlink.cfg -f target/stm32f1x.cfg
```

### 2026-03-18: STM32
It's obvious that ST is a terrible company that locks you into their products and is worthless at propelling innovation. As a result I have ditched the effort to try to get STM32CubeIDE working because of their validation requirements. As such, bare metal programming must be the way to go with my own linker scripts.

From [what](https://kleinembedded.com/stm32-without-cubeide-part-1-the-bare-necessities/) [I've](file:///home/ducks/Downloads/rm0008-stm32f101xx-stm32f102xx-stm32f103xx-stm32f105xx-and-stm32f107xx-advanced-armbased-32bit-mcus-stmicroelectronics.pdf) [seen](https://maldus512.medium.com/bare-metal-programming-on-an-stm32f103-3a0f4e50ca29#:~:text=Note:%20as%20the%20STM32F013%20is%20a%2032%2Dbit,and%20writing%20these%203%20kinds%20of%20memory.), there are two main tools to this toolchain: `arm-none-eabi-gcc` to build and `openOCD` to flash.

For foundations, the STM32 has both flash (disk) and SRAM (memory) along with registers of course. The ARM instructions let you do things with all three. Two registers that are important are the stack pointer (SP) and the program counter (PC), which can both be accessed in memory.

When the Blue Pill is powered on, the reset exception is invoked, causing some registers (SP and PC) to change their value based on the entry in the vector table. The vector table contains values for all exception handlers, fixed at address 0. The Blue Pill has 76 (0x000-0x12C) entries.

The first entry features the address of SP (which should be the last available address), while the second should point to the `main` function. Apparently, if you play your cards right, the flash memory contents can be accessed starting from `0x0000_0000` (boot) as well as `0x0800_0000` (original).

This is where the Linker script comes in. It helps the linker stitch the together the intermediate gcc results to arrive at the deliverable binary, but it usually looks like:

```
MEMORY
{
  FLASH (rx): ORIGIN = 0x08000000, LENGTH = 64K
  SRAM (rwx): ORIGIN = 0x20000000, LENGTH = 20K
}

__reset_stack_pointer = ORIGIN(SRAM) + LENGTH(SRAM);

SECTIONS  
{  
    .text : {  
        . = 0;  
        LONG(_reset_stack_pointer);  
        LONG(main | 1);  
        . = 332;  
        *(.text*)  
    } > FLASH
}
```

This basically spawns in the region needed for the interrupt vector with the first two spaces being the stack pointer and the main function pointer with the LSB being 1 to indicate the end of the vector. `*(.text*)` dumps all the code into the flash memory.

```
arm-none-eabi-gcc -o main.o -c -g -nostdlib -mcpu=cortex-m3 -mthumb main.c
arm-none-eabi-gcc -o application.elf -Wl,-Tmemory.ld -nostartfiles main.o
arm-none-eabi-objcopy -O binary application.elf application.bin
```

This should handle your needs.

To actually get things written to the GPIO, you need to:
- enable GPIO as a whole
- configure the pin as output
- force the pin to be high or low

To enable any peripheral, you ought to write to the RCC registers. In this case, we'll enable the PORT C clock, which can be found in the documentation: 
> `1 << 4` at `0x0000_0018 + 0x4002_1000 = 0x4002_1018`
> Set using bitwise operation: `*(0x4002_1018) |= (1 << 4)`

```
#include <stdint.h>

#define RCC_BASE 0x40021000
#define RCC_APB2ENR_REGISTER (*(volatile uint32_t *)(RCC_BASE + 0x18))
#define RCC_APB2ENR_IOPCEN (1 << 4)
```

It's good to use volatile here, because the compiler doesn't know if the value is going to change over time. So as a result, it could optimize a second read out of the binary.

To configure PIN 13, or where the onboard LED lives, you can look for the GPIO maps. We're gonna go to PORT C again, which uses `0x4001_1000`. 

There are two important registers that live there: CRLow `(0x00)` and CRHigh `(0x04)`: CRLow gets pins 0-7 and CRHigh gets pins 8-15. Each pin gets four bits, two for MODE (mode) and two for configuration (CNF). Just refer to this table.

![[Pasted image 20260319013521.png]]

In general, we won't use AFIO, so CNF is usually going to be 00 because open-drain is usually used for buses or OR gates. Push-pull just has better power consumption, has higher clock speeds and should be used as logic signals only.

```
logic output: 00XX
bus   output: 01XX
       input: XX00
```

It has run. My code runs on the STM32. Without the wicked tyranny of the CubeIDE. Some things are needed to flash the STM32. You need to get `gdb-multiarch` and `openocd`.

```
openocd -f interface/stlink.cfg -f target/stm32f1x.cfg
```

```
gdb-multiarch application.elf
(gdb) target extended-remote localhost:3333
```

With some extra aliasing this is the totality of the environment. Of course I will constantly need to keep the documentation in another tab. But I needed the reading practice anyways.

The last step is actually controlling it, just like how ArduinoIDE does. To do this, we use the ODR (output data register), which has an offset of `0x0C`. There are 16 GPIO pins and you write to the one you feel like.

However, I made a makefile that does all that, you just need a knock-off STLink
