#!/usr/bin/env python
from setuptools import setup, find_packages


def readme():
    with open("README.md") as desc:
        return desc.read()


setup(
    name="hirte",
    version="1.0.0",
    description="Python wrapper implementation for Hirte.",
    long_description=readme(),
    author="Hirte developers",
    url="https://github.com/containers/hirte/",
    license="GPLv2",
    install_requires=[
        "dasbus",
    ],
    package_dir={"": "."},
    packages=find_packages("."),
    include_package_data=True,
    package_data={"hirte": ["py.typed"]},
    zip_safe=True,
    keywords="hirte python",
    classifiers=[
        "Programming Language :: Python",
        "Programming Language :: Python :: 3.7",
        "Programming Language :: Python :: 3.8",
        "Programming Language :: Python :: 3.9",
        "Programming Language :: Python :: 3.10",
    ],
)
