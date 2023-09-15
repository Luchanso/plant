use tokio::time::{sleep, Duration};

mod path;
mod port_info;

const BAUD_RATE: u32 = 9600;
const MIN_MSG_SIZE: u32 = 4;

pub async fn run() {
    let path = path::get();

    sleep(Duration::from_secs(1)).await;

    let mut port = serialport::new(path, BAUD_RATE).open();

    match port {
        Err(error) => {
            println!("Cannot open serialport: {:?}", error);
            port_info::print();
        }
        Ok(_) => (),
    };

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
