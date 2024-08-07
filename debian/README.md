# DEB Packaging of BlueChi

In order to create a `.deb` package for BlueChi, first checkout the branch or tag for which the package should be
built and update the [changelog](./changelog).

Next, build the following container image from the BlueChi directory:

```bash
podman build -t bluechi-deb-build -f debian/bluechi-deb-build .
```

Subsequently, run that container and mount the BlueChi directory into it:

```bash
podman run -it -v <path-to-bluechi>:/var/bluechi-build/bluechi bluechi-deb-build /bin/bash
```

Inside the container create the tarball, move it to the parent directory and rename it in the same go to the latest
entry in [changelog](./changelog):

```bash
cd /var/bluechi-build/bluechi
./build-scripts/create-archive.sh
mv bluechi-<version>.tar.gz ../bluechi_<version>.orig.tar.gz
```

In the BlueChi directory run:

```bash
meson setup builddir
dpkg-buildpackage -uc -us -rfakeroot
```

This will produce the desired `.deb` packages in the parent directory `/var/bluechi-build`. Be sure to move them to
the mounted `/var/bluechi-build/bluechi`.

## PPA for BlueChi

The built `.deb` packages for new Releases of BlueChi are hosted in a GitHub repository. How to use it and easily
install BlueChi, please refer to the [bluechi-ppa README](https://github.com/eclipse-bluechi/bluechi-ppa/).
