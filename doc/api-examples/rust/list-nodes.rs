// Copyright Contributors to the Eclipse BlueChi project
//
// SPDX-License-Identifier: MIT-0

use dbus::blocking::Connection;
use std::time::Duration;

fn main() -> Result<(), Box<dyn std::error::Error>> {
    let conn = Connection::new_system()?;

    let bluechi = conn.with_proxy(
        "org.eclipse.bluechi",
        "/org/eclipse/bluechi",
        Duration::from_millis(5000),
    );

    let (nodes,): (Vec<(String, dbus::Path, String, String)>,) =
        bluechi.method_call("org.eclipse.bluechi.Controller", "ListNodes", ())?;

    for (name, _, status) in nodes {
        println!("Node: {}, Status: {}", name, status);
    }

    Ok(())
}
