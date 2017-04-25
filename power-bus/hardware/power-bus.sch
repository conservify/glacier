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
P 800 950
F 0 "J1" H 800 1100 50  0000 C CNN
F 1 "CONN_01X02" V 900 950 50  0000 C CNN
F 2 "Connectors_Terminal_Blocks:TerminalBlock_Pheonix_MKDS1.5-2pol" H 800 950 50  0001 C CNN
F 3 "" H 800 950 50  0001 C CNN
	1    800  950 
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
Text Label 1300 1000 0    60   ~ 0
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
F 2 "Connectors_Terminal_Blocks:TerminalBlock_Pheonix_MKDS1.5-2pol" H 2300 1650 50  0001 C CNN
F 3 "" H 2300 1650 50  0001 C CNN
	1    2300 1650
	-1   0    0    1   
$EndComp
Text Label 2550 1700 0    60   ~ 0
5V0_1
Text Label 2550 1600 0    60   ~ 0
GND
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
Text Label 3050 3050 0    60   ~ 0
VRAW
Text Label 3050 3150 0    60   ~ 0
GND
Text Label 3050 3250 0    60   ~ 0
12V0_1
Text Label 3050 2950 0    60   ~ 0
12V0_1_EN
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
L CONN_01X02 J4
U 1 1 58FA6511
P 2300 3200
F 0 "J4" H 2300 3350 50  0000 C CNN
F 1 "CONN_01X02" V 2400 3200 50  0000 C CNN
F 2 "Connectors_Terminal_Blocks:TerminalBlock_Pheonix_MKDS1.5-2pol" H 2300 3200 50  0001 C CNN
F 3 "" H 2300 3200 50  0001 C CNN
	1    2300 3200
	-1   0    0    1   
$EndComp
$Comp
L CONN_01X02 J5
U 1 1 58FA6517
P 2300 3900
F 0 "J5" H 2300 4050 50  0000 C CNN
F 1 "CONN_01X02" V 2400 3900 50  0000 C CNN
F 2 "Connectors_Terminal_Blocks:TerminalBlock_Pheonix_MKDS1.5-2pol" H 2300 3900 50  0001 C CNN
F 3 "" H 2300 3900 50  0001 C CNN
	1    2300 3900
	-1   0    0    1   
$EndComp
Text Label 2550 3850 0    60   ~ 0
GND
Text Label 2550 3950 0    60   ~ 0
12V0_2
Text Label 2550 3250 0    60   ~ 0
12V0_1
Text Label 2550 3150 0    60   ~ 0
GND
$Comp
L pololu-vreg U5
U 1 1 58FA671E
P 8000 1550
F 0 "U5" H 8000 1650 60  0000 C CNN
F 1 "pololu-vreg" H 8000 1550 60  0000 C CNN
F 2 "conservify:pololu-vreg-adjustable" H 8000 1550 60  0001 C CNN
F 3 "" H 8000 1550 60  0001 C CNN
	1    8000 1550
	1    0    0    -1  
$EndComp
Text Label 6750 1500 0    60   ~ 0
VRAW
Text Label 6750 1600 0    60   ~ 0
GND
Text Label 6750 1700 0    60   ~ 0
VADJ_1
Text Label 6750 1400 0    60   ~ 0
VADJ_1_EN
$Comp
L CONN_01X02 J6
U 1 1 58FA673A
P 6000 1650
F 0 "J6" H 6000 1800 50  0000 C CNN
F 1 "CONN_01X02" V 6100 1650 50  0000 C CNN
F 2 "Connectors_Terminal_Blocks:TerminalBlock_Pheonix_MKDS1.5-2pol" H 6000 1650 50  0001 C CNN
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
	1600 1000 1000 1000
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
	3050 2950 3550 2950
Wire Wire Line
	3050 3050 3550 3050
Wire Wire Line
	3050 3150 3550 3150
Wire Wire Line
	3050 3250 3550 3250
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
	2850 3150 2500 3150
Wire Wire Line
	2850 3250 2500 3250
Wire Wire Line
	6750 1400 7250 1400
Wire Wire Line
	6750 1500 7250 1500
Wire Wire Line
	6750 1600 7250 1600
Wire Wire Line
	6750 1700 7250 1700
Wire Wire Line
	6550 1600 6200 1600
Wire Wire Line
	6550 1700 6200 1700
$Comp
L GND #PWR01
U 1 1 58FA74B4
P 2300 950
F 0 "#PWR01" H 2300 700 50  0001 C CNN
F 1 "GND" H 2300 800 50  0000 C CNN
F 2 "" H 2300 950 50  0001 C CNN
F 3 "" H 2300 950 50  0001 C CNN
	1    2300 950 
	1    0    0    -1  
$EndComp
Text Label 2450 900  0    60   ~ 0
GND
Wire Wire Line
	2700 900  2300 900 
Wire Wire Line
	2300 900  2300 950 
$Comp
L CONN_01X02 J10
U 1 1 58FA8646
P 1400 3200
F 0 "J10" H 1400 3350 50  0000 C CNN
F 1 "CONN_01X02" V 1500 3200 50  0000 C CNN
F 2 "Connectors_Terminal_Blocks:TerminalBlock_Pheonix_MKDS1.5-2pol" H 1400 3200 50  0001 C CNN
F 3 "" H 1400 3200 50  0001 C CNN
	1    1400 3200
	-1   0    0    1   
$EndComp
Text Label 1650 3250 0    60   ~ 0
12V0_1
Text Label 1650 3150 0    60   ~ 0
GND
Wire Wire Line
	1950 3150 1600 3150
Wire Wire Line
	1950 3250 1600 3250
$Comp
L CONN_01X02 J11
U 1 1 58FA8689
P 1400 3900
F 0 "J11" H 1400 4050 50  0000 C CNN
F 1 "CONN_01X02" V 1500 3900 50  0000 C CNN
F 2 "Connectors_Terminal_Blocks:TerminalBlock_Pheonix_MKDS1.5-2pol" H 1400 3900 50  0001 C CNN
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
L CONN_01X02 J12
U 1 1 58FA881A
P 5100 1650
F 0 "J12" H 5100 1800 50  0000 C CNN
F 1 "CONN_01X02" V 5200 1650 50  0000 C CNN
F 2 "Connectors_Terminal_Blocks:TerminalBlock_Pheonix_MKDS1.5-2pol" H 5100 1650 50  0001 C CNN
F 3 "" H 5100 1650 50  0001 C CNN
	1    5100 1650
	-1   0    0    1   
$EndComp
Text Label 5350 1700 0    60   ~ 0
VADJ_1
Text Label 5350 1600 0    60   ~ 0
GND
Wire Wire Line
	5650 1600 5300 1600
Wire Wire Line
	5650 1700 5300 1700
$Comp
L pololu-vreg U2
U 1 1 58FE353A
P 4300 2400
F 0 "U2" H 4300 2500 60  0000 C CNN
F 1 "pololu-vreg" H 4300 2400 60  0000 C CNN
F 2 "conservify:pololu-vreg-adjustable" H 4300 2400 60  0001 C CNN
F 3 "" H 4300 2400 60  0001 C CNN
	1    4300 2400
	1    0    0    -1  
$EndComp
Text Label 3050 2350 0    60   ~ 0
VRAW
Text Label 3050 2450 0    60   ~ 0
GND
Text Label 3050 2550 0    60   ~ 0
12V0_3
Text Label 3050 2250 0    60   ~ 0
12V0_3_EN
$Comp
L CONN_01X02 J7
U 1 1 58FE3544
P 2300 2500
F 0 "J7" H 2300 2650 50  0000 C CNN
F 1 "CONN_01X02" V 2400 2500 50  0000 C CNN
F 2 "Connectors_Terminal_Blocks:TerminalBlock_Pheonix_MKDS1.5-2pol" H 2300 2500 50  0001 C CNN
F 3 "" H 2300 2500 50  0001 C CNN
	1    2300 2500
	-1   0    0    1   
$EndComp
Text Label 2550 2450 0    60   ~ 0
GND
Text Label 2550 2550 0    60   ~ 0
12V0_3
Wire Wire Line
	3050 2250 3550 2250
Wire Wire Line
	3050 2350 3550 2350
Wire Wire Line
	3050 2450 3550 2450
Wire Wire Line
	3050 2550 3550 2550
Wire Wire Line
	2850 2550 2500 2550
Wire Wire Line
	2850 2450 2500 2450
$Comp
L CONN_01X02 J3
U 1 1 58FE3552
P 1400 2500
F 0 "J3" H 1400 2650 50  0000 C CNN
F 1 "CONN_01X02" V 1500 2500 50  0000 C CNN
F 2 "Connectors_Terminal_Blocks:TerminalBlock_Pheonix_MKDS1.5-2pol" H 1400 2500 50  0001 C CNN
F 3 "" H 1400 2500 50  0001 C CNN
	1    1400 2500
	-1   0    0    1   
$EndComp
Text Label 1650 2450 0    60   ~ 0
GND
Text Label 1650 2550 0    60   ~ 0
12V0_3
Wire Wire Line
	1950 2550 1600 2550
Wire Wire Line
	1950 2450 1600 2450
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
Text Label 3800 4550 0    60   ~ 0
12V0_3_EN
Text Label 3800 4650 0    60   ~ 0
12V0_1_EN
Text Label 3800 4750 0    60   ~ 0
12V0_2_EN
Text Label 3800 4850 0    60   ~ 0
VADJ_1_EN
Text Label 3800 4950 0    60   ~ 0
GND
Wire Wire Line
	3800 4550 4350 4550
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
F 2 "Fuse_Holders_and_Fuses:Fuseholder5x20_horiz_open_universal_Type-III" V 1680 900 50  0001 C CNN
F 3 "" H 1750 900 50  0001 C CNN
	1    1750 900 
	0    1    1    0   
$EndComp
Wire Wire Line
	2200 900  1900 900 
Text Label 1300 900  0    60   ~ 0
VIN
$Comp
L CONN_01X02 J13
U 1 1 58FE47A9
P 2850 4850
F 0 "J13" H 2850 5000 50  0000 C CNN
F 1 "CONN_01X02" V 2950 4850 50  0000 C CNN
F 2 "Socket_Strips:Socket_Strip_Straight_1x02_Pitch2.54mm" H 2850 4850 50  0001 C CNN
F 3 "" H 2850 4850 50  0001 C CNN
	1    2850 4850
	1    0    0    -1  
$EndComp
Text Label 2400 4800 0    60   ~ 0
GND
Text Label 2400 4900 0    60   ~ 0
GND
Wire Wire Line
	2400 4800 2650 4800
Wire Wire Line
	2650 4900 2400 4900
$EndSCHEMATC
