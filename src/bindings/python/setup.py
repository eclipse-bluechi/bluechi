#
# Copyright Contributors to the Eclipse BlueChi project
#
# SPDX-License-Identifier: LGPL-2.1-or-later
from setuptools import setup, find_packages


def readme():
    with open("README.md") as desc:
        return desc.read()


setup(
    name="bluechi",
    version="0.9.0",
    description="Python bindings for BlueChi's D-Bus API",
    long_description=readme(),
    long_description_content_type="text/markdown",
    author="BlueChi developers",
    url="https://github.com/eclipse-bluechi/bluechi/",
    license="LGPL-2.1-or-later",
    install_requires=[
        "dasbus",
    ],
    packages=find_packages(),
    include_package_data=True,
    package_data={"bluechi": ["py.typed"]},
    zip_safe=True,
    keywords=['bluechi', 'python', 'D-Bus', 'systemd'],
    classifiers=[
        "Programming Language :: Python",
        "Programming Language :: Python :: 3",
    ],
    python_requires='>=3.9',
)
