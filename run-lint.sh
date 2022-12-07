#!/bin/bash
find . -name "*.[ch]" ! -path "./src/ini/*" ! -path "./test/**" -print0 | xargs -0 -n 1 -I {} clang-tidy --quiet {} -- -D_GNU_SOURCE
