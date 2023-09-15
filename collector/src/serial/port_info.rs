use serialport::SerialPortType;

pub fn print() {
    let ports = match serialport::available_ports() {
        Ok(result) => {
            println!("--- Available ports --- ");
            result
        }
        Err(error) => {
            println!("No available ports {:?}", error);
            return;
        }
    };

    for p in ports {
        println!("  {}", p.port_name);
        match p.port_type {
            SerialPortType::UsbPort(info) => {
                println!("    Type: USB");
                println!("    VID:{:04x} PID:{:04x}", info.vid, info.pid);
                println!(
                    "     Serial Number: {}",
                    info.serial_number.as_ref().map_or("", String::as_str)
                );
                println!(
                    "      Manufacturer: {}",
                    info.manufacturer.as_ref().map_or("", String::as_str)
                );
                println!(
                    "           Product: {}",
                    info.product.as_ref().map_or("", String::as_str)
                );
                #[cfg(feature = "usbportinfo-interface")]
                println!(
                    "         Interface: {}",
                    info.interface
                        .as_ref()
                        .map_or("".to_string(), |x| format!("{:02x}", *x))
                );
            }
            SerialPortType::BluetoothPort => {
                println!("    Type: Bluetooth");
            }
            SerialPortType::PciPort => {
                println!("    Type: PCI");
            }
            SerialPortType::Unknown => {
                println!("    Type: Unknown");
            }
        }
    }
}
