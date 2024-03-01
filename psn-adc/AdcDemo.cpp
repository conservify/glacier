// AdcDemo.c

#ifdef WIN32
#include <conio.h>
#else
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

#include "AdcDemo.h"

/* Test Program Defaults */
#define DEF_PORT_NUMBER 1 /* Comm port number if Windows  */

#define DEF_PORT_SPEED 38400         /* Can be 57600, 38400, 19200 or 9600 */
#define DEF_TIME_REF TIME_REF_GARMIN /* Time reference type */
#define DEF_SAMPLE_RATE 60           /* Sample rate */
#define DEF_NUMBER_CHANNELS 3        /* Number of channels to record */
#define DEF_HIGH_TO_LOW_PPS FALSE    /* GPS PPS direction */
#define DEF_NO_LED_STATUS FALSE      /* Blink LED On */
#define DEF_12BIT_FLAGS 0            /* Bitmap of ADC channels that should be sending 12 bit data */
#define DEF_TIME_OFFSET 0            /* Should be set to ~30 when using WWV time reference */
#define DEF_CHECK_TIME 1             /* 1 = display time difference between PC and ADC */
#define DEF_SET_PC_TIME 1            /* 1 = set PC time using ADC time */

/* Using to configure the DLL/Library if using PC time  */
#define SET_PC_TIME_NORMAL 1   /* Do not set time if large error between A/D and PC time */
#define SET_PC_TIME_LARGE_OK 2 /* Alwas set the time even if there is a large error */

AdcBoardConfig2 config; /* Configuration information sent to the DLL and ADC board */

/* Incoming ADC sample data is demuxed into this array. */
LONG demuxData[DEF_NUMBER_CHANNELS][MAX_SPS_RATE];
float floatData[DEF_NUMBER_CHANNELS * MAX_SPS_RATE];

HANDLE hBoard = 0; /* Handle to the DLL/ADC board */

BOOL displayBoardType = TRUE; /* Used to display the board type */
BOOL gpsDataMode = 0;         /* Used to enable/disable GPS data */
DWORD commPort;               /* Windows Comm port number */
char commPortStr[32];         /* Linux Comm Port string */

BOOL receivedSamples = FALSE;
int displayInfo = 1; /* If 1 display data header info */
int writeAscii = 0;
int consecutiveErrors = 0;

int ParseCommandLine(int argc, char *argv[]) {
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--ascii") == 0) {
            writeAscii = 1;
        } else {
            /* Check for Comm port string */
            strcpy(commPortStr, argv[1]);
        }
    }

    return 0;
}

int OpenDevice() {
    DWORD version, sts;

    /* Open the board. Function will return 0 if an error has occurred */
    hBoard = PSNOpenBoard();
    if (!hBoard) {
        printf("PSNOpenBoard Error\n");
        return 0;
    }

    /* Get DLL Version number. Version info is returned a 16 bit unsigned word */
    if (!PSNGetBoardInfo(hBoard, ADC_GET_DLL_VERSION, &version)) {
        printf("PSNGetBoardInfo Error\n");
        PSNCloseBoard(hBoard);
        return 0;
    }
    printf("PSNADBoard DLL Version %d.%1d\n", version / 10, version % 10);

    /* Fill in the configuration structure with the default info above */
    MakeConfig(&config);

    printf("PSNConfigBoard()\n");

    /* Pass the configuration information to the DLL. If using the Callback feature, pass
       a function pointer to the DDL. This function will be called by the DLL when new data
       needs to be processed by the application. If using the Poll method set the third
       parameter to 0 or NULL */
    sts = PSNConfigBoard(hBoard, &config, ProcessNewData);
    if (!sts) {
        printf("PSNConfigBoard Error\n");
        PSNCloseBoard(hBoard);
        return 0;
    }

    printf("PSNSendBoardCommand(ADC_CMD_RESET_BOARD)\n");

    if (!PSNSendBoardCommand(hBoard, ADC_CMD_RESET_BOARD, 0)) {
        printf("PSNSendBoardCommand Error\n");
        PSNCloseBoard(hBoard);
        return 0;
    }

    printf("PSNStartStopCollect(TRUE)\n");

    /* Now start the data collection. This will open the Comm port
       and new data from the DLL or ADC board will be sent to the Callback function or
       be available to the PSNGetBoardData() function. */
    sts = PSNStartStopCollect(hBoard, TRUE);
    if (!sts) {
        printf("PSNStartStopCollect Error\n");
        PSNCloseBoard(hBoard);
        return 0;
    }

    printf("while(consecutiveErrors < 3)\n");

    /* Callback Mode: In this mode new data will be posted to the application by the DLL
       using the Callback function.  */
    while (consecutiveErrors < 3) {
        /* NOTE!!! The user must relinquish cpu time so the DLL and other processes on the computer
           can run properly. */
        _sleep(100); /* sleep for 100 ms */
    }

    printf("PSNStartStopCollect(FALSE)\n");

    /* Stop the data collection. This will close the Comm port */
    if (!PSNStartStopCollect(hBoard, FALSE)) {
        printf("PSNStartStopCollect Error\n");
    }

    /* Sleep for a while so the DLL has some time to shut things down */
    _sleep(250);

    printf("PSNCloseBoard()\n");

    /* Now close the board */
    if (!PSNCloseBoard(hBoard)) {
        printf("PSNCloseBoard Error\n");
    }

    printf("Done\n");

    return receivedSamples;
}

int CompareStrings(const void *a, const void *b) {
    const char *sa = (const char *)a;
    const char *sb = (const char *)b;
    return strcmp(sb, sa);
}

#define MAX_DEVICES 10

int FindDevice() {
    DIR *dp;

    struct dirent *entry;

    if ((dp = opendir("/dev")) == NULL) {
        fprintf(stderr, "Unable to open /dev\n");
        return 0;
    }

    char *devices[MAX_DEVICES] = { 0 };
    size_t number = 0;
    while ((entry = readdir(dp)) != NULL) {
        const char *prefix = "ttyUSB";
        if (strncmp(entry->d_name, prefix, strlen(prefix)) == 0) {
            if (number < MAX_DEVICES) {
                char name[32] = { 0 };

                name[0] = 0;
                strcat(name, "/dev/");
                strcat(name, entry->d_name);

                printf("Found %s...\n", name);

                devices[number++] = strdup(name);
            }
        }
    }

    closedir(dp);

    qsort(devices, number, sizeof(char *), CompareStrings);

    for (size_t i = 0; i < number; ++i) {
        strncpy(commPortStr, devices[i], sizeof(commPortStr));

        printf("Trying %s...\n", commPortStr);

        free(devices[i]);
        devices[i] = 0;

        if (OpenDevice()) {
            return 1;
        }

        consecutiveErrors = 0;
    }

    fprintf(stderr, "Unable to find device!\n");

    return 0;
}

/* Program start here..... */
int main(int argc, char *argv[]) {
    ParseCommandLine(argc, argv);

    FindDevice();

    /* And we are done..... */
    return 0;
}

/* In the Callback mode this function is called by the DLL when new data is
   available to the application. It is also called by the function above in
   the poll data mode.  */
void ProcessNewData(DWORD type, void *data, void *data1, DWORD dataLen) {
    switch (type) {
    case ADC_ERROR: /* Display various text messages sent by the DLL or ADC board */
        consecutiveErrors++;
    case ADC_MSG:
    case ADC_AD_MSG:
        DisplayMsg(type, (char *)data);
        break;

    case ADC_AD_DATA: /* New ADC sample data  */
        consecutiveErrors = 0;
        receivedSamples = TRUE;
        NewADData(type, (DataHeader *)data, data1, dataLen);
        break;

    case ADC_GPS_DATA: /* New raw GPS data */
        NewGpsData((BYTE *)data, dataLen);
        break;

    case ADC_STATUS: /* New Status information */
        DisplayStatus((StatusInfo *)data);
        break;

    case ADC_SAVE_TIME_INFO: /* Display Status Information */
        SaveTimeInfo((TimeInfo *)data);
        break;

    default:
        printf("Unknown message type = %d", type);
        break;
    }
}

#include <stdint.h>

typedef struct logfile_t {
    uint32_t samples;

    FILE *bin_fp;
    char bin_file_name[128];

    FILE *csv_fp;
    char csv_file_name[128];
} logfile_t;

logfile_t lf = { 0 };

#define INCOMING_FILE_NAME_CSV "incoming.csv"
#define INCOMING_FILE_NAME_BIN "incoming.bin"
#define FILE_NAME_EXT_CSV "csv"
#define FILE_NAME_EXT_BIN "bin"

BOOL WriteLogFile(logfile_t *lf, DataHeader *hdr, LONG *adcData) {
    if (lf->bin_fp == NULL || lf->samples == MAX_SPS_RATE * 60) {
        if (lf->bin_fp != NULL) {
            fclose(lf->bin_fp);
            lf->bin_fp = NULL;

            printf("Finished %s\n", lf->bin_file_name);
            if (true) {
                if (rename(INCOMING_FILE_NAME_BIN, lf->bin_file_name) != 0) {
                    fprintf(stderr, "Unable to rename %s to %s\n", INCOMING_FILE_NAME_BIN, lf->bin_file_name);
                }
            }
        }

        if (lf->csv_fp != NULL) {
            fclose(lf->csv_fp);
            lf->csv_fp = NULL;

            printf("Finished %s\n", lf->csv_file_name);
            if (true) {
                if (rename(INCOMING_FILE_NAME_CSV, lf->csv_file_name) != 0) {
                    fprintf(stderr, "Unable to rename %s to %s\n", INCOMING_FILE_NAME_CSV, lf->csv_file_name);
                }
            }
        }

        SYSTEMTIME *st = &hdr->packetTime;

        snprintf(lf->bin_file_name, sizeof(lf->bin_file_name), "geophones_%04d%02d%02d_%02d%02d%02d.%s", st->wYear,
                 st->wMonth, st->wDay, st->wHour, st->wMinute, st->wSecond, FILE_NAME_EXT_BIN);

        lf->bin_fp = fopen(INCOMING_FILE_NAME_BIN, "w");
        if (lf->bin_fp == NULL) {
            fprintf(stderr, "Failed! %s\n", INCOMING_FILE_NAME_BIN);
        }

        snprintf(lf->csv_file_name, sizeof(lf->csv_file_name), "geophones_%04d%02d%02d_%02d%02d%02d.%s", st->wYear,
                 st->wMonth, st->wDay, st->wHour, st->wMinute, st->wSecond, FILE_NAME_EXT_CSV);

        lf->csv_fp = fopen(INCOMING_FILE_NAME_CSV, "w");
        if (lf->csv_fp == NULL) {
            fprintf(stderr, "Failed! %s\n", INCOMING_FILE_NAME_CSV);
        }

        lf->samples = 0;
    }

    for (size_t i = 0; i < MAX_SPS_RATE; ++i) {
        for (size_t j = 0; j < DEF_NUMBER_CHANNELS; ++j) {
            floatData[(i * DEF_NUMBER_CHANNELS) + j] = demuxData[j][i];
            if (j > 0)
                fprintf(lf->csv_fp, ",");
            fprintf(lf->csv_fp, "%d", demuxData[j][i]);
        }
        fprintf(lf->csv_fp, "\n");
    }

    uint32_t written = fwrite(floatData, sizeof(floatData), 1, lf->bin_fp);
    if (written != 1) {
        fprintf(stderr, "Unable to write full buffer to disk.\n");
    }
    lf->samples += MAX_SPS_RATE;

    return lf->bin_fp != NULL;
}

/* This function is called once per second with new ADC sample data. The header has the time
  of day of the first sample in the data array as well as the time reference lock status and
  packet ID. The number of samples in the adcData array is the number of channels
  being recorded times the sample rate. */
void NewADData(DWORD type, DataHeader *hdr, void *adcData, DWORD dataLen) {
    SYSTEMTIME *st = &hdr->packetTime;
    DWORD samples, index, c;
    char *boardStr = "???", *lockStr;
    short *spData; // used to demux 16 bit data
    LONG *lpData;  // used to demux 32 bit data from the VolksMeter sensor

    static DWORD boardType;

    /* On the first time here display the ADC board type */
    if (displayBoardType) {
        displayBoardType = 0;
        if (!PSNGetBoardInfo(hBoard, ADC_GET_BOARD_TYPE, &boardType))
            printf("\nPSNGetBoardInfo Error\n\n");
        else {
            boardStr = "Unknown";
            if (boardType == BOARD_V1)
                boardStr = "V1 Rabbit CPU";
            else if (boardType == BOARD_V2)
                boardStr = "V2 PICC CPU";
            else if (boardType == BOARD_VM)
                boardStr = "VolksMeter Sensor";
            else if (boardType == BOARD_V3)
                boardStr = "V3 DspPic CPU";
            else if (boardType == BOARD_SDR24)
                boardStr = "Sdr24 Board";
            printf("\nADC Board Type = %s\n\n", boardStr);
        }
    }
    if (hdr->timeRefStatus == TIME_REF_NOT_LOCKED)
        lockStr = "Not Locked";
    else if (hdr->timeRefStatus == TIME_REF_WAS_LOCKED)
        lockStr = "Was Locked";
    else if (hdr->timeRefStatus == TIME_REF_LOCKED)
        lockStr = "Locked";
    else
        lockStr = "????";

    if (displayInfo) {
        printf("ID=%d Time=%02d/%02d/%02d %02d:%02d:%02d.%03d errors=%d Time Ref Status=%s             ", hdr->packetID,
               st->wMonth, st->wDay, st->wYear % 100, st->wHour, st->wMinute, st->wSecond, st->wMilliseconds,
               consecutiveErrors, lockStr);

#ifdef WIN32
        printf("\r");
#else
        printf("\n");
#endif
        fflush(stdout);
    }

    static FILE *debug_fp = NULL;

    if (debug_fp == NULL) {
        // debug_fp = fopen("/tmp/debug2.log", "a+");
    }

    /* Now demux the data */
    samples = config.sampleRate;
    if (boardType == BOARD_VM || boardType == BOARD_SDR24) {
        lpData = (LONG *)adcData;
        index = 0;
        while (samples--) {
            for (c = 0; c != config.numberChannels; c++) {
                demuxData[c][index] = *lpData++;
                // fprintf(debug_fp, "%d ", demuxData[c][index]);
            }
            ++index;
            // fprintf(debug_fp, "\n");
        }
    } else {
        spData = (short *)adcData;
        index = 0;
        while (samples--) {
            for (c = 0; c != config.numberChannels; c++)
                demuxData[c][index] = *spData++;
            ++index;
        }
    }
    /* The user should add code here to save and or display the data */

    if (WriteLogFile(&lf, hdr, (LONG *)adcData)) {
    }
}

/* Called when new raw GPS data is sent by the DLL. Use the ADC_CMD_GPS_DATA_ON and
  ADC_CMD_GPS_DATA_OFF commands to receive or stop raw GPS data. */
void NewGpsData(BYTE *data, DWORD dataLen) {
    data[dataLen] = 0;
    if (config.timeRefType != TIME_REF_MOT_BIN)
        printf("\n%s", data);
    else
        printf("\nNew GPS Data\n");
}

/* Called when the user pressed the 'i' key. Displays the DLLInfo structure */
void DisplayDllInfo() {
    DLLInfo dllInfo;

    /* Get the DLL Information */
    if (!PSNGetBoardInfo(hBoard, ADC_GET_DLL_INFO, &dllInfo)) {
        printf("\nPSNGetBoardInfo Error\n");
        return;
    }
    printf("\nDLL Information:\n");
    printf("  Max InQueue=%d UserQueue=%d OutQueue=%d CRC Errors=%d\n", dllInfo.maxInQueue, dllInfo.maxUserQueue,
           dllInfo.maxOutQueue, dllInfo.crcErrors);
    printf("  UserQueueFullCount=%d XmitQueueFillCount=%d CPU Loop Errors=%d\n", dllInfo.userQueueFullCount,
           dllInfo.xmitQueueFullCount, dllInfo.cpuLoopErrors);

    fflush(stdout);
}

/* Display the various messages from the DLL or ADC board. */
void DisplayMsg(DWORD type, char *string) {
    char *preStr;

    switch (type) {
    case ADC_MSG:
        preStr = "DLLMsg";
        break;
    case ADC_ERROR:
        preStr = "DLLError";
        break;
    case ADC_AD_MSG:
        preStr = "AdcMsg";
        break;
    default:
        preStr = "???";
        break;
    }
    if (displayInfo)
        printf("\n%s=%s\n", preStr, string);
    else
        printf("%s=%s\n", preStr, string);

    fflush(stdout);
}

/* Displays DLL and ADC board information. Called when a ADC_STATUS message is sent
   by the DLL. The DLL will send this message when a ADC_CMD_SEND_STATUS command
   is sent to the DLL. */
void DisplayStatus(StatusInfo *sts) {
    TimeInfo *ti = &sts->timeInfo;

    printf("\nStatus Information:\n");
    printf("    BoardType=%d Version=%d.%d NumberChannels=%d SampleRate=%d LockStatus=%d\n", sts->boardType,
           sts->majorVersion, sts->minorVersion, sts->numChannels, sts->spsRate, sts->lockStatus);

    printf("    CrcErrors=%d PacketsSent=%d RetranNumber=%d RetranError=%d IncomingPackets=%d\n", sts->crcErrors,
           sts->numProcessed, sts->numRetran, sts->numRetranErr, sts->packetsRcvd);

    printf("    AddFlag=%d AddDropCount=%d WWVWidth=%d LockTime=%d AdjustNum=%d TimeDiff=%d Offset=%d\n",
           ti->addDropFlag, ti->addDropCount, ti->pulseWidth, ti->timeLocked, ti->adjustNumber, ti->averageTimeDiff,
           ti->timeOffset);
    fflush(stdout);
}

/* Saves time adjustment information to a file. Called when a ADC_SAVE_TIME_INFO message
   is sent by the DLL */
void SaveTimeInfo(TimeInfo *info) {
    FILE *fp;

    if (!(fp = fopen(TIME_FILE_NAME, "w")))
        return;
    fprintf(fp, "%d %d %d\n", info->addDropFlag, info->addDropCount, info->pulseWidth);
    fclose(fp);
}

/* Reads the time adjustment information from a file. This information is then sent
   to the DLL.*/
void ReadTimeInfo(TimeInfo *info) {
    FILE *fp;
    char buff[256];
    int cnt, flag, addDrop, width;

    memset(info, 0, sizeof(TimeInfo));
    if (!(fp = fopen(TIME_FILE_NAME, "r")))
        return;
    if (fgets(buff, 127, fp) != NULL) {
        cnt = sscanf(buff, "%d %d %d", &flag, &addDrop, &width);
        if (cnt == 3) {
            info->addDropFlag = flag;
            info->addDropCount = addDrop;
            info->pulseWidth = width;
        }
    }
    fclose(fp);
}

/* This function fills in the configuration structure with information needed
   to run the DLL and ADC board */
void MakeConfig(AdcBoardConfig2 *cfg) {
    TimeInfo timeInfo;

    memset(cfg, 0, sizeof(AdcBoardConfig2));

    ReadTimeInfo(&timeInfo);

    strcpy(cfg->commPortTcpHost, commPortStr);
    cfg->commPort = 1; // use port number under Windows
    cfg->commSpeed = DEF_PORT_SPEED;
    cfg->numberChannels = DEF_NUMBER_CHANNELS;
    cfg->sampleRate = DEF_SAMPLE_RATE;
    cfg->timeRefType = DEF_TIME_REF;
    cfg->highToLowPPS = DEF_HIGH_TO_LOW_PPS;
    cfg->noPPSLedStatus = DEF_NO_LED_STATUS;
    cfg->mode12BitFlags = DEF_12BIT_FLAGS;
    cfg->timeOffset = DEF_TIME_OFFSET;
    cfg->checkPCTime = DEF_CHECK_TIME;
    if (DEF_SET_PC_TIME)
        cfg->setPCTime = SET_PC_TIME_NORMAL;
    cfg->addDropTimer = timeInfo.addDropCount;
    cfg->addDropMode = timeInfo.addDropFlag;
    cfg->pulseWidth = timeInfo.pulseWidth;
}

#ifndef WIN32
void _sleep(int ms) {
    usleep(ms * 1000);
}

BOOL kbhit() {
    return 0;
}

char getch() {
    return 0;
}
#endif
