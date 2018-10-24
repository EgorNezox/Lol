#!/bin/bash
python $PWD "getVersion.py"
cd firmware
perl /usr/local/bin/makepp BUILD_MODE=debug BUILD_PORT=target-device-rev1 -j4