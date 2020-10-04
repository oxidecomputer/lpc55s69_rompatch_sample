ifeq ($(EXTRA_OPTION),)
flags =
else
flags = -D$(EXTRA_OPTION)
endif


basic: small.o entry.o
	arm-none-eabi-ld -o sample -T link.x --build-id=none -nostdlib small.o entry.o

entry.o: entry.S
	arm-none-eabi-as -g -mthumb-interwork -mcpu=cortex-m33 -mthumb -o entry.o entry.S

small.o: small.c
	arm-none-eabi-gcc $(flags) -g -mthumb-interwork -mcmse -mcpu=cortex-m33 -mthumb -c small.c

.PHONY: clean
clean:
	-rm *.o
	-rm sample


