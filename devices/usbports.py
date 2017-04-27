#!/usr/bin/python

import os
import glob
import subprocess
import sys
import re

def parse_env_section(raw):
	values = {}
	for key, value in re.findall('(\S+)=\'(.+)\'', raw):
		values[key] = value
	return values

def find_usb_ttys(matching):
	for usb_dir in glob.glob('/sys/bus/usb/devices/usb*/'):
		for root, dirs, files in os.walk(usb_dir):
			if 'dev' in files:
				sys_path = root.rstrip('/')
				raw_env = subprocess.check_output('udevadm info -q property --export -p ' + sys_path, shell=True)
				env = parse_env_section(raw_env)
				if env['SUBSYSTEM'] == 'tty':
					if not matching or re.match(matching, env['ID_SERIAL']):
						return env['DEVNAME']
	return None

