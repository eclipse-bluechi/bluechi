<!-- markdownlint-disable-file MD013 -->
# Rust

Welcome to the BlueChi Rust module! \o/

## Generating bluechi module

``` bash
# dnf install rust cargo
# cargo install dbus-codegen
# dbus-codegen-rust -s -g -m None -d org.eclipse.bluechi -p /org/eclipse/bluechi > bluechi.rs
# mv bluechi.rs src/
```

### What's the functions names to be called?

In Sept 2023, the generator output was:

``` bash
pub trait OrgEclipseBluechiManager {
    fn ping(&self, arg0: &str) -> Result<String, dbus::Error>;
    fn list_units(&self) -> Result<Vec<(String, String, String, String, String, String, String, dbus::Path<'static>, u32, String, dbus::Path<'static>)>, dbus::Error>;
    fn list_nodes(&self) -> Result<Vec<(String, dbus::Path<'static>, String)>, dbus::Error>;
    fn get_node(&self, arg0: &str) -> Result<dbus::Path<'static>, dbus::Error>;
    fn create_monitor(&self) -> Result<dbus::Path<'static>, dbus::Error>;
    fn set_log_level(&self, arg0: &str) -> Result<(), dbus::Error>;
    fn enable_metrics(&self) -> Result<(), dbus::Error>;
    fn disable_metrics(&self) -> Result<(), dbus::Error>;
}
```

### How to use it?

See [src/main.rs](./src/main.rs) for details but basically the idea is use the **proxy.FUNCTION_NAME()**.

``` bash
# dnf install rust cargo
# pwd
$HOME/bluechi/src/bindings/rust

# cargo run
    Updating crates.io index
   Compiling pkg-config v0.3.27
   Compiling libc v0.2.147
   Compiling libdbus-sys v0.2.5
   Compiling dbus v0.9.7
   Compiling bluechi v0.1.0 (/root/rust/src/bluechi/src/bindings/rust)
    Finished dev [unoptimized + debuginfo] target(s) in 7.48s
     Running `target/debug/bluechi`
Node: "control"
Status: "online"
Path("/org/eclipse/bluechi/node/control\0")
Node: "node1"
Status: "offline"
Path("/org/eclipse/bluechi/node/node1\0")
```
