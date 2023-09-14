// SPDX-License-Identifier: MIT-0

use dbus::blocking::Connection;
use std::time::Duration;

fn main() -> Result<(), Box<dyn std::error::Error>> {
    let conn = Connection::new_system()?;

    let proxy = conn.with_proxy(
        "org.eclipse.bluechi",
        "/org/eclipse/bluechi",
        Duration::from_millis(5000),
    );

    let (nodes,): (Vec<(String, dbus::Path, String)>,) =
        proxy.method_call("org.eclipse.bluechi.Manager", "ListNodes", ())?;

    for node in nodes {
        println!("Node: {}, Status: {}", node.0, node.2);
    }

    Ok(())
}
