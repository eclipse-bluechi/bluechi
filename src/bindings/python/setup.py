#!/usr/bin/env python
# SPDX-License-Identifier: GPL-2.0-or-later
from setuptools import setup, find_packages


def readme():
    with open("README.md") as desc:
        return desc.read()


setup(
    name="hirte",
    version="0.1.0",
    description="Python wrapper implementation for Hirte.",
    long_description=readme(),
    author="Hirte developers",
    url="https://github.com/containers/hirte/",
    license="GPL-2.0-or-later",
    install_requires=[
        "dasbus",
    ],
    package_dir={":": "src/bindings/python"},
    packages=find_packages(include=['hirte', 'hirte.src.python']),
    include_package_data=True,
    package_data={"hirte": ["py.typed"]},
    zip_safe=True,
    keywords=['hirte', 'python'],
    classifiers=[
        "Programming Language :: Python",
        "Programming Language :: Python :: 3.9",
        "Programming Language :: Python :: 3.10",
        "Programming Language :: Python :: 3.11",
    ],
)
