EESchema Schematic File Version 4
LIBS:power-bus-cache
EELAYER 26 0
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
L conservify:CONN_01x02 J1
U 1 1 58FA5FA2
P 850 900
F 0 "J1" H 850 1000 50  0000 C CNN
F 1 "CONN_VIN" V 950 850 50  0000 C CNN
F 2 "conservify:TERMINAL_BLOCK_PHOENIX_MKDS1.5-2pol" H 850 900 50  0001 C CNN
F 3 "" H 850 900 50  0001 C CNN
	1    850  900 
	-1   0    0    1   
$EndComp
$Comp
L conservify:POLOLU_VOLTAGE_REGULATOR U1
U 1 1 58FA5FFD
P 4300 2350
F 0 "U1" H 4300 2450 60  0000 C CNN
F 1 "VREG_GP" H 4300 2350 60  0000 C CNN
F 2 "conservify:POLOLU_VREG_ADJUSTABLE" H 4300 2350 60  0001 C CNN
F 3 "" H 4300 2350 60  0001 C CNN
	1    4300 2350
	1    0    0    -1  
$EndComp
Text Label 2250 900  0    50   ~ 0
VRAW
Text Label 1300 800  0    50   ~ 0
GND
Text Label 3050 2300 0    50   ~ 0
VRAW
Text Label 3050 2400 0    50   ~ 0
GND
Text Label 3050 2500 0    50   ~ 0
V_GP
Text Label 3050 2200 0    50   ~ 0
V_GP_EN
$Comp
L conservify:CONN_01x02 J2
U 1 1 58FA6300
P 2300 2500
F 0 "J2" H 2300 2600 50  0000 C CNN
F 1 "CONN_GP" V 2400 2450 50  0000 C CNN
F 2 "conservify:TERMINAL_BLOCK_PHOENIX_MKDS1.5-2pol" H 2300 2500 50  0001 C CNN
F 3 "" H 2300 2500 50  0001 C CNN
	1    2300 2500
	-1   0    0    1   
$EndComp
Text Label 2550 2400 0    50   ~ 0
GND
$Comp
L conservify:POLOLU_VOLTAGE_REGULATOR U4
U 1 1 58FA6503
P 7650 1050
F 0 "U4" H 7650 1150 60  0000 C CNN
F 1 "VREG_PI" H 7650 1050 60  0000 C CNN
F 2 "conservify:POLOLU_VREG_ADJUSTABLE" H 7650 1050 60  0001 C CNN
F 3 "" H 7650 1050 60  0001 C CNN
	1    7650 1050
	1    0    0    -1  
$EndComp
Text Label 6400 1000 0    50   ~ 0
VRAW
Text Label 6400 1100 0    50   ~ 0
GND
Text Label 6400 1200 0    50   ~ 0
V_PI
Text Label 6400 900  0    50   ~ 0
V_PI_EN
$Comp
L conservify:CONN_01x02 J5
U 1 1 58FA6517
P 5650 1200
F 0 "J5" H 5650 1350 50  0000 C CNN
F 1 "CONN_PI" V 5750 1150 50  0000 C CNN
F 2 "conservify:TERMINAL_BLOCK_PHOENIX_MKDS1.5-2pol" H 5650 1200 50  0001 C CNN
F 3 "" H 5650 1200 50  0001 C CNN
	1    5650 1200
	-1   0    0    1   
$EndComp
Text Label 5900 1100 0    50   ~ 0
GND
Text Label 5900 1200 0    50   ~ 0
V_PI
Wire Wire Line
	3050 2200 3550 2200
Wire Wire Line
	3050 2300 3550 2300
Wire Wire Line
	3050 2400 3550 2400
Wire Wire Line
	3050 2500 3550 2500
Wire Wire Line
	2850 2400 2500 2400
Wire Wire Line
	2850 2500 2500 2500
Wire Wire Line
	6400 900  6900 900 
Wire Wire Line
	6400 1000 6900 1000
Wire Wire Line
	6400 1100 6900 1100
Wire Wire Line
	6400 1200 6900 1200
Wire Wire Line
	6200 1200 5850 1200
Wire Wire Line
	6200 1100 5850 1100
$Comp
L conservify:GND #PWR01
U 1 1 58FA74B4
P 3050 850
F 0 "#PWR01" H 3050 600 50  0001 C CNN
F 1 "GND" H 3050 700 50  0000 C CNN
F 2 "" H 3050 850 50  0001 C CNN
F 3 "" H 3050 850 50  0001 C CNN
	1    3050 850 
	1    0    0    -1  
$EndComp
Text Label 3200 800  0    50   ~ 0
GND
Wire Wire Line
	3050 800  3050 850 
$Comp
L conservify:FUSE F1
U 1 1 58FE4282
P 1900 900
F 0 "F1" H 1980 900 50  0000 C CNN
F 1 "Fuse" H 1800 850 50  0000 C CNN
F 2 "conservify:FUSE_HOLDER_6x30" V 1830 900 50  0001 C CNN
F 3 "" H 1900 900 50  0001 C CNN
	1    1900 900 
	-1   0    0    1   
$EndComp
Text Label 1300 900  0    50   ~ 0
VIN
$Comp
L conservify:MOUNT_HOLE M1
U 1 1 59023B33
P 10300 6300
F 0 "M1" H 10300 6450 50  0000 C CNN
F 1 "MOUNT_HOLE" H 10300 6150 50  0001 C CNN
F 2 "conservify:HOLE_NPTH_3.8MM" H 10325 6075 50  0001 C CNN
F 3 "" H 10300 6300 50  0000 C CNN
	1    10300 6300
	1    0    0    -1  
$EndComp
$Comp
L conservify:MOUNT_HOLE M2
U 1 1 59023BD7
P 10500 6300
F 0 "M2" H 10500 6450 50  0000 C CNN
F 1 "MOUNT_HOLE" H 10500 6150 50  0001 C CNN
F 2 "conservify:HOLE_NPTH_3.8MM" H 10525 6075 50  0001 C CNN
F 3 "" H 10500 6300 50  0000 C CNN
	1    10500 6300
	1    0    0    -1  
$EndComp
$Comp
L conservify:MOUNT_HOLE M3
U 1 1 59023C27
P 10700 6300
F 0 "M3" H 10700 6450 50  0000 C CNN
F 1 "MOUNT_HOLE" H 10700 6150 50  0001 C CNN
F 2 "conservify:HOLE_NPTH_3.8MM" H 10725 6075 50  0001 C CNN
F 3 "" H 10700 6300 50  0000 C CNN
	1    10700 6300
	1    0    0    -1  
$EndComp
$Comp
L conservify:MOUNT_HOLE M4
U 1 1 59023C7E
P 10900 6300
F 0 "M4" H 10900 6450 50  0000 C CNN
F 1 "MOUNT_HOLE" H 10900 6150 50  0001 C CNN
F 2 "conservify:HOLE_NPTH_3.8MM" H 10925 6075 50  0001 C CNN
F 3 "" H 10900 6300 50  0000 C CNN
	1    10900 6300
	1    0    0    -1  
$EndComp
$Comp
L conservify:CONN_01x02 J13
U 1 1 5A0A35B3
P 3800 4800
F 0 "J13" H 3800 4900 50  0000 C CNN
F 1 "CONN_V_RELAY_2" H 3800 4600 50  0000 C CNN
F 2 "conservify:TERMINAL_BLOCK_PHOENIX_MKDS1.5-2pol" H 3800 4800 50  0001 C CNN
F 3 "" H 3800 4800 50  0001 C CNN
	1    3800 4800
	-1   0    0    1   
$EndComp
Text Label 4000 4700 0    50   ~ 0
V_RELAY_2
Text Label 4000 4800 0    50   ~ 0
V_RELAY_2_GND
Wire Wire Line
	3050 2950 3550 2950
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
Text Label 2550 3150 0    50   ~ 0
GND
Text Label 2550 3250 0    50   ~ 0
V_MON
$Comp
L conservify:CONN_01x02 J4
U 1 1 58FA6511
P 2300 3250
F 0 "J4" H 2300 3350 50  0000 C CNN
F 1 "CONN_MON" V 2400 3250 50  0000 C CNN
F 2 "conservify:TERMINAL_BLOCK_PHOENIX_MKDS1.5-2pol" H 2300 3250 50  0001 C CNN
F 3 "" H 2300 3250 50  0001 C CNN
	1    2300 3250
	-1   0    0    1   
$EndComp
Text Label 3050 3250 0    50   ~ 0
V_MON
Text Label 3050 3150 0    50   ~ 0
GND
Text Label 3050 3050 0    50   ~ 0
VRAW
$Comp
L conservify:POLOLU_VOLTAGE_REGULATOR U3
U 1 1 58FA64F5
P 4300 3100
F 0 "U3" H 4300 3200 60  0000 C CNN
F 1 "VREG_MON" H 4300 3100 60  0000 C CNN
F 2 "conservify:POLOLU_VREG_ADJUSTABLE" H 4300 3100 60  0001 C CNN
F 3 "" H 4300 3100 60  0001 C CNN
	1    4300 3100
	1    0    0    -1  
$EndComp
$Comp
L conservify:ADAFRUIT_FEATHER U2
U 1 1 5B9CA2FB
P 10100 2050
F 0 "U2" H 10075 2587 60  0000 C CNN
F 1 "ADAFRUIT_FEATHER" H 10075 2481 60  0000 C CNN
F 2 "conservify:ADAFRUIT_FEATHER" H 10100 2050 60  0001 C CNN
F 3 "" H 10100 2050 60  0001 C CNN
	1    10100 2050
	1    0    0    -1  
$EndComp
$Comp
L conservify:GND #PWR0101
U 1 1 5B9C083B
P 8100 3300
F 0 "#PWR0101" H 8100 3050 50  0001 C CNN
F 1 "GND" H 8100 3150 50  0000 C CNN
F 2 "" H 8100 3300 50  0001 C CNN
F 3 "" H 8100 3300 50  0001 C CNN
	1    8100 3300
	0    -1   -1   0   
$EndComp
$Comp
L conservify:GND #PWR0102
U 1 1 5B9C750A
P 6750 3600
F 0 "#PWR0102" H 6750 3350 50  0001 C CNN
F 1 "GND" H 6750 3450 50  0000 C CNN
F 2 "" H 6750 3600 50  0001 C CNN
F 3 "" H 6750 3600 50  0001 C CNN
	1    6750 3600
	0    1    1    0   
$EndComp
Wire Wire Line
	7300 3600 6750 3600
$Comp
L conservify:GND #PWR0103
U 1 1 5B9C883B
P 6750 2900
F 0 "#PWR0103" H 6750 2650 50  0001 C CNN
F 1 "GND" H 6750 2750 50  0000 C CNN
F 2 "" H 6750 2900 50  0001 C CNN
F 3 "" H 6750 2900 50  0001 C CNN
	1    6750 2900
	0    1    1    0   
$EndComp
$Comp
L conservify:GND #PWR0104
U 1 1 5B9C88CE
P 6750 2100
F 0 "#PWR0104" H 6750 1850 50  0001 C CNN
F 1 "GND" H 6750 1950 50  0000 C CNN
F 2 "" H 6750 2100 50  0001 C CNN
F 3 "" H 6750 2100 50  0001 C CNN
	1    6750 2100
	0    1    1    0   
$EndComp
Wire Wire Line
	6750 2100 7300 2100
Wire Wire Line
	6750 2900 7300 2900
$Comp
L conservify:GND #PWR0105
U 1 1 5B9CAF6C
P 8100 3100
F 0 "#PWR0105" H 8100 2850 50  0001 C CNN
F 1 "GND" H 8100 2950 50  0000 C CNN
F 2 "" H 8100 3100 50  0001 C CNN
F 3 "" H 8100 3100 50  0001 C CNN
	1    8100 3100
	0    -1   -1   0   
$EndComp
$Comp
L conservify:GND #PWR0106
U 1 1 5B9CAFA1
P 8100 2600
F 0 "#PWR0106" H 8100 2350 50  0001 C CNN
F 1 "GND" H 8100 2450 50  0000 C CNN
F 2 "" H 8100 2600 50  0001 C CNN
F 3 "" H 8100 2600 50  0001 C CNN
	1    8100 2600
	0    -1   -1   0   
$EndComp
$Comp
L conservify:GND #PWR0107
U 1 1 5B9CAFD6
P 8100 2300
F 0 "#PWR0107" H 8100 2050 50  0001 C CNN
F 1 "GND" H 8100 2150 50  0000 C CNN
F 2 "" H 8100 2300 50  0001 C CNN
F 3 "" H 8100 2300 50  0001 C CNN
	1    8100 2300
	0    -1   -1   0   
$EndComp
$Comp
L conservify:GND #PWR0108
U 1 1 5B9CB00B
P 8100 1900
F 0 "#PWR0108" H 8100 1650 50  0001 C CNN
F 1 "GND" H 8100 1750 50  0000 C CNN
F 2 "" H 8100 1900 50  0001 C CNN
F 3 "" H 8100 1900 50  0001 C CNN
	1    8100 1900
	0    -1   -1   0   
$EndComp
Wire Wire Line
	7800 3300 8100 3300
Wire Wire Line
	7800 3100 8100 3100
Wire Wire Line
	7800 2600 8100 2600
Wire Wire Line
	7800 2300 8100 2300
Wire Wire Line
	7800 1900 8100 1900
Wire Wire Line
	7800 1800 8100 1800
Wire Wire Line
	7800 1700 8100 1700
Text Label 7850 1700 0    50   ~ 0
PI_5V0
Wire Wire Line
	8100 1700 8100 1800
Text Label 6750 1700 0    50   ~ 0
PI_3V3
Wire Wire Line
	6750 1700 7300 1700
Text Label 7900 2000 0    50   ~ 0
PI_TX
Text Label 7900 2100 0    50   ~ 0
PI_RX
Wire Wire Line
	7800 2000 8100 2000
Wire Wire Line
	8100 2100 7800 2100
$Comp
L conservify:GND #PWR0109
U 1 1 5B9DB01D
P 9150 2100
F 0 "#PWR0109" H 9150 1850 50  0001 C CNN
F 1 "GND" H 9150 1950 50  0000 C CNN
F 2 "" H 9150 2100 50  0001 C CNN
F 3 "" H 9150 2100 50  0001 C CNN
	1    9150 2100
	0    1    1    0   
$EndComp
$Comp
L conservify:GND #PWR0110
U 1 1 5B9DB07C
P 9150 3300
F 0 "#PWR0110" H 9150 3050 50  0001 C CNN
F 1 "GND" H 9150 3150 50  0000 C CNN
F 2 "" H 9150 3300 50  0001 C CNN
F 3 "" H 9150 3300 50  0001 C CNN
	1    9150 3300
	0    1    1    0   
$EndComp
Wire Wire Line
	9150 2100 9400 2100
Wire Wire Line
	9150 3300 9400 3300
$Comp
L conservify:CONN_01x08 J6
U 1 1 5B9E06A1
P 7500 4200
F 0 "J6" H 7450 3700 50  0000 L CNN
F 1 "DS3231" V 7600 4000 50  0000 L CNN
F 2 "conservify:Socket_Strip_Straight_1x08_Pitch2.54mm" H 7500 4200 50  0001 C CNN
F 3 "" H 7500 4200 50  0001 C CNN
	1    7500 4200
	1    0    0    -1  
$EndComp
NoConn ~ 7300 3900
NoConn ~ 7300 4000
NoConn ~ 7300 4100
NoConn ~ 7300 4200
$Comp
L conservify:GND #PWR0111
U 1 1 5B9E69C0
P 7000 4500
F 0 "#PWR0111" H 7000 4250 50  0001 C CNN
F 1 "GND" H 7000 4350 50  0000 C CNN
F 2 "" H 7000 4500 50  0001 C CNN
F 3 "" H 7000 4500 50  0001 C CNN
	1    7000 4500
	0    1    1    0   
$EndComp
Wire Wire Line
	7000 4500 7300 4500
Wire Wire Line
	7000 4600 7300 4600
Text Label 7000 4300 0    50   ~ 0
PI_SDA
Text Label 7000 4400 0    50   ~ 0
PI_SCL
Wire Wire Line
	7000 4300 7300 4300
Wire Wire Line
	7000 4400 7300 4400
Text Label 7000 4600 0    50   ~ 0
PI_3V3
Text Label 6750 1800 0    50   ~ 0
PI_SDA
Text Label 6750 1900 0    50   ~ 0
PI_SCL
Wire Wire Line
	6750 1800 7300 1800
Wire Wire Line
	7300 1900 6750 1900
Text Label 9150 3100 0    50   ~ 0
PI_TX
Text Label 9150 3200 0    50   ~ 0
PI_RX
Wire Wire Line
	9150 3100 9400 3100
Wire Wire Line
	9150 3200 9400 3200
$Comp
L conservify:POLOLU_VOLTAGE_REGULATOR U5
U 1 1 5BA148D5
P 10150 1050
F 0 "U5" H 10150 1150 60  0000 C CNN
F 1 "VREG_MCU" H 10150 1050 60  0000 C CNN
F 2 "conservify:POLOLU_VREG" H 10150 1050 60  0001 C CNN
F 3 "" H 10150 1050 60  0001 C CNN
	1    10150 1050
	1    0    0    -1  
$EndComp
Text Label 8900 1000 0    50   ~ 0
VRAW
Text Label 8900 1100 0    50   ~ 0
GND
Text Label 8900 1200 0    50   ~ 0
V_MCU
Wire Wire Line
	8900 1000 9400 1000
Wire Wire Line
	8900 1100 9400 1100
NoConn ~ 9400 900 
Text Label 10800 2400 0    60   ~ 0
V_MCU
Wire Wire Line
	10750 2400 11100 2400
Text Label 2550 2500 0    50   ~ 0
V_GP
Text Label 8150 1700 0    50   ~ 0
V_PI
Connection ~ 8100 1700
Text Label 10800 3100 0    50   ~ 0
V_PI_EN
Wire Wire Line
	11100 3100 10750 3100
$Comp
L conservify:CONN_01x04 J7
U 1 1 5BA3CCA1
P 9750 4100
F 0 "J7" H 9750 4300 50  0000 C CNN
F 1 "CONN_RB" V 9850 4050 50  0000 C CNN
F 2 "conservify:Socket_Strip_Straight_1x04_Pitch2.54mm" H 9750 4100 50  0001 C CNN
F 3 "" H 9750 4100 50  0001 C CNN
	1    9750 4100
	-1   0    0    1   
$EndComp
Text Label 10000 4100 0    50   ~ 0
RB_TX
Text Label 10000 4200 0    50   ~ 0
RB_RX
$Comp
L conservify:GND #PWR0112
U 1 1 5BA3D392
P 10250 3900
F 0 "#PWR0112" H 10250 3650 50  0001 C CNN
F 1 "GND" H 10250 3750 50  0000 C CNN
F 2 "" H 10250 3900 50  0001 C CNN
F 3 "" H 10250 3900 50  0001 C CNN
	1    10250 3900
	0    -1   -1   0   
$EndComp
Wire Wire Line
	10250 3900 9950 3900
Wire Wire Line
	9950 4100 10250 4100
Wire Wire Line
	10250 4200 9950 4200
Wire Wire Line
	9950 4000 10250 4000
Text Label 10000 4000 0    50   ~ 0
V_RB
Wire Wire Line
	8100 1700 8350 1700
Text Label 8650 1200 0    50   ~ 0
V_RB
Wire Wire Line
	8650 1200 9400 1200
Text Label 10800 2700 0    50   ~ 0
RB_RX
Text Label 10800 2800 0    50   ~ 0
RB_TX
Wire Wire Line
	11050 2700 10750 2700
Wire Wire Line
	10750 2800 11050 2800
Wire Wire Line
	1050 900  1600 900 
Wire Wire Line
	1050 800  1600 800 
NoConn ~ 9400 1800
NoConn ~ 9400 1900
NoConn ~ 9400 2000
NoConn ~ 9400 2200
NoConn ~ 9400 2300
NoConn ~ 9400 2400
NoConn ~ 9400 2500
NoConn ~ 9400 2600
NoConn ~ 9400 2700
NoConn ~ 9400 2800
NoConn ~ 9400 2900
NoConn ~ 9400 3000
NoConn ~ 10750 3300
NoConn ~ 10750 3200
NoConn ~ 10750 2900
NoConn ~ 10750 3000
NoConn ~ 10750 2600
NoConn ~ 10750 2500
NoConn ~ 10750 2300
NoConn ~ 10750 2200
$Comp
L conservify:PWR_FLAG #FLG0101
U 1 1 5B9E5511
P 2650 850
F 0 "#FLG0101" H 2650 925 50  0001 C CNN
F 1 "PWR_FLAG" H 2650 1024 50  0000 C CNN
F 2 "" H 2650 850 50  0001 C CNN
F 3 "~" H 2650 850 50  0001 C CNN
	1    2650 850 
	1    0    0    -1  
$EndComp
Wire Wire Line
	2650 850  2650 900 
Wire Wire Line
	2200 900  2650 900 
$Comp
L conservify:PWR_FLAG #FLG0102
U 1 1 5B9E7270
P 3550 800
F 0 "#FLG0102" H 3550 875 50  0001 C CNN
F 1 "PWR_FLAG" H 3550 974 50  0000 C CNN
F 2 "" H 3550 800 50  0001 C CNN
F 3 "~" H 3550 800 50  0001 C CNN
	1    3550 800 
	1    0    0    -1  
$EndComp
Wire Wire Line
	3050 800  3550 800 
NoConn ~ 7300 2000
NoConn ~ 7300 2500
NoConn ~ 7300 2600
NoConn ~ 7300 2700
NoConn ~ 7300 2800
NoConn ~ 7300 3500
NoConn ~ 7300 3400
NoConn ~ 7300 3300
NoConn ~ 7300 3200
NoConn ~ 7300 3100
NoConn ~ 7300 3000
NoConn ~ 7800 2200
NoConn ~ 7800 2400
NoConn ~ 7800 2500
NoConn ~ 7800 2700
NoConn ~ 7800 2800
NoConn ~ 7800 2900
NoConn ~ 7800 3000
NoConn ~ 7800 3200
NoConn ~ 7800 3600
NoConn ~ 7800 3500
NoConn ~ 7800 3400
Text Label 6750 2200 0    50   ~ 0
V_RELAY_1_EN
Wire Wire Line
	6750 2200 7300 2200
$Comp
L conservify:CONN_02x20_Odd_Even J3
U 1 1 5B9BF33D
P 7500 2600
F 0 "J3" H 7550 1500 50  0000 C CNN
F 1 "RPI" H 7550 3626 50  0000 C CNN
F 2 "conservify:IDC-Header_2x20_P2.54mm_Vertical" H 7500 2600 50  0001 C CNN
F 3 "" H 7500 2600 50  0001 C CNN
	1    7500 2600
	1    0    0    -1  
$EndComp
Text Label 6750 2400 0    50   ~ 0
V_GP_EN
Wire Wire Line
	6750 2300 7300 2300
$Comp
L conservify:G5LE-1 K1
U 1 1 5BA3CB0D
P 2350 5900
F 0 "K1" V 1783 5900 50  0000 C CNN
F 1 "G5LE-1" V 1874 5900 50  0000 C CNN
F 2 "conservify:Relay_SPDT_Omron-G5LE-1" H 2800 5850 50  0001 L CNN
F 3 "http://www.omron.com/ecb/products/pdf/en-g5le.pdf" H 2350 5500 50  0001 C CNN
	1    2350 5900
	1    0    0    -1  
$EndComp
$Comp
L conservify:1N4148 D1
U 1 1 5BA3D5C8
P 1550 5900
F 0 "D1" H 1550 5684 50  0000 C CNN
F 1 "1N4148" H 1550 5775 50  0000 C CNN
F 2 "conservify:SOD-123" H 1550 5725 50  0001 C CNN
F 3 "" H 1550 5900 50  0001 C CNN
	1    1550 5900
	0    1    1    0   
$EndComp
$Comp
L conservify:R R1
U 1 1 5BA40F11
P 1450 6500
F 0 "R1" V 1243 6500 50  0000 C CNN
F 1 "1K" V 1334 6500 50  0000 C CNN
F 2 "conservify:RES-0603" V 1380 6500 50  0001 C CNN
F 3 "" H 1450 6500 50  0001 C CNN
	1    1450 6500
	0    1    1    0   
$EndComp
$Comp
L conservify:R R2
U 1 1 5BA40FDB
P 1750 6950
F 0 "R2" H 1680 6904 50  0000 R CNN
F 1 "1K" H 1680 6995 50  0000 R CNN
F 2 "conservify:RES-0603" V 1680 6950 50  0001 C CNN
F 3 "" H 1750 6950 50  0001 C CNN
	1    1750 6950
	-1   0    0    1   
$EndComp
$Comp
L conservify:MMBT2222 Q1
U 1 1 5BA413F6
P 2150 6500
F 0 "Q1" V 2200 6600 50  0000 L CNN
F 1 "MMBT2222" V 2200 6000 50  0000 L CNN
F 2 "conservify:SOT-23" H 2150 6500 50  0001 C CNN
F 3 "" H 2150 6500 50  0001 C CNN
	1    2150 6500
	1    0    0    -1  
$EndComp
Wire Wire Line
	1550 5750 1550 5450
Wire Wire Line
	1550 5450 2150 5450
Wire Wire Line
	2150 5450 2150 5600
Wire Wire Line
	1550 6050 1550 6250
Wire Wire Line
	1550 6250 2150 6250
Wire Wire Line
	2150 6250 2150 6200
Wire Wire Line
	2150 6300 2150 6250
Connection ~ 2150 6250
Wire Wire Line
	1600 6500 1750 6500
Wire Wire Line
	1750 6500 1750 6800
Connection ~ 1750 6500
Wire Wire Line
	1750 6500 1850 6500
Wire Wire Line
	1750 7100 1750 7350
Wire Wire Line
	1750 7350 2150 7350
Wire Wire Line
	2150 7350 2150 6700
$Comp
L conservify:GND #PWR0113
U 1 1 5BA52FFA
P 2150 7550
F 0 "#PWR0113" H 2150 7300 50  0001 C CNN
F 1 "GND" H 2150 7400 50  0000 C CNN
F 2 "" H 2150 7550 50  0001 C CNN
F 3 "" H 2150 7550 50  0001 C CNN
	1    2150 7550
	1    0    0    -1  
$EndComp
Wire Wire Line
	2150 7350 2150 7550
Connection ~ 2150 7350
Connection ~ 2150 5450
Text Label 2150 5350 1    50   ~ 0
PI_3V3
Wire Wire Line
	2150 5100 2150 5450
Wire Wire Line
	1300 6500 600  6500
Text Label 600  6500 0    50   ~ 0
V_RELAY_1_EN
Wire Wire Line
	2650 5600 2650 5100
Wire Wire Line
	2550 6200 2550 7350
Text Label 2550 7350 1    50   ~ 0
V_RELAY_1_IN
Text Label 2450 5500 1    50   ~ 0
V_RELAY_1
$Comp
L conservify:G5LE-1 K2
U 1 1 5BA7496B
P 4550 5900
F 0 "K2" V 3983 5900 50  0000 C CNN
F 1 "G5LE-1" V 4074 5900 50  0000 C CNN
F 2 "conservify:Relay_SPDT_Omron-G5LE-1" H 5000 5850 50  0001 L CNN
F 3 "http://www.omron.com/ecb/products/pdf/en-g5le.pdf" H 4550 5500 50  0001 C CNN
	1    4550 5900
	1    0    0    -1  
$EndComp
$Comp
L conservify:1N4148 D2
U 1 1 5BA74971
P 3750 5900
F 0 "D2" H 3750 5684 50  0000 C CNN
F 1 "1N4148" H 3750 5775 50  0000 C CNN
F 2 "conservify:SOD-123" H 3750 5725 50  0001 C CNN
F 3 "" H 3750 5900 50  0001 C CNN
	1    3750 5900
	0    1    1    0   
$EndComp
$Comp
L conservify:R R3
U 1 1 5BA74977
P 3650 6500
F 0 "R3" V 3443 6500 50  0000 C CNN
F 1 "1K" V 3534 6500 50  0000 C CNN
F 2 "conservify:RES-0603" V 3580 6500 50  0001 C CNN
F 3 "" H 3650 6500 50  0001 C CNN
	1    3650 6500
	0    1    1    0   
$EndComp
$Comp
L conservify:R R4
U 1 1 5BA7497D
P 3950 6950
F 0 "R4" H 3880 6904 50  0000 R CNN
F 1 "1K" H 3880 6995 50  0000 R CNN
F 2 "conservify:RES-0603" V 3880 6950 50  0001 C CNN
F 3 "" H 3950 6950 50  0001 C CNN
	1    3950 6950
	-1   0    0    1   
$EndComp
$Comp
L conservify:MMBT2222 Q2
U 1 1 5BA74983
P 4350 6500
F 0 "Q2" V 4400 6600 50  0000 L CNN
F 1 "MMBT2222" V 4400 6000 50  0000 L CNN
F 2 "conservify:SOT-23" H 4350 6500 50  0001 C CNN
F 3 "" H 4350 6500 50  0001 C CNN
	1    4350 6500
	1    0    0    -1  
$EndComp
Wire Wire Line
	3750 5750 3750 5450
Wire Wire Line
	3750 5450 4350 5450
Wire Wire Line
	4350 5450 4350 5600
Wire Wire Line
	3750 6050 3750 6250
Wire Wire Line
	3750 6250 4350 6250
Wire Wire Line
	4350 6250 4350 6200
Wire Wire Line
	4350 6300 4350 6250
Connection ~ 4350 6250
Wire Wire Line
	3800 6500 3950 6500
Wire Wire Line
	3950 6500 3950 6800
Connection ~ 3950 6500
Wire Wire Line
	3950 6500 4050 6500
Wire Wire Line
	3950 7100 3950 7350
Wire Wire Line
	3950 7350 4350 7350
Wire Wire Line
	4350 7350 4350 6700
$Comp
L conservify:GND #PWR0114
U 1 1 5BA74998
P 4350 7550
F 0 "#PWR0114" H 4350 7300 50  0001 C CNN
F 1 "GND" H 4350 7400 50  0000 C CNN
F 2 "" H 4350 7550 50  0001 C CNN
F 3 "" H 4350 7550 50  0001 C CNN
	1    4350 7550
	1    0    0    -1  
$EndComp
Wire Wire Line
	4350 7350 4350 7550
Connection ~ 4350 7350
Connection ~ 4350 5450
Text Label 4350 5350 1    50   ~ 0
PI_3V3
Wire Wire Line
	4350 5100 4350 5450
Wire Wire Line
	3500 6500 2800 6500
Text Label 2800 6500 0    50   ~ 0
V_RELAY_2_EN
Wire Wire Line
	4850 5600 4850 5100
Wire Wire Line
	4750 6200 4750 7350
Text Label 4750 7350 1    50   ~ 0
V_RELAY_2_IN
Text Label 4650 5500 1    50   ~ 0
V_RELAY_2
Wire Wire Line
	6750 2400 7300 2400
Text Label 6750 2300 0    50   ~ 0
V_RELAY_2_EN
$Comp
L conservify:CONN_01x02 J8
U 1 1 5BA89AC5
P 850 1500
F 0 "J8" H 850 1600 50  0000 C CNN
F 1 "CONN_GND" V 950 1400 50  0000 C CNN
F 2 "conservify:TERMINAL_BLOCK_PHOENIX_MKDS1.5-2pol" H 850 1500 50  0001 C CNN
F 3 "" H 850 1500 50  0001 C CNN
	1    850  1500
	-1   0    0    1   
$EndComp
Text Label 1100 1500 0    50   ~ 0
GND
Text Label 1100 2100 0    50   ~ 0
GND
Wire Wire Line
	1050 1400 1500 1400
Wire Wire Line
	1500 1500 1050 1500
$Comp
L conservify:CONN_01x02 J9
U 1 1 5BA95687
P 1600 4800
F 0 "J9" H 1600 4900 50  0000 C CNN
F 1 "CONN_V_RELAY_1" H 1600 4600 50  0000 C CNN
F 2 "conservify:TERMINAL_BLOCK_PHOENIX_MKDS1.5-2pol" H 1600 4800 50  0001 C CNN
F 3 "" H 1600 4800 50  0001 C CNN
	1    1600 4800
	-1   0    0    1   
$EndComp
Text Label 1800 4700 0    50   ~ 0
V_RELAY_1
Text Label 4000 4000 0    50   ~ 0
V_RELAY_2_IN
Text Label 1800 4000 0    50   ~ 0
V_RELAY_1_IN
$Comp
L conservify:CONN_01x02 J10
U 1 1 5BAC0233
P 850 2100
F 0 "J10" H 850 2200 50  0000 C CNN
F 1 "CONN_V_RAW" V 950 2050 50  0000 C CNN
F 2 "conservify:TERMINAL_BLOCK_PHOENIX_MKDS1.5-2pol" H 850 2100 50  0001 C CNN
F 3 "" H 850 2100 50  0001 C CNN
	1    850  2100
	-1   0    0    1   
$EndComp
Text Label 1100 2000 0    50   ~ 0
VRAW
Text Label 1100 1400 0    50   ~ 0
VRAW
Wire Wire Line
	1050 2000 1500 2000
Wire Wire Line
	1500 2100 1050 2100
Wire Wire Line
	4650 5100 4650 5600
NoConn ~ 4850 5100
Wire Wire Line
	2450 5100 2450 5600
NoConn ~ 2650 5100
NoConn ~ 3050 2950
$Comp
L conservify:CONN_01x02 J11
U 1 1 5BB0D3A5
P 1600 4100
F 0 "J11" H 1600 4200 50  0000 C CNN
F 1 "CONN_V_RELAY_1" H 1600 3900 50  0000 C CNN
F 2 "conservify:TERMINAL_BLOCK_PHOENIX_MKDS1.5-2pol" H 1600 4100 50  0001 C CNN
F 3 "" H 1600 4100 50  0001 C CNN
	1    1600 4100
	-1   0    0    1   
$EndComp
$Comp
L conservify:CONN_01x02 J12
U 1 1 5BB0D411
P 3800 4100
F 0 "J12" H 3800 4200 50  0000 C CNN
F 1 "CONN_V_RELAY_2" H 3800 3900 50  0000 C CNN
F 2 "conservify:TERMINAL_BLOCK_PHOENIX_MKDS1.5-2pol" H 3800 4100 50  0001 C CNN
F 3 "" H 3800 4100 50  0001 C CNN
	1    3800 4100
	-1   0    0    1   
$EndComp
Text Label 1800 4800 0    50   ~ 0
V_RELAY_1_GND
Text Label 1800 4100 0    50   ~ 0
V_RELAY_1_GND
Text Label 4000 4100 0    50   ~ 0
V_RELAY_2_GND
Wire Wire Line
	1800 4000 2400 4000
Wire Wire Line
	1800 4100 2400 4100
Wire Wire Line
	1800 4700 2400 4700
Wire Wire Line
	1800 4800 2400 4800
Wire Wire Line
	4000 4000 4650 4000
Wire Wire Line
	4000 4100 4650 4100
Wire Wire Line
	4000 4700 4600 4700
Wire Wire Line
	4000 4800 4600 4800
$Comp
L conservify:CONN_01x01 J14
U 1 1 5BB5EC78
P 850 2600
F 0 "J14" H 850 2700 50  0000 C CNN
F 1 "CONN_01x01" V 950 2600 50  0000 C CNN
F 2 "conservify:Socket_Strip_Straight_1x01_Pitch2.54mm" H 850 2600 50  0001 C CNN
F 3 "" H 850 2600 50  0001 C CNN
	1    850  2600
	-1   0    0    1   
$EndComp
Text Label 1100 2600 0    50   ~ 0
GND
Wire Wire Line
	1050 2600 1500 2600
$EndSCHEMATC
