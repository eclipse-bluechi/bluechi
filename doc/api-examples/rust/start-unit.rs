// Copyright Contributors to the Eclipse BlueChi project
//
// SPDX-License-Identifier: MIT-0

use clap::Parser;
use dbus::blocking::Connection;
use dbus::Path;
use std::time::Duration;

#[derive(Parser)]
struct Cli {
    /// The node name to list the units for
    #[clap(short, long)]
    node_name: String,

    /// The name of the unit to start
    #[clap(short, long)]
    unit_name: String,
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
        bluechi.method_call("org.eclipse.bluechi.Controller", "GetNode", (&args.node_name,))?;

    let node_proxy = conn.with_proxy("org.eclipse.bluechi", node, Duration::from_millis(5000));

    let (job_path,): (Path,) = node_proxy.method_call(
        "org.eclipse.bluechi.Node",
        "StartUnit",
        (&args.unit_name, "replace"),
    )?;

    println!(
        "Started unit '{}' on node '{}': {}",
        args.unit_name, args.node_name, job_path
    );

    Ok(())
}
