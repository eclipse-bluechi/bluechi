# Subprojects used by BlueChi

This directory contains all external subprojects used by BlueChi as git submodules. These are integrated into the main
project using mesons [subproject feature](https://mesonbuild.com/Subprojects.html).

Currently BlueChi uses the following external projects:

- [hashmap.c](https://github.com/engelmi/hashmap.c.git)
- [inih](https://github.com/benhoyt/inih.git)

Unfortunately, `hashmap.c` doesn't provide a `meson.build` file and patching on the fly would result in uncommitted
changes. Because of that a fork has been created containing the necessary `meson.build` and is used for now.

## Adding submodules

New submodules can be added via:

```bash
git submodule add <repository url> subprojects/<repository name>
```

After adding the new repository, checkout the desired tag to be used:

```bash
git -C subprojects/<repository name> checkout <tag>
```

For the new subproject to be usable by BlueChi, it needs to be integrated in the main [meson.build](../meson.build). One
requirement for this is that the subproject itself uses meson and provides a `meson.build`. If the subproject itself is
a `meson` project, include it as [subproject](https://mesonbuild.com/Subprojects.html). The included `inih` or
`hashmap.c` can serve as a reference here. Otherwise the `subdir` function can be used (similar to bluechi internal projects).

And, finally, add and commit the changes. The same procedure can be used to upgrade a submodule to a newer tag.
