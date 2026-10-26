# Button Press Counter with Debouncing

Counts button presses on INT0 (PD2) and prints the count over UART. Demonstrates `volatile`, the ISR-sets-flag / main-does-the-work pattern, and software debouncing.

**Target:** ATmega328P (Arduino Uno), plain avr-gcc, direct register access.
**Depends on:** the interrupt-driven UART TX driver (`uart.c`/`uart.h`: ring buffer + `USART_UDRE_vect`).

## Hardware

| Part | Connection |
|---|---|
| Button | PD2 (Arduino pin 2) to GND, internal pull-up |
| Serial output | USB, 115200 baud (PuTTY on COM5) |

## Design

The ISR only records that an event happened; main does the slow work (printing).

```
ISR(INT0_vect) -> count++, flag = 1
main loop      -> if (flag) { clear flag; print count }
```

- `count` and `flag` are `volatile`. The compiler cannot see the hardware invoking the ISR, so without `volatile` it could cache them in registers and main would never see the change.
- `count` is `uint8_t`: an 8-bit read is a single instruction and cannot be torn by an interrupt (see project 03 for the 16-bit case).
- Main clears `flag` before printing, so a press arriving during the print is not lost.
- No printing inside the ISR: it would keep other interrupts waiting, and if the UART buffer filled, `uart_puts()` would wait for the TX ISR, which cannot run inside another ISR (deadlock).

## Version A: no debounce

One physical press often increments the counter several times. Contact bounce (1 to 10 ms of repeated opening and closing) creates multiple falling edges, and each one fires the ISR.

## Version B: lockout debounce

1. The ISR increments the count, sets the flag, and disables its own source (`EIMSK &= ~(1 << INT0)`).
2. Main prints, waits 50 ms, waits for the button to be released (`PIND` bit 2 reads high), waits another 50 ms (release bounce).
3. Main clears the stale pending flag (`EIFR = (1 << INTF0)`, written with `=`, since writing 1 clears the flag) and re-enables INT0.

Clearing `INTF0` before re-enabling matters: edges that arrived while INT0 was disabled still set the flag, and without clearing it the ISR would fire immediately on a stale bounce edge.

Result: exactly +1 per press.

## Trade-offs of this debounce

- Simple, no timer needed
- Blocking: main is stuck for 100 ms or more (`_delay_ms` is a busy-wait)
- Real presses during the lockout are ignored
- A timer-based approach (ISR starts a timer, a timer interrupt re-enables the button) avoids blocking

## Registers used

`DDRD`, `PORTD`, `PIND` (pin and pull-up), `EICRA` (falling edge), `EIMSK` (INT0 enable), `EIFR` (clear pending flag), `SREG` bit I via `sei()`.

## Build and flash

```
make
make flash PORT=COM5
```

## References

- ATmega328P datasheet (DS40002061): "External Interrupts" (EIMSK, EIFR)
- avr-libc manual: FAQ entry "What is volatile and how do I use it?", `<util/delay.h>`
