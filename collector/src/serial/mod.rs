use bytes::{Buf, BytesMut};
use crc;
use tokio::io::{AsyncReadExt, AsyncWriteExt};
use tokio_cron_scheduler::{Job, JobScheduler, JobSchedulerError};
use tokio_serial::SerialPortBuilderExt;

use crate::metrics::GROUND_MOISTURE;

mod path;
mod port_info;

const BAUD_RATE: u32 = 9600;

const CRC: crc::Crc<u8> = crc::Crc::<u8>::new(&crc::CRC_8_MAXIM_DOW);

async fn measurement_request() {
    // 3a 01 00 70
    let message: Vec<u8> = vec![0x3A, 0x01, 0x00];
    let checksum = CRC.checksum(message.as_slice());
    let request_message = [message, vec![checksum]].concat();
    let request_message = request_message.as_slice();

    let path = path::get();
    let mut serial_stream = tokio_serial::new(path, BAUD_RATE).open_native_async();

    println!("Open");

    match serial_stream {
        Ok(mut stream) => {
            // to
            stream.write(request_message).await.unwrap();
            println!("Wrote {:?}", request_message);
            // stream.write_buf(&mut request_message).await.unwrap();

            println!("Waiting");
            let mut read_buf = BytesMut::with_capacity(32);
            stream.read_buf(&mut read_buf).await.unwrap();

            let checksum = CRC.checksum(read_buf.chunk());

            if checksum != 0 {
                println!("Checksum don't match");
            }

            GROUND_MOISTURE.set(read_buf.chunk()[3].into());

            println!("result {:?}", read_buf);
        }
        Err(error) => {
            println!("Cannot open serialport: {:?}", error);
            port_info::print();
        }
    };
}

pub async fn run() -> Result<(), JobSchedulerError> {
    // measurement_request().await;
    let scheduler = JobScheduler::new().await?;
    let job = Job::new_async("1/3 * * * * *", move |_uuid, mut job| {
        Box::pin(async move {
            // let _ = job.shutdown().await;
            measurement_request().await;
        })
    })
    .unwrap();

    scheduler.add(job).await?;
    scheduler.start().await?;

    Ok(())
}
