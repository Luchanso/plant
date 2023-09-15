use lazy_static::lazy_static;

use prometheus::IntGauge;
use prometheus::{opts, register_int_gauge};

lazy_static! {
    pub static ref GROUND_MOISTURE: IntGauge =
        register_int_gauge!(opts!("ground_mousture", "Ground moisture"))
            .expect("Can't create the ground_mousture metric");
}
