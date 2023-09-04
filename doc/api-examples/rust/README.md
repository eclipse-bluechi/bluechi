# Rust
<!-- markdownlint-disable-file MD013 MD051 -->
* [Rust](#rust)
  * [Generating bluechi module](#generating-bluechi-module)
    * [Generated functions](#generated-functions)
    * [Importing module](#importing-module)

Rust developers can take advance of **dbus-codegen-rust** to generate a Rust module
and use,tag,release in any pace it's appropriate to their project.

## Generating bluechi module

Make sure a BlueChi node is running and execute the below commands.

``` bash
# dnf install rust cargo
# cargo install dbus-codegen
# dbus-codegen-rust -s -g -m None -d org.eclipse.bluechi -p /org/eclipse/bluechi > bluechi.rs
# mv bluechi.rs src/
```

### Generated functions

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

### Importing module

As every project is different, here a tradicional example of importing the generated module in a **main.rs**:

**Dir structure**:

``` bash
.
├── Cargo.toml
└── src
    ├── bluechi.rs
    └── main.rs
```

As soon the files **bluechi.rs** and [main.rs](./src/main.rs) were copies to `src/` it's possible to compile a new binary.

**Generating the binary**:

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
