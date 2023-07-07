# SPDX-License-Identifier: GPL-2.0-or-later
from setuptools import setup, find_packages


def readme():
    with open("README.md") as desc:
        return desc.read()


setup(
    name="pyhirte",
    version="0.4.0",
    description="Python bindings for hirte's D-Bus API",
    long_description=readme(),
    author="Hirte developers",
    url="https://github.com/containers/hirte/",
    license="LGPL-2.0-or-later",
    install_requires=[
        "dasbus",
    ],
    packages=find_packages(),
    include_package_data=True,
    package_data={"hirte": ["py.typed"]},
    zip_safe=True,
    keywords=['hirte', 'python', 'D-Bus', 'systemd'],
    classifiers=[
        "Programming Language :: Python",
        "Programming Language :: Python :: 3",
    ],
    python_requires='>=3.9',
)