# Hirte

Hirte is a service orchestrator tool intended for multi-node devices (e.g.: edge devices) clusters
with a predefined number of nodes and with a focus on highly regulated environment such as those requiring functional
safety (for example in cars).

Hirte is relying on [systemd](https://github.com/systemd/systemd) and its D-Bus API, which
it extends for multi-node use case.

Hirte can also be used to orchestrate containers using [podman](https://github.com/containers/podman/) and its ability
to generate systemd service configuration to run a container.

## How to contribute

### Submitting patches

Patches are welcome!

Please submit patches to [github.com:containers/hirte](https://github.com/containers/hirte).
More information about the development can be found in [README.developer](https://github.com/containers/hirte/README.developer.md).

If you are not familiar with the development process you can read about it in
[Get started with GitHub](https://docs.github.com/en/get-started).


### Found a bug or documentation issue?
To submit a bug or suggest an enhancement for hirte please use
[GitHub issues](https://github.com/containers/hirte/issues).


## Still need help?
If you have any other questions, please join [CentOS Automotive SIG mailing list](https://lists.centos.org/mailman/listinfo/centos-automotive-sig/) and ask there.


