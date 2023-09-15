mod api;
mod metrics;
mod serial;

#[tokio::main]
async fn main() {
    tokio::join!(api::run(), serial::run());
}
