mod bluechi;

use std::time::Duration;
use dbus::blocking::Connection;

fn main() -> Result<(), Box<dyn std::error::Error>> {
    let conn = Connection::new_system()?;

    let proxy = conn.with_proxy(
        "org.eclipse.bluechi",
        "/org/eclipse/bluechi",
        Duration::from_millis(5000)
    );

    // Import our generated Trait
    use bluechi::OrgEclipseBluechiManager;

    let result: Result<Vec<(std::string::String, dbus::Path<'_>, std::string::String)>, dbus::Error> = proxy.list_nodes();

    match result {
        Ok(vec) => {
            for item in  vec.iter() {
                let (name, path, status) = item;
                println!("Node: {:?}", name);
                println!("{:?}", path);
                println!("Status: {:?}", status);
            }
        }
        Err(err) => {
            eprintln!("Error: {:?}", err);
        }
    }

    Ok(())
}
