```
/*
 Simple binary protocol.

 Example:
 ┌───────────────── Message start marker, always 0x3A
 │  ┌────────────── Message code: 0x01 - measurement result
 │  │  ┌─────────── Payload length: 0x01 - payload contains one byte
 │  │  │  ┌──────── Message payload: 0xA0 - ADC measurement = 160
 3A 01 01 A0 93 ─── Maxim/Dallas integrated iButton CRC8
*/
```

1. Collect analog information from sensor and send to serial port
2. Read data with Rust application
3. Select Path
4. CRC
5. Multi-threading
6. ***
