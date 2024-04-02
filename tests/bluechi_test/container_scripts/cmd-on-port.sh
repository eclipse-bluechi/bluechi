#!/bin/bash -e
#
# Copyright Contributors to the Eclipse BlueChi project
# SPDX-License-Identifier: LGPL-2.1-or-later

# The script takes only one parameter, which should be the port number
if [ -z "$1" ]; then
    exit 1
fi

PID="$(lsof -i:$1 -t)"

if [ -z "$PID" ]; then
    # No process listens on the specified port
    exit 2
fi

# Return the full command line of the process listening on the specified port/protocol
ps -hww -o args -p ${PID}
