#!/bin/bash -xe
#
# Copyright Contributors to the Eclipse BlueChi project
#
# SPDX-License-Identifier: LGPL-2.1-or-later

SCRIPT_DIR=$( realpath "$0"  )
SCRIPT_DIR=$(dirname "$SCRIPT_DIR")/
BINDINGS_DIR=$SCRIPT_DIR"../src/bindings/"

generator=$BINDINGS_DIR"generator/src/generator.py"
data_dir=$SCRIPT_DIR"../data/"

function python(){
    # Generate python bindings
    template_dir=$BINDINGS_DIR"/python/templates/"
    output_file_path=$BINDINGS_DIR"/python/bluechi/api.py"
    python3 "$generator" "$data_dir" "$template_dir" "$output_file_path"
    isort "$output_file_path"
    black "$output_file_path"
}

echo "Generating bindings for $1"
echo ""
$1
