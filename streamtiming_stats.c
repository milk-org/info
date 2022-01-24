/** @file streamtiming_stats.c
 */

#include <math.h>
#include <ncurses.h>
#include <sched.h>

#include "CommandLineInterface/CLIcore.h"

#include "COREMOD_memory/COREMOD_memory.h"
#include "COREMOD_tools/COREMOD_tools.h"
#include "timediff.h"

// ==========================================
// Forward declaration(s)
// ==========================================

errno_t info_image_streamtiming_stats_disp(double *tdiffvarray,
                                           long    NBsamples,
                                           double  tdiffvmax,
                                           long    tdiffcntmax,
                                           long    NBpercbin);

//
//
errno_t info_image_streamtiming_stats(imageID ID,
                                      int     sem,
                                      long    NBsamplesmax,
                                      float   samplestimeout,
                                      long    NBpercbin)
{
    long cnt;

    double *tdiffvarray;
    tdiffvarray = (double *) malloc(sizeof(double) * NBsamplesmax);

    // warmup
    for (cnt = 0; cnt < SEMAPHORE_MAXVAL; cnt++)
    {
        sem_wait(data.image[ID].semptr[sem]);
    }

    // collect timing data
    long cnt0 = data.image[ID].md[0].cnt0;

    struct timespec tstart;
    struct timespec t0;
    struct timespec t1;
    struct timespec tdiff;
    double          tdiffv;

    sem_wait(data.image[ID].semptr[sem]);
    clock_gettime(CLOCK_REALTIME, &tstart);
    t0.tv_sec  = tstart.tv_sec;
    t0.tv_nsec = tstart.tv_nsec;

    double tdiffvmax   = 0.0;
    long   tdiffcntmax = 0;

    int  loopOK   = 1;
    long framecnt = 0;
    while (loopOK == 1)
    {
        //for (long framecnt = 0; framecnt < NBsamplesmax; framecnt++)
        sem_wait(data.image[ID].semptr[sem]);
        clock_gettime(CLOCK_REALTIME, &t1);
        tdiff                 = info_time_diff(t0, t1);
        tdiffv                = 1.0 * tdiff.tv_sec + 1.0e-9 * tdiff.tv_nsec;
        tdiffvarray[framecnt] = tdiffv;
        t0.tv_sec             = t1.tv_sec;
        t0.tv_nsec            = t1.tv_nsec;

        if (tdiffv > tdiffvmax)
        {
            tdiffvmax   = tdiffv;
            tdiffcntmax = framecnt;
        }
        framecnt++;
        if (framecnt == NBsamplesmax)
        {
            loopOK = 0;
        }

        tdiff              = info_time_diff(tstart, t1);
        double tdiffstartv = 1.0 * tdiff.tv_sec + 1.0e-9 * tdiff.tv_nsec;
        if (tdiffstartv > samplestimeout)
        {
            loopOK = 0;
        }
    }
    long cntdiff   = data.image[ID].md[0].cnt0 - cnt0 - 1;
    long NBsamples = framecnt;

    info_image_streamtiming_stats_disp(tdiffvarray,
                                       NBsamples,
                                       tdiffvmax,
                                       tdiffcntmax,
                                       NBpercbin);

    free(tdiffvarray);

    return RETURN_SUCCESS;
}




errno_t info_image_streamtiming_stats_disp(double *tdiffvarray,
                                           long    NBsamples,
                                           double  tdiffvmax,
                                           long    tdiffcntmax,
                                           long    NBpercbin)
{
    float RMSval = 0.0;
    float AVEval = 0.0;

    int percMedianIndex;

    float *percarray;
    long  *percNarray;

    // printw("ALLOCATE arrays\n");
    percarray  = (float *) malloc(sizeof(float) * NBpercbin);
    percNarray = (long *) malloc(sizeof(long) * NBpercbin);

    long perccnt = 0;

    float p;
    long  N;

    for (N = 1; N < 5; N++)
    {
        if (N < NBsamples)
        {
            percNarray[perccnt] = N;
            percarray[perccnt]  = 1.0 * N / NBsamples;
            perccnt++;
        }
    }

    for (p = 0.0001; p < 0.1; p *= 10.0)
    {
        N = (long) (p * NBsamples);
        if ((N > percNarray[perccnt - 1]) && (perccnt < NBpercbin - 1))
        {
            percNarray[perccnt] = N;
            percarray[perccnt]  = 1.0 * N / NBsamples;
            perccnt++;
        }
    }

    for (p = 0.1; p < 0.41; p += 0.1)
    {
        N = (long) (p * NBsamples);
        if ((N > percNarray[perccnt - 1]) && (perccnt < NBpercbin - 1))
        {
            percNarray[perccnt] = N;
            percarray[perccnt]  = 1.0 * N / NBsamples;
            perccnt++;
        }
    }

    p                   = 0.5;
    N                   = (long) (p * NBsamples);
    percNarray[perccnt] = N;
    percarray[perccnt]  = 1.0 * N / NBsamples;
    percMedianIndex     = perccnt;
    perccnt++;

    for (p = 0.6; p < 0.91; p += 0.1)
    {
        N = (long) (p * NBsamples);
        if ((N > percNarray[perccnt - 1]) && (perccnt < NBpercbin - 1))
        {
            percNarray[perccnt] = N;
            percarray[perccnt]  = 1.0 * N / NBsamples;
            perccnt++;
        }
    }

    for (p = 0.9; p < 0.999; p = p + 0.5 * (1.0 - p))
    {
        N = (long) (p * NBsamples);
        if ((N > percNarray[perccnt - 1]) && (perccnt < NBpercbin - 1))
        {
            percNarray[perccnt] = N;
            percarray[perccnt]  = 1.0 * N / NBsamples;
            perccnt++;
        }
    }

    for (long N1 = 5; N1 > 0; N1--)
    {
        N = NBsamples - N1;
        if ((N > percNarray[perccnt - 1]) && (perccnt < NBpercbin - 1) &&
            (N > 0))
        {
            percNarray[perccnt] = N;
            percarray[perccnt]  = 1.0 * N / NBsamples;
            perccnt++;
        }
    }




    // process timing data
    quick_sort_double(tdiffvarray, NBsamples);

    long i;
    for (i = 0; i < NBsamples; i++)
    {
        AVEval += tdiffvarray[i];
        RMSval += tdiffvarray[i] * tdiffvarray[i];
    }
    RMSval = RMSval / NBsamples;
    AVEval /= NBsamples;
    RMSval = sqrt(RMSval - AVEval * AVEval);

    printw("\n NBsamples = %ld  (cntdiff = %ld)     NBpercbin = %ld\n\n",
           NBsamples,
           tdiffcntmax,
           NBpercbin);




    for (int percbin = 0; percbin < NBpercbin; percbin++)
    {
        if (percbin == percMedianIndex)
        {
            attron(A_BOLD);
            printw(
                "%2d/%2d  %6.3f% \%  %6.3f% \%  [%10ld] [%10ld]    %10.3f us\n",
                percbin,
                NBpercbin,
                100.0 * percarray[percbin],
                100.0 * (1.0 - percarray[percbin]),
                percNarray[percbin],
                NBsamples - percNarray[percbin],
                1.0e6 * tdiffvarray[percNarray[percbin]]);
            attroff(A_BOLD);
        }
        else
        {
            if (tdiffvarray[percNarray[percbin]] >
                1.2 * tdiffvarray[percNarray[percMedianIndex]])
            {
                attron(A_BOLD | COLOR_PAIR(4));
            }
            if (tdiffvarray[percNarray[percbin]] >
                1.5 * tdiffvarray[percNarray[percMedianIndex]])
            {
                attron(A_BOLD | COLOR_PAIR(5));
            }
            if (tdiffvarray[percNarray[percbin]] >
                1.99 * tdiffvarray[percNarray[percMedianIndex]])
            {
                attron(A_BOLD | COLOR_PAIR(6));
            }

            printw(
                "%2d/%2d  %6.3f% \%  %6.3f% \%  [%10ld] [%10ld]    %10.3f us   "
                " %+10.3f us\n",
                percbin,
                NBpercbin,
                100.0 * percarray[percbin],
                100.0 * (1.0 - percarray[percbin]),
                percNarray[percbin],
                NBsamples - percNarray[percbin],
                1.0e6 * tdiffvarray[percNarray[percbin]],
                1.0e6 * (tdiffvarray[percNarray[percbin]] -
                         tdiffvarray[percNarray[percMedianIndex]]));
        }
    }
    attroff(A_BOLD | COLOR_PAIR(4));

    printw("\n  Average Time Interval = %10.3f us    -> frequ = %10.3f Hz\n",
           1.0e6 * AVEval,
           1.0 / AVEval);
    printw("                    RMS = %10.3f us  ( %5.3f \%)\n",
           1.0e6 * RMSval,
           100.0 * RMSval / AVEval);
    printw("  Max delay : %10.3f us   frame # %ld\n",
           1.0e6 * tdiffvmax,
           tdiffcntmax);


    free(percarray);
    free(percNarray);

    return RETURN_SUCCESS;
}
