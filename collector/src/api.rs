use std::env;

use axum::http::StatusCode;
use axum::routing::get;
use axum::Router;
use prometheus::TextEncoder;
use rand::Rng;

use crate::metrics::GROUND_MOISTURE;

fn get_string_metrics() -> Result<String, prometheus::Error> {
    let encoder = TextEncoder::new();
    encoder.encode_to_string(&prometheus::gather())
}

async fn get_metrics() -> Result<String, StatusCode> {
    println!("call get_metrics");
    match env::var("MOCK") {
        Ok(_) => GROUND_MOISTURE.set(rand::thread_rng().gen_range(0..255)),
        Err(_) => (),
    }

    match get_string_metrics() {
        Ok(result) => Ok(result),
        Err(err) => Ok(err.to_string()),
    }
}

pub async fn run() {
    let app = Router::new().route("/metrics", get(get_metrics));

    // run it with hyper on localhost:3001
    axum::Server::bind(&"0.0.0.0:3001".parse().unwrap())
        .serve(app.into_make_service())
        .await
        .unwrap();
}
