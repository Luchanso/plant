## Run

With random data

```sh
MOCK=1 cargo run
```

Production mod

```sh
cargo run
```

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

1. Multi-threading or Async
2. Write SQLite file
3. Docker-compose for Grafana
4. Grafana draw grafics
5. Docker container
6. Select Path
7. CRC
8. ***
