// Copyright Contributors to the Eclipse BlueChi project
//
// SPDX-License-Identifier: MIT-0

use dbus::{
    arg::{RefArg, Variant},
    blocking::{stdintf::org_freedesktop_dbus::PropertiesPropertiesChanged, Connection},
    Message,
};
use std::time::Duration;

fn main() -> Result<(), Box<dyn std::error::Error>> {
    let conn = Connection::new_system()?;

    let bluechi = conn.with_proxy(
        "org.eclipse.bluechi",
        "/org/eclipse/bluechi",
        Duration::from_millis(5000),
    );

    let (nodes,): (Vec<(String, dbus::Path, String)>,) =
        bluechi.method_call("org.eclipse.bluechi.Controller", "ListNodes", ())?;

    for (name, path, _) in nodes {
        let node_name = name;
        let bluechi = conn.with_proxy("org.eclipse.bluechi", path, Duration::from_millis(5000));
        let _id = bluechi.match_signal(
            move |signal: PropertiesPropertiesChanged, _: &Connection, _: &Message| {
                match signal.changed_properties.get_key_value("Status") {
                    Some((_, Variant(changed_value))) => println!(
                        "Node {}: {}",
                        node_name,
                        changed_value.as_str().unwrap_or("")
                    ),
                    _ => {}
                }
                true
            },
        );
    }

    loop {
        conn.process(Duration::from_millis(1000))?;
    }
}
