# BlueChi bindings

This directory contains bindings for the D-Bus API of BlueChi. The [API description](./../../data/) of BlueChi is used to
generate basic client stub code that provides the complete set of functionality. Based on the generated code, custom
functions are written to simplify recurring tasks.

## Directory structure

- [bindings/generator](./generator/): small python project used to generate client code for the D-Bus API
- [bindinds/python](./python/): python client for bluechi
