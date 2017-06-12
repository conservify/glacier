EESchema Schematic File Version 2
LIBS:power
LIBS:device
LIBS:transistors
LIBS:conn
LIBS:linear
LIBS:regul
LIBS:74xx
LIBS:cmos4000
LIBS:adc-dac
LIBS:memory
LIBS:xilinx
LIBS:microcontrollers
LIBS:dsp
LIBS:microchip
LIBS:analog_switches
LIBS:motorola
LIBS:texas
LIBS:intel
LIBS:audio
LIBS:interface
LIBS:digital-audio
LIBS:philips
LIBS:display
LIBS:cypress
LIBS:siliconi
LIBS:opto
LIBS:atmel
LIBS:contrib
LIBS:valves
LIBS:high-level
EELAYER 25 0
EELAYER END
$Descr A4 11693 8268
encoding utf-8
Sheet 2 3
Title ""
Date ""
Rev ""
Comp ""
Comment1 ""
Comment2 ""
Comment3 ""
Comment4 ""
$EndDescr
$Comp
L accelerometer U?
U 1 1 593F1348
P 7450 4350
F 0 "U?" H 7750 4500 60  0000 C CNN
F 1 "accelerometer" H 7450 4350 60  0000 C CNN
F 2 "" H 7450 4350 60  0001 C CNN
F 3 "" H 7450 4350 60  0001 C CNN
	1    7450 4350
	1    0    0    -1  
$EndComp
$Comp
L charge-controller U?
U 1 1 593F135F
P 7200 2450
F 0 "U?" H 7350 2550 60  0000 C CNN
F 1 "charge-controller" H 7000 2650 60  0000 C CNN
F 2 "" H 7200 2450 60  0001 C CNN
F 3 "" H 7200 2450 60  0001 C CNN
	1    7200 2450
	1    0    0    -1  
$EndComp
$Comp
L geophone U?
U 1 1 593F1378
P 7450 5700
F 0 "U?" H 7400 5600 60  0000 C CNN
F 1 "geophone" H 7450 5700 60  0000 C CNN
F 2 "" H 7450 5700 60  0001 C CNN
F 3 "" H 7450 5700 60  0001 C CNN
	1    7450 5700
	1    0    0    -1  
$EndComp
$Comp
L power-bus U?
U 1 1 593F13B3
P 4200 2300
F 0 "U?" H 4150 2200 60  0000 C CNN
F 1 "power-bus" H 4200 2300 60  0000 C CNN
F 2 "" H 4200 2300 60  0001 C CNN
F 3 "" H 4200 2300 60  0001 C CNN
	1    4200 2300
	-1   0    0    1   
$EndComp
$Comp
L raspberry-pi U?
U 1 1 593F13FA
P 4450 4950
F 0 "U?" H 4450 4850 60  0000 C CNN
F 1 "raspberry-pi" H 4450 4950 60  0000 C CNN
F 2 "" H 4450 4950 60  0001 C CNN
F 3 "" H 4450 4950 60  0001 C CNN
	1    4450 4950
	1    0    0    -1  
$EndComp
$Comp
L usb-eth-adapter U?
U 1 1 593F1495
P 4350 4350
F 0 "U?" H 4350 4250 60  0000 C CNN
F 1 "usb-eth-adapter" H 4350 4350 60  0000 C CNN
F 2 "" H 4350 4350 60  0001 C CNN
F 3 "" H 4350 4350 60  0001 C CNN
	1    4350 4350
	-1   0    0    1   
$EndComp
$Comp
L wireless-bridge U?
U 1 1 593F1504
P 4550 5750
F 0 "U?" H 4550 5650 60  0000 C CNN
F 1 "wireless-bridge" H 4550 5750 60  0000 C CNN
F 2 "" H 4550 5750 60  0001 C CNN
F 3 "" H 4550 5750 60  0001 C CNN
	1    4550 5750
	1    0    0    -1  
$EndComp
$Comp
L hard-drive U?
U 1 1 593F15BD
P 4400 3800
F 0 "U?" H 4400 3700 60  0000 C CNN
F 1 "hard-drive" H 4400 3800 60  0000 C CNN
F 2 "" H 4400 3800 60  0001 C CNN
F 3 "" H 4400 3800 60  0001 C CNN
	1    4400 3800
	1    0    0    -1  
$EndComp
$Comp
L hard-drive U?
U 1 1 593F16E0
P 4400 3350
F 0 "U?" H 4400 3250 60  0000 C CNN
F 1 "hard-drive" H 4400 3350 60  0000 C CNN
F 2 "" H 4400 3350 60  0001 C CNN
F 3 "" H 4400 3350 60  0001 C CNN
	1    4400 3350
	1    0    0    -1  
$EndComp
Text Label 5150 2050 0    60   ~ 0
5v
Text Label 5100 2150 0    60   ~ 0
15v
Text Label 5000 2250 0    60   ~ 0
12v#1
Text Label 5000 2350 0    60   ~ 0
12v#2
Text Label 6150 4550 0    60   ~ 0
12v#1
$Comp
L batteries U?
U 1 1 593F2508
P 6700 1800
F 0 "U?" H 6700 1700 60  0000 C CNN
F 1 "batteries" H 6700 1800 60  0000 C CNN
F 2 "" H 6700 1800 60  0001 C CNN
F 3 "" H 6700 1800 60  0001 C CNN
	1    6700 1800
	1    0    0    -1  
$EndComp
Text Label 3400 4750 0    60   ~ 0
5v
Wire Wire Line
	3400 5650 3700 5650
Wire Wire Line
	5150 4200 6500 4200
Wire Wire Line
	4950 2050 5250 2050
Wire Wire Line
	5250 2150 4950 2150
Wire Wire Line
	5250 2250 4950 2250
Wire Wire Line
	5250 2350 4950 2350
Wire Wire Line
	4950 2450 5950 2450
Wire Wire Line
	5950 2250 5800 2250
Wire Wire Line
	5800 2250 5800 1800
Wire Wire Line
	5800 1800 5950 1800
Wire Wire Line
	3400 4750 3700 4750
Wire Wire Line
	3400 4850 3700 4850
Wire Wire Line
	3400 4950 3700 4950
Wire Wire Line
	3400 5050 3700 5050
Wire Wire Line
	3400 5150 3700 5150
Text Label 3400 4850 0    60   ~ 0
usb1
Text Label 3400 4950 0    60   ~ 0
usb2
Text Label 3400 5050 0    60   ~ 0
usb3
Text Label 3400 5150 0    60   ~ 0
usb4
Wire Wire Line
	3400 3700 3700 3700
Wire Wire Line
	3400 3900 3700 3900
Wire Wire Line
	3400 3450 3700 3450
Wire Wire Line
	3400 3250 3700 3250
Text Label 3500 3900 2    60   ~ 0
5v
Text Label 3400 3450 0    60   ~ 0
5v
Text Label 3400 3250 0    60   ~ 0
usb2
Text Label 3400 3700 0    60   ~ 0
usb1
Text Label 3400 4500 0    60   ~ 0
usb3
Wire Wire Line
	3400 4500 3700 4500
Wire Wire Line
	3700 5250 3400 5250
Text Label 3400 5250 0    60   ~ 0
eth0
Text Label 3400 5650 0    60   ~ 0
eth0
Wire Wire Line
	3700 5550 3400 5550
Text Label 3400 5550 0    60   ~ 0
12v#2
Text Label 6150 4450 0    60   ~ 0
usb4
Text Label 6250 5900 0    60   ~ 0
15v
Wire Wire Line
	6250 5900 6500 5900
Wire Wire Line
	6250 5600 6500 5600
Wire Wire Line
	6250 5500 6500 5500
Wire Wire Line
	6500 5800 6000 5800
Wire Wire Line
	5800 4400 5800 4300
Wire Wire Line
	5800 4300 6500 4300
$Comp
L gps U?
U 1 1 593F4001
P 5600 5800
F 0 "U?" H 5600 5750 60  0000 C CNN
F 1 "gps" H 5600 5850 60  0000 C CNN
F 2 "" H 5600 5850 60  0001 C CNN
F 3 "" H 5600 5850 60  0001 C CNN
	1    5600 5800
	-1   0    0    1   
$EndComp
$Comp
L gps U?
U 1 1 593F40B7
P 5800 4800
F 0 "U?" H 5800 4750 60  0000 C CNN
F 1 "gps" H 5800 4850 60  0000 C CNN
F 2 "" H 5800 4850 60  0001 C CNN
F 3 "" H 5800 4850 60  0001 C CNN
	1    5800 4800
	0    1    1    0   
$EndComp
$Comp
L geophone-sensor U?
U 1 1 593F420B
P 7150 5150
F 0 "U?" H 7150 5100 60  0000 C CNN
F 1 "geophone-sensor" H 7150 5200 60  0000 C CNN
F 2 "" H 7150 5200 60  0001 C CNN
F 3 "" H 7150 5200 60  0001 C CNN
	1    7150 5150
	1    0    0    -1  
$EndComp
Wire Wire Line
	6250 5500 6250 5150
Wire Wire Line
	6250 5150 6500 5150
Wire Wire Line
	6150 4450 6500 4450
Wire Wire Line
	6150 4550 6500 4550
Text Label 5150 4300 0    60   ~ 0
usb5
Text Label 5150 4400 0    60   ~ 0
usb6
Wire Wire Line
	5150 4300 5400 4300
Wire Wire Line
	5400 4400 5150 4400
Text Label 6250 5600 0    60   ~ 0
usb5
Text Label 5700 2550 0    60   ~ 0
usb6
Wire Wire Line
	5700 2550 5950 2550
$Comp
L solar-panels U?
U 1 1 593F50F1
P 6700 1350
F 0 "U?" H 6700 1300 60  0000 C CNN
F 1 "solar-panels" H 6700 1400 60  0000 C CNN
F 2 "" H 6700 1400 60  0001 C CNN
F 3 "" H 6700 1400 60  0001 C CNN
	1    6700 1350
	1    0    0    -1  
$EndComp
Wire Wire Line
	5950 2350 5650 2350
Wire Wire Line
	5650 2350 5650 1350
Wire Wire Line
	5650 1350 5950 1350
$EndSCHEMATC
