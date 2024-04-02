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

    /// The names of the units to enable. Names are separated by ','.
    #[clap(short, long, value_delimiter = ',')]
    unit_names: Vec<String>,
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

    let (carries_install_info, changes): (bool, Vec<(String, String, String)>) = node_proxy
        .method_call(
            "org.eclipse.bluechi.Node",
            "EnableUnitFiles",
            (args.unit_names, false, false),
        )?;

    if carries_install_info {
        println!("The unit files included enablement information");
    } else {
        println!("The unit files did not include any enablement information");
    }

    for (op_type, file_name, file_dest) in changes {
        if op_type == "symlink" {
            println!("Created symlink {} -> {}", file_name, file_dest);
        } else if op_type == "unlink" {
            println!("Removed '{}'", file_name);
        }
    }

    Ok(())
}
