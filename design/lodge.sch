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
LIBS:design-cache
EELAYER 25 0
EELAYER END
$Descr A4 11693 8268
encoding utf-8
Sheet 3 3
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
L charge-controller U23
U 1 1 593F5FDD
P 6750 2600
F 0 "U23" H 6900 2700 60  0001 C CNN
F 1 "charge-controller" H 6550 2800 60  0000 C CNN
F 2 "" H 6750 2600 60  0001 C CNN
F 3 "" H 6750 2600 60  0001 C CNN
	1    6750 2600
	1    0    0    -1  
$EndComp
$Comp
L power-bus U15
U 1 1 593F5FEB
P 3750 2450
F 0 "U15" H 3700 2350 60  0001 C CNN
F 1 "power-bus" H 3750 2450 60  0000 C CNN
F 2 "" H 3750 2450 60  0001 C CNN
F 3 "" H 3750 2450 60  0001 C CNN
	1    3750 2450
	-1   0    0    1   
$EndComp
$Comp
L raspberry-pi U19
U 1 1 593F5FF2
P 4000 5100
F 0 "U19" H 4000 5000 60  0001 C CNN
F 1 "raspberry-pi" H 4000 5100 60  0000 C CNN
F 2 "" H 4000 5100 60  0001 C CNN
F 3 "" H 4000 5100 60  0001 C CNN
	1    4000 5100
	1    0    0    -1  
$EndComp
$Comp
L usb-eth-adapter U16
U 1 1 593F5FF9
P 3900 4500
F 0 "U16" H 3900 4400 60  0001 C CNN
F 1 "usb-eth-adapter" H 3900 4500 60  0000 C CNN
F 2 "" H 3900 4500 60  0001 C CNN
F 3 "" H 3900 4500 60  0001 C CNN
	1    3900 4500
	-1   0    0    1   
$EndComp
$Comp
L wireless-bridge U20
U 1 1 593F6000
P 4100 5900
F 0 "U20" H 4100 5800 60  0001 C CNN
F 1 "wireless-bridge" H 4100 5900 60  0000 C CNN
F 2 "" H 4100 5900 60  0001 C CNN
F 3 "" H 4100 5900 60  0001 C CNN
	1    4100 5900
	1    0    0    -1  
$EndComp
$Comp
L hard-drive U18
U 1 1 593F6007
P 3950 3950
F 0 "U18" H 3950 3850 60  0001 C CNN
F 1 "hard-drive" H 3950 3950 60  0000 C CNN
F 2 "" H 3950 3950 60  0001 C CNN
F 3 "" H 3950 3950 60  0001 C CNN
	1    3950 3950
	1    0    0    -1  
$EndComp
$Comp
L hard-drive U17
U 1 1 593F600E
P 3950 3500
F 0 "U17" H 3950 3400 60  0001 C CNN
F 1 "hard-drive" H 3950 3500 60  0000 C CNN
F 2 "" H 3950 3500 60  0001 C CNN
F 3 "" H 3950 3500 60  0001 C CNN
	1    3950 3500
	1    0    0    -1  
$EndComp
Text Label 4700 2200 0    60   ~ 0
5v
Text Label 4650 2300 0    60   ~ 0
15v
Text Label 4550 2400 0    60   ~ 0
12v#1
Text Label 4550 2500 0    60   ~ 0
12v#2
$Comp
L batteries U22
U 1 1 593F601A
P 6250 1950
F 0 "U22" H 6250 1850 60  0001 C CNN
F 1 "batteries" H 6250 1950 60  0000 C CNN
F 2 "" H 6250 1950 60  0001 C CNN
F 3 "" H 6250 1950 60  0001 C CNN
	1    6250 1950
	1    0    0    -1  
$EndComp
Text Label 2950 4900 0    60   ~ 0
5v
Wire Wire Line
	2950 5800 3250 5800
Wire Wire Line
	4700 4350 7350 4350
Wire Wire Line
	4500 2200 4800 2200
Wire Wire Line
	4800 2300 4500 2300
Wire Wire Line
	4800 2400 4500 2400
Wire Wire Line
	4800 2500 4500 2500
Wire Wire Line
	4500 2600 5500 2600
Wire Wire Line
	5350 2400 5500 2400
Wire Wire Line
	5350 1950 5350 2400
Wire Wire Line
	5350 1950 5500 1950
Wire Wire Line
	2950 4900 3250 4900
Wire Wire Line
	2950 5000 3250 5000
Wire Wire Line
	2950 5100 3250 5100
Wire Wire Line
	2950 5200 3250 5200
Wire Wire Line
	2950 5300 3250 5300
Text Label 2950 5000 0    60   ~ 0
usb1
Text Label 2950 5100 0    60   ~ 0
usb2
Text Label 2950 5200 0    60   ~ 0
usb3
Text Label 2950 5300 0    60   ~ 0
usb4
Wire Wire Line
	2950 3850 3250 3850
Wire Wire Line
	2950 4050 3250 4050
Wire Wire Line
	2950 3600 3250 3600
Wire Wire Line
	2950 3400 3250 3400
Text Label 3050 4050 2    60   ~ 0
5v
Text Label 2950 3600 0    60   ~ 0
5v
Text Label 2950 3400 0    60   ~ 0
usb2
Text Label 2950 3850 0    60   ~ 0
usb1
Text Label 2950 4650 0    60   ~ 0
usb3
Wire Wire Line
	2950 4650 3250 4650
Wire Wire Line
	3250 5400 2950 5400
Text Label 2950 5400 0    60   ~ 0
eth0
Text Label 2950 5800 0    60   ~ 0
eth0
Wire Wire Line
	3250 5700 2950 5700
Text Label 2950 5700 0    60   ~ 0
12v#2
Text Label 4700 4450 0    60   ~ 0
usb5
Text Label 4700 4550 0    60   ~ 0
usb6
Wire Wire Line
	4700 4450 4950 4450
Wire Wire Line
	4950 4550 4700 4550
Text Label 5250 2700 0    60   ~ 0
usb6
Wire Wire Line
	5250 2700 5500 2700
$Comp
L solar-panels U21
U 1 1 593F606C
P 6250 1500
F 0 "U21" H 6250 1450 60  0001 C CNN
F 1 "solar-panels" H 6250 1550 60  0000 C CNN
F 2 "" H 6250 1550 60  0001 C CNN
F 3 "" H 6250 1550 60  0001 C CNN
	1    6250 1500
	1    0    0    -1  
$EndComp
Wire Wire Line
	5500 2500 5200 2500
Wire Wire Line
	5200 2500 5200 1500
Wire Wire Line
	5200 1500 5500 1500
$Comp
L ac-mains-charger U26
U 1 1 593F63BD
P 8900 2300
F 0 "U26" H 8900 2200 60  0001 C CNN
F 1 "ac-mains-charger" H 8900 2300 60  0000 C CNN
F 2 "" H 8900 2300 60  0001 C CNN
F 3 "" H 8900 2300 60  0001 C CNN
	1    8900 2300
	1    0    0    -1  
$EndComp
$Comp
L inverter U24
U 1 1 593F6430
P 7950 3150
F 0 "U24" H 7950 3100 60  0001 C CNN
F 1 "inverter" H 7950 3200 60  0000 C CNN
F 2 "" H 7950 3200 60  0001 C CNN
F 3 "" H 7950 3200 60  0001 C CNN
	1    7950 3150
	1    0    0    -1  
$EndComp
$Comp
L router U25
U 1 1 593F66CA
P 8000 4300
F 0 "U25" H 8000 4250 60  0001 C CNN
F 1 "router" H 8000 4350 60  0000 C CNN
F 2 "" H 8000 4350 60  0001 C CNN
F 3 "" H 8000 4350 60  0001 C CNN
	1    8000 4300
	1    0    0    -1  
$EndComp
Wire Wire Line
	5350 2200 8050 2200
Wire Wire Line
	7350 2200 7350 3050
Connection ~ 5350 2200
Connection ~ 7350 2200
Wire Wire Line
	7150 3250 7150 4250
Wire Wire Line
	7150 3250 7350 3250
Wire Wire Line
	7150 4250 7350 4250
Wire Wire Line
	8050 2450 7450 2450
Text Label 7450 2450 0    60   ~ 0
lodge-power
$EndSCHEMATC
