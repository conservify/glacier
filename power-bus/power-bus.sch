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
LIBS:conservify
LIBS:power-bus-cache
EELAYER 25 0
EELAYER END
$Descr A4 11693 8268
encoding utf-8
Sheet 1 1
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
L CONN_01X02 J1
U 1 1 58FA5FA2
P 800 850
F 0 "J1" H 800 1000 50  0000 C CNN
F 1 "CONN_01X02" V 900 850 50  0000 C CNN
F 2 "conservify:TerminalBlock_Phoenix_MKDS1.5-2pol" H 800 850 50  0001 C CNN
F 3 "" H 800 850 50  0001 C CNN
	1    800  850 
	-1   0    0    1   
$EndComp
$Comp
L pololu-vreg U1
U 1 1 58FA5FFD
P 4300 1550
F 0 "U1" H 4300 1650 60  0000 C CNN
F 1 "pololu-vreg" H 4300 1550 60  0000 C CNN
F 2 "conservify:pololu-vreg" H 4300 1550 60  0001 C CNN
F 3 "" H 4300 1550 60  0001 C CNN
	1    4300 1550
	1    0    0    -1  
$EndComp
Text Label 1950 900  0    60   ~ 0
VRAW
Text Label 1300 800  0    60   ~ 0
GND
Text Label 3050 1500 0    60   ~ 0
VRAW
Text Label 3050 1600 0    60   ~ 0
GND
Text Label 3050 1700 0    60   ~ 0
5V0_1
Text Label 3050 1400 0    60   ~ 0
5V0_1_EN
$Comp
L CONN_01X02 J2
U 1 1 58FA6300
P 2300 1650
F 0 "J2" H 2300 1800 50  0000 C CNN
F 1 "CONN_01X02" V 2400 1650 50  0000 C CNN
F 2 "conservify:TerminalBlock_Phoenix_MKDS1.5-2pol" H 2300 1650 50  0001 C CNN
F 3 "" H 2300 1650 50  0001 C CNN
	1    2300 1650
	-1   0    0    1   
$EndComp
Text Label 2550 1700 0    60   ~ 0
5V0_1
Text Label 2550 1600 0    60   ~ 0
GND
$Comp
L pololu-vreg U4
U 1 1 58FA6503
P 4300 3800
F 0 "U4" H 4300 3900 60  0000 C CNN
F 1 "pololu-vreg" H 4300 3800 60  0000 C CNN
F 2 "conservify:pololu-vreg-adjustable" H 4300 3800 60  0001 C CNN
F 3 "" H 4300 3800 60  0001 C CNN
	1    4300 3800
	1    0    0    -1  
$EndComp
Text Label 3050 3750 0    60   ~ 0
VRAW
Text Label 3050 3850 0    60   ~ 0
GND
Text Label 3050 3950 0    60   ~ 0
12V0_2
Text Label 3050 3650 0    60   ~ 0
12V0_2_EN
$Comp
L CONN_01X02 J5
U 1 1 58FA6517
P 2300 3900
F 0 "J5" H 2300 4050 50  0000 C CNN
F 1 "CONN_01X02" V 2400 3900 50  0000 C CNN
F 2 "conservify:TerminalBlock_Phoenix_MKDS1.5-2pol" H 2300 3900 50  0001 C CNN
F 3 "" H 2300 3900 50  0001 C CNN
	1    2300 3900
	-1   0    0    1   
$EndComp
Text Label 2550 3850 0    60   ~ 0
GND
Text Label 2550 3950 0    60   ~ 0
12V0_2
$Comp
L CONN_01X02 J6
U 1 1 58FA673A
P 6000 1650
F 0 "J6" H 6000 1800 50  0000 C CNN
F 1 "CONN_01X02" V 6100 1650 50  0000 C CNN
F 2 "conservify:TerminalBlock_Phoenix_MKDS1.5-2pol" H 6000 1650 50  0001 C CNN
F 3 "" H 6000 1650 50  0001 C CNN
	1    6000 1650
	-1   0    0    1   
$EndComp
Text Label 6250 1700 0    60   ~ 0
VADJ_1
Text Label 6250 1600 0    60   ~ 0
GND
Wire Wire Line
	1600 900  1000 900 
Wire Wire Line
	1600 800  1000 800 
Wire Wire Line
	3050 1400 3550 1400
Wire Wire Line
	3050 1500 3550 1500
Wire Wire Line
	3050 1600 3550 1600
Wire Wire Line
	3050 1700 3550 1700
Wire Wire Line
	2850 1600 2500 1600
Wire Wire Line
	2850 1700 2500 1700
Wire Wire Line
	3050 3650 3550 3650
Wire Wire Line
	3050 3750 3550 3750
Wire Wire Line
	3050 3850 3550 3850
Wire Wire Line
	3050 3950 3550 3950
Wire Wire Line
	2850 3950 2500 3950
Wire Wire Line
	2850 3850 2500 3850
Wire Wire Line
	6550 1600 6200 1600
Wire Wire Line
	6550 1700 6200 1700
$Comp
L GND #PWR01
U 1 1 58FA74B4
P 1050 1800
F 0 "#PWR01" H 1050 1550 50  0001 C CNN
F 1 "GND" H 1050 1650 50  0000 C CNN
F 2 "" H 1050 1800 50  0001 C CNN
F 3 "" H 1050 1800 50  0001 C CNN
	1    1050 1800
	1    0    0    -1  
$EndComp
Text Label 1200 1750 0    60   ~ 0
GND
Wire Wire Line
	1450 1750 1050 1750
Wire Wire Line
	1050 1750 1050 1800
$Comp
L CONN_01X02 J11
U 1 1 58FA8689
P 1400 3900
F 0 "J11" H 1400 4050 50  0000 C CNN
F 1 "CONN_01X02" V 1500 3900 50  0000 C CNN
F 2 "conservify:TerminalBlock_Phoenix_MKDS1.5-2pol" H 1400 3900 50  0001 C CNN
F 3 "" H 1400 3900 50  0001 C CNN
	1    1400 3900
	-1   0    0    1   
$EndComp
Text Label 1650 3850 0    60   ~ 0
GND
Text Label 1650 3950 0    60   ~ 0
12V0_2
Wire Wire Line
	1950 3950 1600 3950
Wire Wire Line
	1950 3850 1600 3850
$Comp
L CONN_01X05 J9
U 1 1 58FE3D5C
P 4550 4750
F 0 "J9" H 4550 5050 50  0000 C CNN
F 1 "CONN_01X05" V 4650 4750 50  0000 C CNN
F 2 "Socket_Strips:Socket_Strip_Straight_1x05_Pitch2.54mm" H 4550 4750 50  0001 C CNN
F 3 "" H 4550 4750 50  0001 C CNN
	1    4550 4750
	1    0    0    -1  
$EndComp
Text Label 3800 4750 0    60   ~ 0
12V0_2_EN
Text Label 3800 4850 0    60   ~ 0
VADJ_1_EN
Text Label 3800 4950 0    60   ~ 0
GND
Wire Wire Line
	3800 4650 4350 4650
Wire Wire Line
	3800 4750 4350 4750
Wire Wire Line
	3800 4850 4350 4850
Wire Wire Line
	3800 4950 4350 4950
$Comp
L Fuse F1
U 1 1 58FE4282
P 1750 900
F 0 "F1" V 1830 900 50  0000 C CNN
F 1 "Fuse" V 1675 900 50  0000 C CNN
F 2 "conservify:Fuseholder6x30_horiz_open_inline_Type-I" V 1680 900 50  0001 C CNN
F 3 "" H 1750 900 50  0001 C CNN
	1    1750 900 
	0    1    1    0   
$EndComp
Wire Wire Line
	2200 900  1900 900 
Text Label 1300 900  0    60   ~ 0
VIN
$Comp
L MOUNT_HOLE M1
U 1 1 59023B33
P 5050 2600
F 0 "M1" H 5050 2750 50  0000 C CNN
F 1 "MOUNT_HOLE" H 5050 2450 50  0001 C CNN
F 2 "conservify:HOLE_NPTH_3.8MM" H 5075 2375 50  0001 C CNN
F 3 "" H 5050 2600 50  0000 C CNN
	1    5050 2600
	1    0    0    -1  
$EndComp
$Comp
L MOUNT_HOLE M2
U 1 1 59023BD7
P 5250 2600
F 0 "M2" H 5250 2750 50  0000 C CNN
F 1 "MOUNT_HOLE" H 5250 2450 50  0001 C CNN
F 2 "conservify:HOLE_NPTH_3.8MM" H 5275 2375 50  0001 C CNN
F 3 "" H 5250 2600 50  0000 C CNN
	1    5250 2600
	1    0    0    -1  
$EndComp
$Comp
L MOUNT_HOLE M3
U 1 1 59023C27
P 5450 2600
F 0 "M3" H 5450 2750 50  0000 C CNN
F 1 "MOUNT_HOLE" H 5450 2450 50  0001 C CNN
F 2 "conservify:HOLE_NPTH_3.8MM" H 5475 2375 50  0001 C CNN
F 3 "" H 5450 2600 50  0000 C CNN
	1    5450 2600
	1    0    0    -1  
$EndComp
$Comp
L MOUNT_HOLE M4
U 1 1 59023C7E
P 5650 2600
F 0 "M4" H 5650 2750 50  0000 C CNN
F 1 "MOUNT_HOLE" H 5650 2450 50  0001 C CNN
F 2 "conservify:HOLE_NPTH_3.8MM" H 5675 2375 50  0001 C CNN
F 3 "" H 5650 2600 50  0000 C CNN
	1    5650 2600
	1    0    0    -1  
$EndComp
$Comp
L CONN_01X02 J8
U 1 1 59024BCB
P 1400 5000
F 0 "J8" H 1400 5150 50  0000 C CNN
F 1 "CONN_01X02" V 1500 5000 50  0000 C CNN
F 2 "conservify:TerminalBlock_Phoenix_MKDS1.5-2pol" H 1400 5000 50  0001 C CNN
F 3 "" H 1400 5000 50  0001 C CNN
	1    1400 5000
	-1   0    0    1   
$EndComp
Text Label 1750 4950 0    60   ~ 0
VRAW
Text Label 1750 5050 0    60   ~ 0
GND
Wire Wire Line
	2050 4950 1600 4950
Wire Wire Line
	2050 5050 1600 5050
$Comp
L CONN_01X02 J12
U 1 1 5A0A3125
P 1400 4500
F 0 "J12" H 1400 4650 50  0000 C CNN
F 1 "CONN_01X02" V 1500 4500 50  0000 C CNN
F 2 "conservify:TerminalBlock_Phoenix_MKDS1.5-2pol" H 1400 4500 50  0001 C CNN
F 3 "" H 1400 4500 50  0001 C CNN
	1    1400 4500
	-1   0    0    1   
$EndComp
Text Label 1750 4450 0    60   ~ 0
VRAW
Text Label 1750 4550 0    60   ~ 0
GND
Wire Wire Line
	2050 4450 1600 4450
Wire Wire Line
	2050 4550 1600 4550
$Comp
L CONN_01X02 J13
U 1 1 5A0A35B3
P 1400 5500
F 0 "J13" H 1400 5650 50  0000 C CNN
F 1 "CONN_01X02" V 1500 5500 50  0000 C CNN
F 2 "conservify:TerminalBlock_Phoenix_MKDS1.5-2pol" H 1400 5500 50  0001 C CNN
F 3 "" H 1400 5500 50  0001 C CNN
	1    1400 5500
	-1   0    0    1   
$EndComp
Text Label 1750 5450 0    60   ~ 0
VRAW
Text Label 1750 5550 0    60   ~ 0
GND
Wire Wire Line
	2050 5450 1600 5450
Wire Wire Line
	2050 5550 1600 5550
Text Label 3050 2950 0    60   ~ 0
12V0_1_EN
Wire Wire Line
	3050 2950 3550 2950
Wire Wire Line
	1950 3250 1600 3250
Wire Wire Line
	1950 3150 1600 3150
Text Label 1650 3150 0    60   ~ 0
GND
Text Label 1650 3250 0    60   ~ 0
12V0_1
$Comp
L CONN_01X02 J10
U 1 1 58FA8646
P 1400 3200
F 0 "J10" H 1400 3350 50  0000 C CNN
F 1 "CONN_01X02" V 1500 3200 50  0000 C CNN
F 2 "conservify:TerminalBlock_Phoenix_MKDS1.5-2pol" H 1400 3200 50  0001 C CNN
F 3 "" H 1400 3200 50  0001 C CNN
	1    1400 3200
	-1   0    0    1   
$EndComp
Wire Wire Line
	2850 3250 2500 3250
Wire Wire Line
	2850 3150 2500 3150
Wire Wire Line
	3050 3250 3550 3250
Wire Wire Line
	3050 3150 3550 3150
Wire Wire Line
	3050 3050 3550 3050
Text Label 2550 3150 0    60   ~ 0
GND
Text Label 2550 3250 0    60   ~ 0
12V0_1
$Comp
L CONN_01X02 J4
U 1 1 58FA6511
P 2300 3200
F 0 "J4" H 2300 3350 50  0000 C CNN
F 1 "CONN_01X02" V 2400 3200 50  0000 C CNN
F 2 "conservify:TerminalBlock_Phoenix_MKDS1.5-2pol" H 2300 3200 50  0001 C CNN
F 3 "" H 2300 3200 50  0001 C CNN
	1    2300 3200
	-1   0    0    1   
$EndComp
Text Label 3050 3250 0    60   ~ 0
12V0_1
Text Label 3050 3150 0    60   ~ 0
GND
Text Label 3050 3050 0    60   ~ 0
VRAW
$Comp
L pololu-vreg U3
U 1 1 58FA64F5
P 4300 3100
F 0 "U3" H 4300 3200 60  0000 C CNN
F 1 "pololu-vreg" H 4300 3100 60  0000 C CNN
F 2 "conservify:pololu-vreg-adjustable" H 4300 3100 60  0001 C CNN
F 3 "" H 4300 3100 60  0001 C CNN
	1    4300 3100
	1    0    0    -1  
$EndComp
Text Label 3800 4650 0    60   ~ 0
12V0_1_EN
$EndSCHEMATC
