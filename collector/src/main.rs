mod api;
mod metrics;
mod port_info;

use std::{thread, time};

use prometheus::TextEncoder;

use crate::metrics::GROUND_MOISTURE;

const BAUD_RATE: u32 = 9600;
const PATH: &str = "COM3";
const MIN_MSG_SIZE: u32 = 4;

#[tokio::main]
async fn main() {
    // port_info::print();

    GROUND_MOISTURE.set(255);
    // println!("hello world {:?}",);

    api::run().await;

    // let mut port = serialport::new(PATH, BAUD_RATE)
    //     .open()
    //     .expect("Failed to open port");

    // let mut data;

    // loop {
    //     loop {
    //         data = port.bytes_to_read().expect("Something went wrong");

    //         if data > MIN_MSG_SIZE {
    //             break;
    //         }

    //         thread::sleep(time::Duration::from_millis(500));

    //         println!("bytes in buffer: {}", data);
    //     }

    //     let mut serial_buf: Vec<u8> = vec![0; data.try_into().unwrap()];

    //     port.read(serial_buf.as_mut_slice())
    //         .expect("Found no data!");

    //     println!("{:#?}", data);
    //     println!("{:#?}", serial_buf);
    // }

    // CRON?
    // db.writeAnalogData(255, date.now())
}
