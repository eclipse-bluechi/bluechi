# Eclipse BlueChi&trade; documentation pages

Here are stored the source for BlueChi's documentation.

This documentation is built using [mkdocs](https://www.mkdocs.org/) and thus
relies on [markdown](https://daringfireball.net/projects/markdown/) for its
syntax.

## Building the doc locally

We recommend using python's virtual environment to build the doc locally should
you need or want to.

Install python's virtualenvwrapper:

```shell
dnf install python3-virtualenvwrapper
```

Initiate the virtual environment:

```shell
mkvirtualenv mkdocs
```

**Note:** you may have to restart your shell for this command to become available.

Install the dependencies used for building the documentation:

```shell
pip install -r requirements.txt
```

You can then preview the documentation using:

```shell
mkdocs serve -f doc/mkdocs.yml
```

**Note**: we recommend that you run `mkdocs` from the top level directory.

## Some quick command for virtualenvwrapper

* Create a new virtual environment: `mkvirtualenv`

* Leave a running virtual environment: `deactivate`

* List the existing virtual environment: `workon`

* Activate an existing virtual environment: `workon <virtual env name>`
