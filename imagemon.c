/**
 * @file    imagemon.c
 * @brief   image monitor
 */

#define NCURSES_WIDECHAR 1

#include <math.h>
#include <ncurses.h>
#include <curses.h>

#include <stdio.h>
#include <wchar.h>
#include <wctype.h>
#include <locale.h>

#include "CommandLineInterface/CLIcore.h"

#include "COREMOD_arith/COREMOD_arith.h"
#include "COREMOD_memory/COREMOD_memory.h"
#include "print_header.h"
#include "streamtiming_stats.h"
#include "timediff.h"


#include "TUItools.h"




// screen size
static short unsigned int wrow, wcol;


static long long       cntlast;
static struct timespec tlast;



// Local variables pointers
static char  *instreamname;
static float *updatefrequency;

static CLICMDARGDEF farg[] = {{CLIARG_IMG,
                               ".insname",
                               "input stream",
                               "im1",
                               CLIARG_VISIBLE_DEFAULT,
                               (void **) &instreamname,
                               NULL},
                              {CLIARG_FLOAT32,
                               ".frequ",
                               "frequency [Hz]",
                               "3.0",
                               CLIARG_VISIBLE_DEFAULT,
                               (void **) &updatefrequency,
                               NULL}};

static CLICMDDATA CLIcmddata = {
    "imgmon", "image monitor", CLICMD_FIELDS_DEFAULTS};




// detailed help
static errno_t help_function()
{
    return RETURN_SUCCESS;
}



errno_t info_image_monitor(const char *ID_name, float frequ);
errno_t printstatus(imageID ID);



static errno_t compute_function()
{
    DEBUG_TRACE_FSTART();


    INSERT_TUI_SETUP

    // define screens
    static int NBTUIscreen = 3;

    TUIscreenarray[0].index = 1;
    TUIscreenarray[0].keych = 'h';
    strcpy(TUIscreenarray[0].name, "[h] Help");

    TUIscreenarray[1].index = 2;
    TUIscreenarray[1].keych = KEY_F(2);
    strcpy(TUIscreenarray[1].name, "[F2] summary");

    TUIscreenarray[2].index = 3;
    TUIscreenarray[2].keych = KEY_F(3);
    strcpy(TUIscreenarray[2].name, "[F3] timing");


    INSERT_STD_PROCINFO_COMPUTEFUNC_INIT

    double pinfotdelay = 1.0 * processinfo->triggerdelay.tv_sec +
                         1.0e-9 * processinfo->triggerdelay.tv_nsec;
    int diplaycntinterval = (int) ((1.0 / *updatefrequency) / pinfotdelay);
    int dispcnt           = 0;

    imageID ID        = image_ID(instreamname);
    int     TUIscreen = 2;
    int     sem       = -1;

    int timingbuffinit = 0;

    INSERT_STD_PROCINFO_COMPUTEFUNC_LOOPSTART

    {

        INSTERT_TUI_KEYCONTROLS


        if (TUIinputkch == ' ')
        {
            TUI_printfw("BUFFER INIT\n");
            // flush timing buffer
            timingbuffinit = 1;
        }

        if ((dispcnt == 0) && (TUIpause == 0))
        {
            erase();

            INSERT_TUI_SCREEN_MENU
            TUI_newline();


            //TUI_printfw("Display frequency = %f Hz -> diplaycntinterval = %d\n", *updatefrequency, diplaycntinterval);
            //TUI_printfw("[ %8ld ] input ch : %d %c\n", processinfo->loopcnt, (int) TUIinputkch, TUIinputkch);

            if (TUIscreen == 1)
            {
                TUI_printfw("h / F2 / F3 : change screen\n");
                TUI_printfw("x : exit\n");
            }

            if (TUIscreen == 2)
            {
                processinfo->triggermode = PROCESSINFO_TRIGGERMODE_DELAY;
                printstatus(ID);
            }

            if (TUIscreen == 3)
            {
                processinfo->triggermode =
                    PROCESSINFO_TRIGGERMODE_IMMEDIATE; // DIIIIIIRTY

                if (sem == -1)
                {
                    int semdefault = 0;
                    sem = ImageStreamIO_getsemwaitindex(&data.image[ID],
                                                        semdefault);
                }
                long  NBtsamples     = 10000;
                float samplestimeout = pinfotdelay;
                TUI_printfw(
                    "Listening on semaphore %d, collecting %ld samples, "
                    "updating "
                    "every %.3f sec\n",
                    sem,
                    NBtsamples,
                    samplestimeout);
                TUI_printfw("Press SPACE to reset buffer\n");

                // Hack to avoid missing a large amount of frames while waiting for the processinfo trigger delay
                info_image_streamtiming_stats(
                    ID,
                    sem,
                    NBtsamples,
                    processinfo->triggerdelay.tv_sec +
                        1.0e-9 * processinfo->triggerdelay.tv_nsec,
                    timingbuffinit);
                timingbuffinit = 0;
            }
            else
            {
                sem = -1;
            }

            refresh();
        }

        dispcnt++;

        if (dispcnt > diplaycntinterval)
        {
            dispcnt = 0;
        }
    }

    INSERT_STD_PROCINFO_COMPUTEFUNC_END

    endwin();

    DEBUG_TRACE_FEXIT();
    return RETURN_SUCCESS;
}




INSERT_STD_FPSCLIfunctions




    // Register function in CLI
    errno_t
    CLIADDCMD_info__imagemon()
{
    INSERT_STD_CLIREGISTERFUNC

    return RETURN_SUCCESS;
}




errno_t printstatus(imageID ID)
{
    long          j;
    double        frequ;
    long          NBhistopt = 20;
    long         *vcnt;
    long          h;
    unsigned long cnt;
    long          i;

    int customcolor;

    float  minPV = 60000;
    float  maxPV = 0;
    double average;
    double imtotal;


    char line1[200];

    double tmp;
    double RMS = 0.0;

    double RMS01 = 0.0;
    long   vcntmax;
    int    semval;

    DEBUG_TRACEPOINT("window size %3d %3d", wcol, wrow);

    uint8_t datatype;
    {
        // Image name, type and size
        char str[STRINGMAXLEN_DEFAULT];
        char str1[STRINGMAXLEN_DEFAULT];

        TUI_printfw("%s  ", data.image[ID].name);
        datatype = data.image[ID].md[0].datatype;
        sprintf(str,
                "%s [ %6ld",
                ImageStreamIO_typename(datatype),
                (long) data.image[ID].md[0].size[0]);

        for (j = 1; j < data.image[ID].md[0].naxis; j++)
        {
            WRITE_STRING(str1,
                         "%s x %6ld",
                         str,
                         (long) data.image[ID].md[0].size[j]);

            strcpy(str, str1);
        }

        WRITE_STRING(str1, "%s]", str);
        strcpy(str, str1);

        TUI_printfw("%-28s\n", str);
    }




    TUI_printfw("[write %d] ", data.image[ID].md[0].write);
    TUI_printfw("[status %2d] ", data.image[ID].md[0].status);


    {
        // timing and counters

        struct timespec tnow;
        struct timespec tdiff;
        double          tdiffv;

        clock_gettime(CLOCK_REALTIME, &tnow);
        tdiff = info_time_diff(tlast, tnow);
        clock_gettime(CLOCK_REALTIME, &tlast);

        tdiffv  = 1.0 * tdiff.tv_sec + 1.0e-9 * tdiff.tv_nsec;
        frequ   = (data.image[ID].md[0].cnt0 - cntlast) / tdiffv;
        cntlast = data.image[ID].md[0].cnt0;

        TUI_printfw("[cnt0 %8d] [%6.2f Hz] ", data.image[ID].md[0].cnt0, frequ);
        TUI_printfw("[cnt1 %8d]\n", data.image[ID].md[0].cnt1);
    }



    TUI_printfw("[%3ld sems ", data.image[ID].md[0].sem);
    for (int s = 0; s < data.image[ID].md[0].sem; s++)
    {
        sem_getvalue(data.image[ID].semptr[s], &semval);
        TUI_printfw(" %6d ", semval);
    }
    TUI_printfw("]\n");

    TUI_printfw("[ WRITE   ", data.image[ID].md[0].sem);
    for (int s = 0; s < data.image[ID].md[0].sem; s++)
    {
        TUI_printfw(" %6d ", data.image[ID].semWritePID[s]);
    }
    TUI_printfw("]\n");

    TUI_printfw("[ READ    ", data.image[ID].md[0].sem);
    for (int s = 0; s < data.image[ID].md[0].sem; s++)
    {
        TUI_printfw(" %6d ", data.image[ID].semReadPID[s]);
    }
    TUI_printfw("]\n");

    sem_getvalue(data.image[ID].semlog, &semval);
    TUI_printfw(" [semlog % 3d] ", semval);

    TUI_printfw(" [circbuff %3d/%3d  %4ld]",
                data.image[ID].md->CBindex,
                data.image[ID].md->CBsize,
                data.image[ID].md->CBcycle);

    TUI_printfw("\n");

    average = arith_image_mean(data.image[ID].name);
    imtotal = arith_image_total(data.image[ID].name);

    if (datatype == _DATATYPE_FLOAT)
    {
        TUI_printfw("median %12g   ", arith_image_median(data.image[ID].name));
    }

    TUI_printfw("average %12g    total = %12g\n",
                imtotal / data.image[ID].md[0].nelement,
                imtotal);

    vcnt = (long *) malloc(sizeof(long) * NBhistopt);
    if (vcnt == NULL)
    {
        PRINT_ERROR("malloc returns NULL pointer");
        abort();
    }

    if (datatype == _DATATYPE_FLOAT)
    {
        minPV = data.image[ID].array.F[0];
        maxPV = minPV;

        for (h = 0; h < NBhistopt; h++)
        {
            vcnt[h] = 0;
        }
        for (unsigned long ii = 0; ii < data.image[ID].md[0].nelement; ii++)
        {
            if (data.image[ID].array.F[ii] < minPV)
            {
                minPV = data.image[ID].array.F[ii];
            }
            if (data.image[ID].array.F[ii] > maxPV)
            {
                maxPV = data.image[ID].array.F[ii];
            }
            tmp = (1.0 * data.image[ID].array.F[ii] - average);
            RMS += tmp * tmp;
            h = (long) (1.0 * NBhistopt *
                        ((float) (data.image[ID].array.F[ii] - minPV)) /
                        (maxPV - minPV));
            if ((h > -1) && (h < NBhistopt))
            {
                vcnt[h]++;
            }
        }
    }

    if (datatype == _DATATYPE_DOUBLE)
    {
        minPV = data.image[ID].array.D[0];
        maxPV = minPV;

        for (h = 0; h < NBhistopt; h++)
        {
            vcnt[h] = 0;
        }
        for (unsigned long ii = 0; ii < data.image[ID].md[0].nelement; ii++)
        {
            if (data.image[ID].array.D[ii] < minPV)
            {
                minPV = data.image[ID].array.D[ii];
            }
            if (data.image[ID].array.D[ii] > maxPV)
            {
                maxPV = data.image[ID].array.D[ii];
            }
            tmp = (1.0 * data.image[ID].array.D[ii] - average);
            RMS += tmp * tmp;
            h = (long) (1.0 * NBhistopt *
                        ((float) (data.image[ID].array.D[ii] - minPV)) /
                        (maxPV - minPV));
            if ((h > -1) && (h < NBhistopt))
            {
                vcnt[h]++;
            }
        }
    }

    if (datatype == _DATATYPE_UINT8)
    {
        minPV = data.image[ID].array.UI8[0];
        maxPV = minPV;

        for (h = 0; h < NBhistopt; h++)
        {
            vcnt[h] = 0;
        }
        for (unsigned long ii = 0; ii < data.image[ID].md[0].nelement; ii++)
        {
            if (data.image[ID].array.UI8[ii] < minPV)
            {
                minPV = data.image[ID].array.UI8[ii];
            }
            if (data.image[ID].array.UI8[ii] > maxPV)
            {
                maxPV = data.image[ID].array.UI8[ii];
            }
            tmp = (1.0 * data.image[ID].array.UI8[ii] - average);
            RMS += tmp * tmp;
            h = (long) (1.0 * NBhistopt *
                        ((float) (data.image[ID].array.UI8[ii] - minPV)) /
                        (maxPV - minPV));
            if ((h > -1) && (h < NBhistopt))
            {
                vcnt[h]++;
            }
        }
    }

    if (datatype == _DATATYPE_UINT16)
    {
        minPV = data.image[ID].array.UI16[0];
        maxPV = minPV;

        for (h = 0; h < NBhistopt; h++)
        {
            vcnt[h] = 0;
        }
        for (unsigned long ii = 0; ii < data.image[ID].md[0].nelement; ii++)
        {
            if (data.image[ID].array.UI16[ii] < minPV)
            {
                minPV = data.image[ID].array.UI16[ii];
            }
            if (data.image[ID].array.UI16[ii] > maxPV)
            {
                maxPV = data.image[ID].array.UI16[ii];
            }
            tmp = (1.0 * data.image[ID].array.UI16[ii] - average);
            RMS += tmp * tmp;
            h = (long) (1.0 * NBhistopt *
                        ((float) (data.image[ID].array.UI16[ii] - minPV)) /
                        (maxPV - minPV));
            if ((h > -1) && (h < NBhistopt))
            {
                vcnt[h]++;
            }
        }
    }

    if (datatype == _DATATYPE_UINT32)
    {
        minPV = data.image[ID].array.UI32[0];
        maxPV = minPV;

        for (h = 0; h < NBhistopt; h++)
        {
            vcnt[h] = 0;
        }
        for (unsigned long ii = 0; ii < data.image[ID].md[0].nelement; ii++)
        {
            if (data.image[ID].array.UI32[ii] < minPV)
            {
                minPV = data.image[ID].array.UI32[ii];
            }
            if (data.image[ID].array.UI32[ii] > maxPV)
            {
                maxPV = data.image[ID].array.UI32[ii];
            }
            tmp = (1.0 * data.image[ID].array.UI32[ii] - average);
            RMS += tmp * tmp;
            h = (long) (1.0 * NBhistopt *
                        ((float) (data.image[ID].array.UI32[ii] - minPV)) /
                        (maxPV - minPV));
            if ((h > -1) && (h < NBhistopt))
            {
                vcnt[h]++;
            }
        }
    }

    if (datatype == _DATATYPE_UINT64)
    {
        minPV = data.image[ID].array.UI64[0];
        maxPV = minPV;

        for (h = 0; h < NBhistopt; h++)
        {
            vcnt[h] = 0;
        }
        for (unsigned long ii = 0; ii < data.image[ID].md[0].nelement; ii++)
        {
            if (data.image[ID].array.UI64[ii] < minPV)
            {
                minPV = data.image[ID].array.UI64[ii];
            }
            if (data.image[ID].array.UI64[ii] > maxPV)
            {
                maxPV = data.image[ID].array.UI64[ii];
            }
            tmp = (1.0 * data.image[ID].array.UI64[ii] - average);
            RMS += tmp * tmp;
            h = (long) (1.0 * NBhistopt *
                        ((float) (data.image[ID].array.UI64[ii] - minPV)) /
                        (maxPV - minPV));
            if ((h > -1) && (h < NBhistopt))
            {
                vcnt[h]++;
            }
        }
    }

    if (datatype == _DATATYPE_INT8)
    {
        minPV = data.image[ID].array.SI8[0];
        maxPV = minPV;

        for (h = 0; h < NBhistopt; h++)
        {
            vcnt[h] = 0;
        }
        for (unsigned long ii = 0; ii < data.image[ID].md[0].nelement; ii++)
        {
            if (data.image[ID].array.SI8[ii] < minPV)
            {
                minPV = data.image[ID].array.SI8[ii];
            }
            if (data.image[ID].array.SI8[ii] > maxPV)
            {
                maxPV = data.image[ID].array.SI8[ii];
            }
            tmp = (1.0 * data.image[ID].array.SI8[ii] - average);
            RMS += tmp * tmp;
            h = (long) (1.0 * NBhistopt *
                        ((float) (data.image[ID].array.SI8[ii] - minPV)) /
                        (maxPV - minPV));
            if ((h > -1) && (h < NBhistopt))
            {
                vcnt[h]++;
            }
        }
    }

    if (datatype == _DATATYPE_INT16)
    {
        minPV = data.image[ID].array.SI16[0];
        maxPV = minPV;

        for (h = 0; h < NBhistopt; h++)
        {
            vcnt[h] = 0;
        }
        for (unsigned long ii = 0; ii < data.image[ID].md[0].nelement; ii++)
        {
            if (data.image[ID].array.SI16[ii] < minPV)
            {
                minPV = data.image[ID].array.SI16[ii];
            }
            if (data.image[ID].array.SI16[ii] > maxPV)
            {
                maxPV = data.image[ID].array.SI16[ii];
            }
            tmp = (1.0 * data.image[ID].array.SI16[ii] - average);
            RMS += tmp * tmp;
            h = (long) (1.0 * NBhistopt *
                        ((float) (data.image[ID].array.SI16[ii] - minPV)) /
                        (maxPV - minPV));
            if ((h > -1) && (h < NBhistopt))
            {
                vcnt[h]++;
            }
        }
    }

    if (datatype == _DATATYPE_INT32)
    {
        minPV = data.image[ID].array.SI32[0];
        maxPV = minPV;

        for (h = 0; h < NBhistopt; h++)
        {
            vcnt[h] = 0;
        }
        for (unsigned long ii = 0; ii < data.image[ID].md[0].nelement; ii++)
        {
            if (data.image[ID].array.SI32[ii] < minPV)
            {
                minPV = data.image[ID].array.SI32[ii];
            }
            if (data.image[ID].array.SI32[ii] > maxPV)
            {
                maxPV = data.image[ID].array.SI32[ii];
            }
            tmp = (1.0 * data.image[ID].array.SI32[ii] - average);
            RMS += tmp * tmp;
            h = (long) (1.0 * NBhistopt *
                        ((float) (data.image[ID].array.SI32[ii] - minPV)) /
                        (maxPV - minPV));
            if ((h > -1) && (h < NBhistopt))
            {
                vcnt[h]++;
            }
        }
    }

    if (datatype == _DATATYPE_INT64)
    {
        minPV = data.image[ID].array.SI64[0];
        maxPV = minPV;

        for (h = 0; h < NBhistopt; h++)
        {
            vcnt[h] = 0;
        }
        for (unsigned long ii = 0; ii < data.image[ID].md[0].nelement; ii++)
        {
            if (data.image[ID].array.SI64[ii] < minPV)
            {
                minPV = data.image[ID].array.SI64[ii];
            }
            if (data.image[ID].array.SI64[ii] > maxPV)
            {
                maxPV = data.image[ID].array.SI64[ii];
            }
            tmp = (1.0 * data.image[ID].array.SI64[ii] - average);
            RMS += tmp * tmp;
            h = (long) (1.0 * NBhistopt *
                        ((float) (data.image[ID].array.SI64[ii] - minPV)) /
                        (maxPV - minPV));
            if ((h > -1) && (h < NBhistopt))
            {
                vcnt[h]++;
            }
        }
    }

    RMS   = sqrt(RMS / data.image[ID].md[0].nelement);
    RMS01 = 0.9 * RMS01 + 0.1 * RMS;

    TUI_printfw("RMS = %12.6g     ->  %12.6g\n", RMS, RMS01);

    print_header(" PIXEL VALUES ", '-');
    TUI_printfw("min - max   :   %12.6e - %12.6e\n", minPV, maxPV);

    if (data.image[ID].md[0].nelement > 25)
    {
        vcntmax = 0;
        for (h = 0; h < NBhistopt; h++)
            if (vcnt[h] > vcntmax)
            {
                vcntmax = vcnt[h];
            }

        for (h = 0; h < NBhistopt; h++)
        {
            customcolor = 1;
            if (h == NBhistopt - 1)
            {
                customcolor = 2;
            }
            sprintf(line1,
                    "[%12.4e - %12.4e] %7ld",
                    (minPV + 1.0 * (maxPV - minPV) * h / NBhistopt),
                    (minPV + 1.0 * (maxPV - minPV) * (h + 1) / NBhistopt),
                    vcnt[h]);

            TUI_printfw(
                "%s",
                line1); //(minPV + 1.0*(maxPV-minPV)*h/NBhistopt), (minPV
            //+ 1.0*(maxPV-minPV)*(h+1)/NBhistopt), vcnt[h]);
            attron(COLOR_PAIR(customcolor));

            cnt = 0;
            i   = 0;
            while ((cnt < wcol - strlen(line1) - 1) && (i < vcnt[h]))
            {
                TUI_printfw(" ");
                i += (long) (vcntmax / (wcol - strlen(line1))) + 1;
                cnt++;
            }
            attroff(COLOR_PAIR(customcolor));
            TUI_printfw("\n");
        }
    }
    else
    {
        if (data.image[ID].md[0].datatype == _DATATYPE_FLOAT)
        {
            for (unsigned long ii = 0; ii < data.image[ID].md[0].nelement; ii++)
            {
                TUI_printfw("%3ld  %f\n", ii, data.image[ID].array.F[ii]);
            }
        }

        if (data.image[ID].md[0].datatype == _DATATYPE_DOUBLE)
        {
            for (unsigned long ii = 0; ii < data.image[ID].md[0].nelement; ii++)
            {
                TUI_printfw("%3ld  %f\n",
                            ii,
                            (float) data.image[ID].array.D[ii]);
            }
        }

        if (data.image[ID].md[0].datatype == _DATATYPE_UINT8)
        {
            for (unsigned long ii = 0; ii < data.image[ID].md[0].nelement; ii++)
            {
                TUI_printfw("%3ld  %5u\n", ii, data.image[ID].array.UI8[ii]);
            }
        }

        if (data.image[ID].md[0].datatype == _DATATYPE_UINT16)
        {
            for (unsigned long ii = 0; ii < data.image[ID].md[0].nelement; ii++)
            {
                TUI_printfw("%3ld  %5u\n", ii, data.image[ID].array.UI16[ii]);
            }
        }

        if (data.image[ID].md[0].datatype == _DATATYPE_UINT32)
        {
            for (unsigned long ii = 0; ii < data.image[ID].md[0].nelement; ii++)
            {
                TUI_printfw("%3ld  %5lu\n", ii, data.image[ID].array.UI32[ii]);
            }
        }

        if (data.image[ID].md[0].datatype == _DATATYPE_UINT64)
        {
            for (unsigned long ii = 0; ii < data.image[ID].md[0].nelement; ii++)
            {
                TUI_printfw("%3ld  %5lu\n", ii, data.image[ID].array.UI64[ii]);
            }
        }

        if (data.image[ID].md[0].datatype == _DATATYPE_INT8)
        {
            for (unsigned long ii = 0; ii < data.image[ID].md[0].nelement; ii++)
            {
                TUI_printfw("%3ld  %5d\n", ii, data.image[ID].array.SI8[ii]);
            }
        }

        if (data.image[ID].md[0].datatype == _DATATYPE_INT16)
        {
            for (unsigned long ii = 0; ii < data.image[ID].md[0].nelement; ii++)
            {
                TUI_printfw("%3ld  %5d\n", ii, data.image[ID].array.SI16[ii]);
            }
        }

        if (data.image[ID].md[0].datatype == _DATATYPE_INT32)
        {
            for (unsigned long ii = 0; ii < data.image[ID].md[0].nelement; ii++)
            {
                TUI_printfw("%3ld  %5ld\n",
                            ii,
                            (long) data.image[ID].array.SI32[ii]);
            }
        }

        if (data.image[ID].md[0].datatype == _DATATYPE_INT64)
        {
            for (unsigned long ii = 0; ii < data.image[ID].md[0].nelement; ii++)
            {
                TUI_printfw("%3ld  %5ld\n",
                            ii,
                            (long) data.image[ID].array.SI64[ii]);
            }
        }
    }

    free(vcnt);

    return RETURN_SUCCESS;
}
