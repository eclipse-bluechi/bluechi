# Hirte testing e2e

## Environment Setup

E2E tests rely on containers as seprate compute entities,
Use of containers enble to simulaute hirte functional behaviour
on single compute.

Test directory contains two container files

- Containerfile-build
- Containerfile

Containerfile-build published in AutoSD registry
registry.gitlab.com/centos/automotive/sample-images.
It is used for hirte binary build on each PR,

Containerfile create hirte-images contain systemd init to simulate
systemd modules which are part of hirte solution

Base workflows

- Create and run simulated containers with systemd
- Check Nodes registration complete