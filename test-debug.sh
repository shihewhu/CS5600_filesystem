#!/bin/bash

./mktest foo.img
make
./homework -cmdline -image foo.img