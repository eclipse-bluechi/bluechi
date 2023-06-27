#!/bin/bash -xe
# SPDX-License-Identifier: GPL-2.0-or-later

CURR_DIR=$( realpath "$0"  )
CURR_DIR=$(dirname "$CURR_DIR")

generate_python_bindings () {
    generator=$CURR_DIR"/generator/src/generator.py"
    data_dir=$CURR_DIR"/../../data/"
    template_dir=$CURR_DIR"/generator/templates/python"
    output_dir=$CURR_DIR"/python/pyhirte"
    output_lang="python"

    python3 $generator $data_dir $template_dir $output_lang $output_dir
    black $output_dir
}

build_python_package () {
    cd $CURR_DIR"/python"
    python -m build
}

install_python_package () {
    build_python_package
    pip install --force dist/pyhirte-0.1.0-py3-none-any.whl        
}

uninstall_python_package () {
    pip uninstall pyhirte
}

echo "Running: "$1
echo ""
eval $1
