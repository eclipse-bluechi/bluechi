# BlueChi

## Creating a new release

Following actions need to be taken to release a new stable version of BlueChi:

1. Creating and merging a release commit
2. Tagging a release commit
3. Creating and merging a post release commit

### Creating and merging a release commit

Following files need to be updated to create a new stable release commit:

* `RELEASE` variable needs to be updated in `build-scripts/build-srpm.sh` to contain a number instead of date and git
  hash, which are used for snapshot builds. The number should contain `1` for the first build of the release, but it
  may be increased if we need additional package rebuilds.
* A new `%changelog` section in `bluechi.spec.in` should be added and it should contain information about the new package
  release

Here is the example of the release commit for version
[0.1.0](https://github.com/containers/bluechi/pull/214).

### Tagging a release commit

When the release commit is merged, we need to tag the release commit to be able to create an official release visible
on the GitHub [project pages](https://github.com/containers/bluechi/releases).

The tag should use `v<VERSION>` format, where `<VERSION>` is the version of the release we want to create (for example
the tag should be `v0.1.0` for the 0.1.0 version). There are 2 ways to create a release tag:

* Using GitHub web UI, when creating a release (see following section)
* Using standard git commands
  1. Create a release tag pointing to the release commit:

     ```shell
     git tag v<VERSION> <GIT HASH>
     ```

     where `v<VERSION` is the release tag and `<GIT HASH>` is the git has of the relevant release commit.
  2. Push the commit to the repo:

     ``` shell
     git push origin v<VERSION>
     ```

     where `v<VERSION>` is the new release tag.

**WARNING**: If a release tag has a different format, source archive for the release will have a different structure,
which would cause RPM build for Fedora to fail.

Pushing the new tag to the repository will trigger a GitHub workflow which automatically creates a new release. It will
be listed on the [release page](https://github.com/containers/bluechi/releases) of BlueChi. Assets such as the .src.rpm
are generated and added to that release by the workflow. In addition to the generated release notes, its good to
manually add a **Highlights** section describing the most important changes.

### Creating and merging a post release commit

Following files need to be updated to switch the build from release build back to snapshot build:

* `RELEASE` variable needs to be updated in `build-scripts/build-srpm.sh` to contain a date and git hash instead of
  a number.
* Project version needs to be increased in `meson.build`

Here is the example of the post release commit for version
[0.1.0](https://github.com/containers/bluechi/pull/215).

**WARNING:** There should not be merged any commits to the project between the release commit and the post release
commit to make things clear.
