/**
 * @file    imagemon.c
 * @brief   image monitor
 */

#include <math.h>
#include <ncurses.h>

#include "CommandLineInterface/CLIcore.h"

#include "COREMOD_arith/COREMOD_arith.h"
#include "COREMOD_memory/COREMOD_memory.h"
#include "print_header.h"
#include "streamtiming_stats.h"
#include "timediff.h"

#include "TUItools.h"


static short unsigned int wrow, wcol;


static long long       cntlast;
static struct timespec tlast;

//extern int infoscreen_wcol;
//extern int infoscreen_wrow;

// Local variables pointers
static char   *instreamname;
static double *updatefrequency;

static CLICMDARGDEF farg[] = {{CLIARG_IMG,
                               ".insname",
                               "input stream",
                               "im1",
                               CLIARG_VISIBLE_DEFAULT,
                               (void **) &instreamname,
                               NULL},
                              {CLIARG_FLOAT,
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



errno_t info_image_monitor(const char *ID_name, double frequ);




static errno_t compute_function()
{
    DEBUG_TRACE_FSTART();

    INSERT_STD_PROCINFO_COMPUTEFUNC_START

    //    printf("stream    : %s\n", instreamname);
    //    printf("Frequency = %f\n", *updatefrequency);

    info_image_monitor(instreamname, *updatefrequency);

    INSERT_STD_PROCINFO_COMPUTEFUNC_END

    DEBUG_TRACEPOINT(" ");

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
    struct timespec tnow;
    struct timespec tdiff;
    double          tdiffv;
    char            str[STRINGMAXLEN_DEFAULT];
    char            str1[STRINGMAXLEN_DEFAULT];

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

    uint8_t datatype;
    char    line1[200];

    double tmp;
    double RMS = 0.0;

    double RMS01 = 0.0;
    long   vcntmax;
    int    semval;
    long   s;

    TUI_printfw("[ %3d x %3d ] ", wcol, wrow);

    TUI_printfw("%s  ", data.image[ID].name);

    datatype = data.image[ID].md[0].datatype;

    if (datatype == _DATATYPE_UINT8)
    {
        TUI_printfw("type:  UINT8             ");
    }
    if (datatype == _DATATYPE_INT8)
    {
        TUI_printfw("type:  INT8              ");
    }

    if (datatype == _DATATYPE_UINT16)
    {
        TUI_printfw("type:  UINT16            ");
    }
    if (datatype == _DATATYPE_INT16)
    {
        TUI_printfw("type:  INT16             ");
    }

    if (datatype == _DATATYPE_UINT32)
    {
        TUI_printfw("type:  UINT32            ");
    }
    if (datatype == _DATATYPE_INT32)
    {
        TUI_printfw("type:  INT32             ");
    }

    if (datatype == _DATATYPE_UINT64)
    {
        TUI_printfw("type:  UINT64            ");
    }
    if (datatype == _DATATYPE_INT64)
    {
        TUI_printfw("type:  INT64             ");
    }

    if (datatype == _DATATYPE_FLOAT)
    {
        TUI_printfw("type:  FLOAT              ");
    }

    if (datatype == _DATATYPE_DOUBLE)
    {
        TUI_printfw("type:  DOUBLE             ");
    }

    if (datatype == _DATATYPE_COMPLEX_FLOAT)
    {
        TUI_printfw("type:  COMPLEX_FLOAT      ");
    }

    if (datatype == _DATATYPE_COMPLEX_DOUBLE)
    {
        TUI_printfw("type:  COMPLEX_DOUBLE     ");
    }

    sprintf(str, "[ %6ld", (long) data.image[ID].md[0].size[0]);

    for (j = 1; j < data.image[ID].md[0].naxis; j++)
    {
        {
            int slen = snprintf(str1,
                                STRINGMAXLEN_DEFAULT,
                                "%s x %6ld",
                                str,
                                (long) data.image[ID].md[0].size[j]);
            if (slen < 1)
            {
                PRINT_ERROR("snprintf wrote <1 char");
                abort(); // can't handle this error any other way
            }
            if (slen >= STRINGMAXLEN_DEFAULT)
            {
                PRINT_ERROR("snprintf string truncation");
                abort(); // can't handle this error any other way
            }
        }

        strcpy(str, str1);
    }

    {
        int slen = snprintf(str1, STRINGMAXLEN_DEFAULT, "%s]", str);
        if (slen < 1)
        {
            PRINT_ERROR("snprintf wrote <1 char");
            abort(); // can't handle this error any other way
        }
        if (slen >= STRINGMAXLEN_DEFAULT)
        {
            PRINT_ERROR("snprintf string truncation");
            abort(); // can't handle this error any other way
        }
    }

    strcpy(str, str1);

    TUI_printfw("%-28s\n", str);

    clock_gettime(CLOCK_REALTIME, &tnow);
    tdiff = info_time_diff(tlast, tnow);
    clock_gettime(CLOCK_REALTIME, &tlast);

    tdiffv  = 1.0 * tdiff.tv_sec + 1.0e-9 * tdiff.tv_nsec;
    frequ   = (data.image[ID].md[0].cnt0 - cntlast) / tdiffv;
    cntlast = data.image[ID].md[0].cnt0;

    TUI_printfw("[write %d] ", data.image[ID].md[0].write);
    TUI_printfw("[status %2d] ", data.image[ID].md[0].status);
    TUI_printfw("[cnt0 %8d] [%6.2f Hz] ", data.image[ID].md[0].cnt0, frequ);
    TUI_printfw("[cnt1 %8d]\n", data.image[ID].md[0].cnt1);

    TUI_printfw("[%3ld sems ", data.image[ID].md[0].sem);
    for (s = 0; s < data.image[ID].md[0].sem; s++)
    {
        sem_getvalue(data.image[ID].semptr[s], &semval);
        TUI_printfw(" %6d ", semval);
    }
    TUI_printfw("]\n");

    TUI_printfw("[ WRITE   ", data.image[ID].md[0].sem);
    for (s = 0; s < data.image[ID].md[0].sem; s++)
    {
        TUI_printfw(" %6d ", data.image[ID].semWritePID[s]);
    }
    TUI_printfw("]\n");

    TUI_printfw("[ READ    ", data.image[ID].md[0].sem);
    for (s = 0; s < data.image[ID].md[0].sem; s++)
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



errno_t info_image_monitor(const char *ID_name, double frequ)
{
    imageID ID;
    // long     mode = 0; // 0 for large image, 1 for small image
    long NBpix;
    long npix;
    int  sem;
    long cnt;

    int  MonMode = 0;
    char monstring[200];

    // 0: image summary
    // 1: timing info for sem

    ID = image_ID(ID_name);
    if (ID == -1)
    {
        printf("Image %s not found in memory\n\n", ID_name);
        fflush(stdout);

        return RETURN_SUCCESS;
    }


    npix = data.image[ID].md[0].nelement;




    // default: use ncurses
    TUI_set_screenprintmode(SCREENPRINT_NCURSES);

    if (getenv("MILK_TUIPRINT_STDIO"))
    {
        // use stdio instead of ncurses
        TUI_set_screenprintmode(SCREENPRINT_STDIO);
    }

    if (getenv("MILK_TUIPRINT_NONE"))
    {
        TUI_set_screenprintmode(SCREENPRINT_NONE);
    }

    TUI_init_terminal(&wrow, &wcol);


    int loopOK = 1;
    int freeze = 0;
    int Xexit  = 0; // toggles to 1 when users types x

    long NBtsamples = 1024;
    int  part       = 0;
    int  NBpart     = 4;

    while (loopOK == 1)
    {
        usleep((long) (1000000.0 / frequ));
        int ch = getch();



        if (freeze == 0)
        {
            //attron(A_BOLD);
            sprintf(monstring, "Mode %d   PRESS x TO STOP MONITOR", MonMode);
            //processtools__print_header(monstring, '-');
            TUI_print_header(monstring, '-');
            //attroff(A_BOLD);
        }




        switch (ch)
        {
        case 'x': // Exit control screen
            loopOK = 0;
            Xexit  = 1;
            break;
        case 's':
            MonMode = 0; // summary
            break;

        case '0':
            MonMode = 1; // Sem timing
            sem     = 0;
            break;

        case '1':
            MonMode = 1; // Sem timing
            sem     = 1;
            break;

        case '2':
            MonMode = 1; // Sem timing
            sem     = 2;
            break;

        case '+':
            if (MonMode == 1)
            {
                NBtsamples *= 2;
            }
            break;

        case '-':
            if (MonMode == 1)
            {
                NBtsamples /= 2;
            }
            break;

        case KEY_UP:
            if (MonMode == 1)
            {
                NBpart++;
                part = -1;
            }
            break;

        case KEY_DOWN:
            if (MonMode == 1)
            {
                NBpart--;
                part = -1;
            }
            break;
        }




        if (freeze == 0)
        {
            erase();


            //TUI_newline();

            if (MonMode == 0)
            {
                //clear();
                printstatus(ID);
            }

            if (MonMode == 1)
            {
                //clear();

                if (part > NBpart - 1)
                {
                    part = 0;
                }
                info_image_streamtiming_stats(ID_name,
                                              sem,
                                              NBtsamples,
                                              0,
                                              1); // part, NBpart);
                part++;
                if (part > NBpart - 1)
                {
                    part = 0;
                }
            }



            refresh();
        }
    }

    endwin();

    return RETURN_SUCCESS;
}
