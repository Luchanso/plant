use dotenv::dotenv;

mod api;
mod metrics;
mod serial;


#[tokio::main]
async fn main() {
    dotenv().ok();

    tokio::join!(api::run(), serial::run());
}
