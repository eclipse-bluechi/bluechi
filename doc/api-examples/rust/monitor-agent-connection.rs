// Copyright Contributors to the Eclipse BlueChi project
//
// SPDX-License-Identifier: MIT-0

use dbus::{
    arg::Variant,
    blocking::{stdintf::org_freedesktop_dbus::PropertiesPropertiesChanged, Connection},
    Message,
};
use std::time::Duration;

fn main() -> Result<(), Box<dyn std::error::Error>> {
    let conn = Connection::new_system()?;

    let bluechi = conn.with_proxy(
        "org.eclipse.bluechi.Agent",
        "/org/eclipse/bluechi",
        Duration::from_millis(5000),
    );
    let _id = bluechi.match_signal(
        |signal: PropertiesPropertiesChanged, _: &Connection, _: &Message| {
            match signal.changed_properties.get_key_value("Status") {
                Some((_, Variant(changed_value))) => {
                    println!("Agent status: {}", changed_value.as_str().unwrap_or(""))
                }
                _ => {}
            }
            true
        },
    );

    loop {
        conn.process(Duration::from_millis(1000))?;
    }
}
