# Subprojects used by hirte

This directory contains all external subprojects used by hirte as git submodules. These are integrated into the main
project using mesons [subproject feature](https://mesonbuild.com/Subprojects.html).

Currently, hirte uses the following external projects:

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

And, finally, add and commit the changes. The same procedure can be used to upgrade a submodule to a newer tag.
