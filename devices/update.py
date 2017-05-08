#!/usr/bin/python

import usbports
import os

def ensure_link(source, link_name):
	if source:
		if os.readlink(link_name) != source:
			os.unlink(link_name)
			os.symlink(source, link_name)
	elif os.path.islink(link_name):
		os.unlink(link_name)

def main():
	obsidian = usbports.find_usb_ttys('Prolific')
	adc = usbports.find_usb_ttys('FTDI')

	obsidian_link = "/tmp/obsidian"
	adc_link = "/tmp/adc"

	ensure_link(obsidian, "/tmp/obsidian")
	ensure_link(adc, "/tmp/adc")

if __name__ == "__main__":
	main()
