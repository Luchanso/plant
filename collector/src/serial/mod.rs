use bytes::BytesMut;
use tokio::{
    io::AsyncReadExt,
    time::{sleep, Duration},
};
use tokio_cron_scheduler::{Job, JobScheduler, JobSchedulerError};
use tokio_serial::{SerialPortBuilderExt, SerialStream};

mod path;
mod port_info;

const BAUD_RATE: u32 = 9600;
const MIN_MSG_SIZE: u32 = 4;

async fn measurement_request(mut port: SerialStream) {
    // port.try_write();
}

pub async fn run() {
    let path = path::get();

    sleep(Duration::from_secs(1)).await;

    let _ = schedule().await;

    let mut port = tokio_serial::new(path, BAUD_RATE)
        .open_native_async()
        .unwrap();

    loop {
        let mut buffer = BytesMut::with_capacity(10);
        let _ = port.read_buf(&mut buffer).await;

        println!("{:?}", &buffer[..]);
    }

    return;

    // let mut port = serialport::new(path, BAUD_RATE).open();

    // match port {
    //     Err(error) => {
    //         println!("Cannot open serialport: {:?}", error);
    //         port_info::print();
    //     }
    //     Ok(result) => read(result).await,
    // };
}

async fn schedule() -> Result<(), JobSchedulerError> {
    let mut sched = JobScheduler::new().await?;

    // Add basic cron job
    sched.add(
        Job::new("1/10 * * * * *", |_uuid, _l| {
            println!("I run every 10 seconds");
        })?
    ).await?;

    sched.start().await?;

    Ok(())
}
