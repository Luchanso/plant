use std::env;

const PATH: &str = "COM3";

pub fn get() -> String {
    match env::var("PLANT_DEVICE") {
        Ok(result) => result,
        Err(error) => {
            match error {
                env::VarError::NotPresent => println!("WARNING: env PLANT_DEVICE not present, using default value \"COM3\""),
                env::VarError::NotUnicode(_) => println!("WARNING: env PLANT_DEVICE not contain valid unicode data, using default value \"COM3\""),
            };

            PATH.to_string()
        }
    }
}
