# agent configuration directory

This directory is used for user configuration of `bluechi-agent`.
Each configuration file should end with `.conf` suffix, otherwise it's ignored.
Configuration files from this directory are loaded by alphabetical order,
configuration from a file loaded later overwrites the previous configuration.

It's suggested to name configuration files using `<NN>-<FILE NAME>.conf` template,
where `<NN>` is a number, for example `95-debug-logging.conf`.
