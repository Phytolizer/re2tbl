#!/bin/sh

cd "${MESON_SOURCE_ROOT}/doc"
doxygen && make html
