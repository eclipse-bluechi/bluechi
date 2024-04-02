#
# Copyright Contributors to the Eclipse BlueChi project
#
# SPDX-License-Identifier: LGPL-2.1-or-later

import sys
from pathlib import Path
from typing import List, Dict, Any

from xml_parser import list_api_files, parse_api_file
from template import model_to_data_dict, render
from model import Interface


def read_api_files(dir: str) -> List[Interface]:
    interfaces: List[Interface] = []
    for api_file in list_api_files(dir):
        parsed_file = parse_api_file(api_file)
        if len(parsed_file) != 1:
            raise Exception(
                f"Expected only one interface per file, but got {len(parsed_file)}"
            )
        interfaces.append(parsed_file[0])
    return interfaces


def generate(template_dir: str, output_file_path: str, data: Dict[str, Any]):
    """Generate typed python code based on the provided templates and data and writing it to the output file.

    Parameters:
    template_dir: The template directory containing the root template "api.tmpl" that will be used for rendering
    output_file_path: The output file (path + name of the file)
    data: The data dictionary passed to the Jinja2 template rendering

    """
    path = Path(output_file_path)

    if path.is_dir():
        print("Output file path needs to include file name!")
        return

    # raise exception if directory does not exist
    path.parent.is_dir()

    with open(output_file_path, "w") as outf:
        outf.write(render("api.tmpl", template_dir, data))


if __name__ == "__main__":
    if len(sys.argv) != 4:
        print("Mismatching arguments!")
        print("")
        print("Usage:")
        print("   generator.py <data-dir> <template-dir> <out-file-path>")
        print("")
        print("   supported languages: [python]")

        raise Exception("Mismatching arguments!")

    _, data_dir, template_dir, output_file_path = sys.argv
    print(f"Data directory: {data_dir}")
    print(f"Template directory: {template_dir}")
    print(f"Output file path: {output_file_path}")

    interfaces = read_api_files(data_dir)
    data = model_to_data_dict(interfaces)

    generate(template_dir, output_file_path, data)
