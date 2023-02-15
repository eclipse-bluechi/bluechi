# Hirte testing e2e

## Environment Setup

E2E tests rely on containers as separate compute entities,
containers simulate hirte functional behaviour
on a single runner.

Test directory contains two container files

- Containerfile-build
- Containerfile

Containerfile-build output image is published to
[AutoSD registry](registry.gitlab.com/centos/automotive/sample-images).
Usage: Container hirte base image (contains centosstream9 base image,
systemd and devel packages), build hirte binaries.

Containerfile output image, hirte-images, layered above hirte-base image installed
with hirte compiled products and configurations for e2e testing.

Base workflows

- Create and run simulated containers with systemd
- Check hirte-agents registration complete
