# Hirte E2E Testing

## Environment Setup

E2E tests rely on containers as separate compute entities. These containers are used to simulate hirte's functional
behavior on a single runner.

The [tests](./tests/) directory contains two container files.

The `Containerfile-build` file describes the builder base image that is published to
[registry.gitlab.com/centos/automotive/sample-images/hirte/hirte-builder](https://gitlab.com/CentOS/automotive/sample-images/container_registry/3897597).
It contains core dependencies such as systemd and devel packages.

The `Containerfile` is based on the builder base image and contains compiled products and configurations for e2e
testing.

In general testing is conducted in the following way:

1) Instantiate the E2E test environment by creating and running containers with systemd.
2) Check that the registration of `hirte-agent` is complete.
