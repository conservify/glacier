EESchema Schematic File Version 2
LIBS:power
LIBS:device
LIBS:switches
LIBS:relays
LIBS:motors
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
LIBS:pi-power-center-cache
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
L Conn_02x20_Odd_Even J1
U 1 1 59F7B61E
P 3250 3200
F 0 "J1" H 3300 4200 50  0000 C CNN
F 1 "Conn_02x20_Odd_Even" H 3300 2100 50  0000 C CNN
F 2 "Socket_Strips:Socket_Strip_Straight_2x20_Pitch2.54mm" H 3250 3200 50  0001 C CNN
F 3 "" H 3250 3200 50  0001 C CNN
	1    3250 3200
	1    0    0    -1  
$EndComp
$Comp
L FEATHER U1
U 1 1 59F7B664
P 5700 2600
F 0 "U1" H 5300 1150 60  0000 C CNN
F 1 "FEATHER" H 5700 2600 60  0000 C CNN
F 2 "conservify:feather" H 5700 2600 60  0001 C CNN
F 3 "" H 5700 2600 60  0001 C CNN
	1    5700 2600
	1    0    0    -1  
$EndComp
$Comp
L +3V3 #PWR02
U 1 1 59F7B746
P 3900 2300
F 0 "#PWR02" H 3900 2150 50  0001 C CNN
F 1 "+3V3" H 3900 2440 50  0000 C CNN
F 2 "" H 3900 2300 50  0001 C CNN
F 3 "" H 3900 2300 50  0001 C CNN
	1    3900 2300
	0    1    1    0   
$EndComp
$Comp
L GND #PWR03
U 1 1 59F7B785
P 4700 2650
F 0 "#PWR03" H 4700 2400 50  0001 C CNN
F 1 "GND" H 4700 2500 50  0000 C CNN
F 2 "" H 4700 2650 50  0001 C CNN
F 3 "" H 4700 2650 50  0001 C CNN
	1    4700 2650
	0    1    1    0   
$EndComp
$Comp
L +3V3 #PWR04
U 1 1 59F7B796
P 4700 2450
F 0 "#PWR04" H 4700 2300 50  0001 C CNN
F 1 "+3V3" H 4700 2590 50  0000 C CNN
F 2 "" H 4700 2450 50  0001 C CNN
F 3 "" H 4700 2450 50  0001 C CNN
	1    4700 2450
	0    -1   -1   0   
$EndComp
Wire Wire Line
	4700 2450 5000 2450
Wire Wire Line
	5000 2650 4700 2650
Wire Wire Line
	3900 2300 3550 2300
Wire Wire Line
	3550 2700 4100 2700
$Comp
L GND #PWR05
U 1 1 59F7BA8C
P 4100 2700
F 0 "#PWR05" H 4100 2450 50  0001 C CNN
F 1 "GND" H 4100 2550 50  0000 C CNN
F 2 "" H 4100 2700 50  0001 C CNN
F 3 "" H 4100 2700 50  0001 C CNN
	1    4100 2700
	0    -1   -1   0   
$EndComp
Wire Wire Line
	3950 2700 3950 4200
Wire Wire Line
	3950 4200 3550 4200
Connection ~ 3950 2700
Text Label 3600 2800 0    60   ~ 0
BCM17
Text Label 3600 2900 0    60   ~ 0
BCM27
Text Label 3600 3000 0    60   ~ 0
BCM22
Wire Wire Line
	3550 2800 3900 2800
Wire Wire Line
	3900 2900 3550 2900
Wire Wire Line
	3900 3000 3550 3000
Text Label 4600 2750 0    60   ~ 0
BCM17
Wire Wire Line
	4600 2750 5000 2750
Text Label 4150 3300 0    60   ~ 0
BCM27
Text Label 4150 3200 0    60   ~ 0
BCM22
$Comp
L GND #PWR06
U 1 1 59F7BEC6
P 4100 3000
F 0 "#PWR06" H 4100 2750 50  0001 C CNN
F 1 "GND" H 4100 2850 50  0000 C CNN
F 2 "" H 4100 3000 50  0001 C CNN
F 3 "" H 4100 3000 50  0001 C CNN
	1    4100 3000
	-1   0    0    1   
$EndComp
$Comp
L Conn_01x04 J2
U 1 1 59F8BE1B
P 4650 3100
F 0 "J2" H 4650 3300 50  0000 C CNN
F 1 "Conn_01x04" H 4650 2800 50  0000 C CNN
F 2 "Socket_Strips:Socket_Strip_Straight_1x04_Pitch2.54mm" H 4650 3100 50  0001 C CNN
F 3 "" H 4650 3100 50  0001 C CNN
	1    4650 3100
	1    0    0    -1  
$EndComp
Wire Wire Line
	4100 3000 4450 3000
Wire Wire Line
	4450 3000 4450 3100
Wire Wire Line
	4450 3200 4150 3200
Wire Wire Line
	4150 3300 4450 3300
$Comp
L GND #PWR?
U 1 1 59F8F222
P 2750 2500
F 0 "#PWR?" H 2750 2250 50  0001 C CNN
F 1 "GND" H 2750 2350 50  0000 C CNN
F 2 "" H 2750 2500 50  0001 C CNN
F 3 "" H 2750 2500 50  0001 C CNN
	1    2750 2500
	0    1    1    0   
$EndComp
Wire Wire Line
	3050 2500 2750 2500
$EndSCHEMATC
