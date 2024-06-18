# Contributing to Eclipse BlueChi&trade;

Thank you for your interest in Eclipse BlueChi.

## Project description

[Eclipse BlueChi](https://projects.eclipse.org/projects/automotive.bluechi) is a deterministic multi-node service
controller with a focus on highly regulated ecosystems. It is written in C and integrates seamless with
systemd via its D-Bus API relaying D-Bus messages over TCP for multi-node
support. BlueChi is built around three components:

- bluechi-controller
- bluechi-agent
- bluechictl

## Testing

RPM packages for the BlueChi project are available on
[bluechi-snapshot](https://copr.fedorainfracloud.org/coprs/g/centos-automotive-sig/bluechi-snapshot/)
COPR repo. To install BlueChi packages on your system please add that repo using:

```bash
dnf copr enable @centos-automotive-sig/bluechi-snapshot
```

When done you can install relevant BlueChi packages using:

```bash
dnf install bluechi bluechi-agent bluechi-ctl
```

## Found a bug or documentation issue?

To submit a bug or suggest an enhancement please use [GitHub issues](https://github.com/eclipse-bluechi/bluechi/issues).

## Submitting patches

Patches are welcome!

Please submit patches to [github.com/eclipse-bluechi/bluechi](https://github.com/eclipse-bluechi/bluechi).
More information about the development can be found in [README.developer.md](README.developer.md).

You can read [Get started with GitHub](https://docs.github.com/en/get-started)
if you are not familiar with the development process to learn more about it.

## Developer resources

Information regarding source code management, builds, coding standards, and
more can be found in the [README.developer.md](./README.developer.md).

## Eclipse Contributor Agreement

In order to be able to contribute to Eclipse Foundation projects you must
electronically sign the [Eclipse Contributor Agreement (ECA)](https://www.eclipse.org/legal/ECA.php).

The ECA provides the Eclipse Foundation with a permanent record that you agree
that each of your contributions will comply with the commitments documented in
the Developer Certificate of Origin (DCO). Having an ECA on file associated with
the email address matching the "Author" field of your contribution's Git commits
fulfills the DCO's requirement that you sign-off on your contributions.

For more information, please see the [Eclipse Committer Handbook](https://www.eclipse.org/projects/handbook/#resources-commit)

## Need help?

If you need help, please join the following mailing lists:

- [Eclipse BlueChi development](https://accounts.eclipse.org/mailing-list/bluechi-dev)
- [CentOS Automotive SIG](https://lists.centos.org/postorius/lists/automotive-sig.lists.centos.org/)
