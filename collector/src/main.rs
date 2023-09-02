use console::Term;
use dialoguer::{theme::ColorfulTheme, FuzzySelect};

use std::time::Duration;

mod port_info;

fn main() -> std::io::Result<()> {
    let items = vec!["Item 1", "item 2"];
    let selection = FuzzySelect::with_theme(&ColorfulTheme::default())
        .items(&items)
        .default(0)
        .interact_on_opt(&Term::stderr())?;

    match selection {
        Some(index) => println!("User selected item : {}", items[index]),
        None => println!("User did not select anything"),
    }

    port_info::print();

    let mut port = serialport::new("COM3", 9_600)
        .timeout(Duration::from_millis(10000))
        .open()
        .expect("Failed to open port");

    let mut serial_buf: Vec<u8> = vec![0; 32];
    port.read(serial_buf.as_mut_slice())
        .expect("Found no data!");

    let string: String = String::from_utf8(serial_buf).expect("Found invalid UTF-8");

    println!("{:#?}", string);

    Ok(())
}
