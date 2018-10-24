#!/bin/bash
cd ../	# for run python script
python "getVersion.py"
cd firmware	# for run build
perl /usr/local/bin/makepp BUILD_MODE=debug BUILD_PORT=target-device-rev1 -j4 build