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

1. Multi-threading or Async
2. Write SQLite file
3. Docker-compose for Grafana
4. Grafana draw grafics
5. Docker container
6. Select Path
7. CRC
8. ***
