// Copyright Contributors to the Eclipse BlueChi project
//
// SPDX-License-Identifier: MIT-0

use clap::Parser;
use dbus::arg::Variant;
use dbus::blocking::Connection;
use dbus::Path;
use std::time::Duration;

#[derive(Parser)]
struct Cli {
    /// The node name to list the units for
    #[clap(short, long)]
    node_name: String,

    /// The unit name to set the cpu weight for
    #[clap(short, long)]
    unit_name: String,

    /// The new value of the cpu weight
    #[clap(short, long)]
    cpu_weight: u64,
}

fn main() -> Result<(), Box<dyn std::error::Error>> {
    let args = Cli::parse();

    let conn = Connection::new_system()?;

    let bluechi = conn.with_proxy(
        "org.eclipse.bluechi",
        "/org/eclipse/bluechi",
        Duration::from_millis(5000),
    );

    let (node,): (Path,) =
        bluechi.method_call("org.eclipse.bluechi.Controller", "GetNode", (args.node_name,))?;

    let node_proxy = conn.with_proxy("org.eclipse.bluechi", node, Duration::from_millis(5000));

    let new_cpu_weight = Variant(args.cpu_weight);
    let mut values: Vec<(String, Variant<u64>)> = Vec::new();
    values.push(("CPUWeight".to_string(), new_cpu_weight));

    node_proxy.method_call(
        "org.eclipse.bluechi.Node",
        "SetUnitProperties",
        (args.unit_name, false, values),
    )?;

    Ok(())
}
