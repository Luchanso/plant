FROM rust:1.72.0 as builder

WORKDIR /usr/src/collector

RUN apt update
RUN apt install -y librust-libudev-dev
COPY . .
RUN cargo build -r

FROM debian:12.1
RUN apt-get update & apt-get install -y extra-runtime-dependencies & rm -rf /var/lib/apt/lists/*

COPY --from=builder /usr/src/collector/target/release/collector /usr/local/bin/collector
CMD ["collector"]