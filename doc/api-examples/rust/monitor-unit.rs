// Copyright Contributors to the Eclipse BlueChi project
//
// SPDX-License-Identifier: MIT-0

use clap::Parser;
use dbus::{arg, blocking::Connection, Message, Path};
use std::time::Duration;

pub struct UnitNewSignal {
    pub node: String,
    pub unit: String,
    pub reason: String,
}

impl arg::AppendAll for UnitNewSignal {
    fn append(&self, i: &mut arg::IterAppend) {
        arg::RefArg::append(&self.node, i);
    }
}

impl arg::ReadAll for UnitNewSignal {
    fn read(i: &mut arg::Iter) -> Result<Self, arg::TypeMismatchError> {
        Ok(UnitNewSignal {
            node: i.read()?,
            unit: i.read()?,
            reason: i.read()?,
        })
    }
}

impl dbus::message::SignalArgs for UnitNewSignal {
    const NAME: &'static str = "UnitNew";
    const INTERFACE: &'static str = "org.eclipse.bluechi.Monitor";
}

pub struct UnitRemovedSignal {
    pub node: String,
    pub unit: String,
    pub reason: String,
}

impl arg::AppendAll for UnitRemovedSignal {
    fn append(&self, i: &mut arg::IterAppend) {
        arg::RefArg::append(&self.node, i);
    }
}

impl arg::ReadAll for UnitRemovedSignal {
    fn read(i: &mut arg::Iter) -> Result<Self, arg::TypeMismatchError> {
        Ok(UnitRemovedSignal {
            node: i.read()?,
            unit: i.read()?,
            reason: i.read()?,
        })
    }
}

impl dbus::message::SignalArgs for UnitRemovedSignal {
    const NAME: &'static str = "UnitRemoved";
    const INTERFACE: &'static str = "org.eclipse.bluechi.Monitor";
}

pub struct UnitPropertiesChangedSignal {
    pub node: String,
    pub unit: String,
    pub interface: String,
}

impl arg::AppendAll for UnitPropertiesChangedSignal {
    fn append(&self, i: &mut arg::IterAppend) {
        arg::RefArg::append(&self.node, i);
    }
}

impl arg::ReadAll for UnitPropertiesChangedSignal {
    fn read(i: &mut arg::Iter) -> Result<Self, arg::TypeMismatchError> {
        Ok(UnitPropertiesChangedSignal {
            node: i.read()?,
            unit: i.read()?,
            interface: i.read()?,
        })
    }
}

impl dbus::message::SignalArgs for UnitPropertiesChangedSignal {
    const NAME: &'static str = "UnitPropertiesChanged";
    const INTERFACE: &'static str = "org.eclipse.bluechi.Monitor";
}

pub struct UnitStateChangedSignal {
    pub node: String,
    pub unit: String,
    pub active_state: String,
    pub sub_state: String,
    pub reason: String,
}

impl arg::AppendAll for UnitStateChangedSignal {
    fn append(&self, i: &mut arg::IterAppend) {
        arg::RefArg::append(&self.node, i);
    }
}

impl arg::ReadAll for UnitStateChangedSignal {
    fn read(i: &mut arg::Iter) -> Result<Self, arg::TypeMismatchError> {
        Ok(UnitStateChangedSignal {
            node: i.read()?,
            unit: i.read()?,
            active_state: i.read()?,
            sub_state: i.read()?,
            reason: i.read()?,
        })
    }
}

impl dbus::message::SignalArgs for UnitStateChangedSignal {
    const NAME: &'static str = "UnitStateChanged";
    const INTERFACE: &'static str = "org.eclipse.bluechi.Monitor";
}

#[derive(Parser)]
struct Cli {
    /// The node name to list the units for
    #[clap(short, long)]
    unit_name: String,
}

fn main() -> Result<(), Box<dyn std::error::Error>> {
    let args = Cli::parse();
    let unit_name = args.unit_name;
    let node_name = "*"; // Match all nodes

    let conn = Connection::new_system()?;

    let bluechi = conn.with_proxy(
        "org.eclipse.bluechi",
        "/org/eclipse/bluechi",
        Duration::from_millis(5000),
    );

    let (monitor,): (Path,) =
        bluechi.method_call("org.eclipse.bluechi.Controller", "CreateMonitor", ())?;
    let monitor_proxy =
        conn.with_proxy("org.eclipse.bluechi", monitor, Duration::from_millis(5000));

    let _id =
        monitor_proxy.match_signal(move |signal: UnitNewSignal, _: &Connection, _: &Message| {
            println!(
                "New Unit {} on node {}, reason: {}",
                signal.unit, signal.node, signal.reason
            );
            true
        });

    let _id = monitor_proxy.match_signal(
        move |signal: UnitRemovedSignal, _: &Connection, _: &Message| {
            println!(
                "Removed Unit {} on node {}, reason: {}",
                signal.unit, signal.node, signal.reason
            );
            true
        },
    );

    let _id = monitor_proxy.match_signal(
        move |signal: UnitPropertiesChangedSignal, _: &Connection, _: &Message| {
            println!(
                "Unit {} on node {} changed for iface {}",
                signal.unit, signal.node, signal.interface
            );
            true
        },
    );

    let _id = monitor_proxy.match_signal(
        move |signal: UnitStateChangedSignal, _: &Connection, _: &Message| {
            println!(
                "Unit {} on node {} changed to state({}, {}), reason: {}",
                signal.unit, signal.node, signal.active_state, signal.sub_state, signal.reason
            );
            true
        },
    );

    monitor_proxy.method_call(
        "org.eclipse.bluechi.Monitor",
        "Subscribe",
        (node_name, unit_name),
    )?;

    loop {
        conn.process(Duration::from_millis(1000))?;
    }
}
