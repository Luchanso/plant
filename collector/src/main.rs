#![allow(dead_code)]
#![allow(unused_variables)]

use dotenv::dotenv;

mod api;
mod metrics;
mod serial;

#[tokio::main]
async fn main() {
    dotenv().ok();

    let _ = tokio::join!(api::run(), serial::run());
}
