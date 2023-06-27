# hirte bindings

This directory contains bindings for the D-Bus API of hirte. The [API description](./../../data/) of hirte is used to
generate basic client stub code that provides the complete set of functionality. Based on the generated code, custom
functions are written to simplify recurring tasks.

## Directory structure

- [bindings/generator](./generator/): small python project used to generate client code for the D-Bus API
- [bindinds/python](./python/): python client for hirte
- [bindings/examples](./examples/): contains example code for using the bindings

## Generating code

For (re-)generating client stubs (e.g. due to a change in the API), the generator can be invoked directly:

```sh
generator/src/generator.py <data-dir> <template-dir> <language> <out-dir>
```

Currently, the following `language`s are supported:

- python

To simplify this, the [build.sh](./build.sh) contains compact functions to generate (as well as to build and install)
pyhirte code. For example

```sh
bash build.sh generate_python_bindings
```

will (re-)generate the python bindings and output them to [./python/pyhirte/gen/](./python/pyhirte/gen/).
