Atmel AtMega328p Xplained mini

## Simple binary protocol

```
/*
 Simple binary protocol.

 Example:
 ┌───────────────── Message start marker, always 0x3A
 │  ┌────────────── Message code: 0x02 - measurement result
 │  │  ┌─────────── Payload length: 0x01 - payload contains one byte
 │  │  │  ┌──────── Message payload: 0xA0 - ADC measurement = 160
 3A 02 01 A0 77 ─── Maxim/Dallas integrated iButton CRC8
Hence, the smallest possible message using this protocol is 4 bytes long.
*/
```