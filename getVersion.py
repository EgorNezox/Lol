# -*- coding: utf-8 -*-
import subprocess
import os

git = "C:\Program Files\Git\git-cmd.exe"
cmd = "git log -1 --oneline"

def get_version():
	process = subprocess.check_output([git, cmd])
	lines = process.decode('utf-8').split()
	return lines[0]

def add_version_to_file():
	version = get_version()[:7]
	print("Build version: ", version)
	new_line = "#define HOST_VERSION " + '"' + version + '"'  + "\n"
	file_name = "sazhenn.h"
	
	with open(file_name, 'r') as file:
		data = file.readlines()

	data[14] = new_line

	with open(file_name, 'w') as file:
		file.writelines( data )

def main():
	add_version_to_file()

if __name__ == "__main__":
	# execute only if run as a script
	main()