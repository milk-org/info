/** @file streamtiming_stats.c
 */

#include <math.h>
#include <ncurses.h>
#include <sched.h>

#include "COREMOD_memory/COREMOD_memory.h"
#include "COREMOD_tools/COREMOD_tools.h"
#include "CommandLineInterface/CLIcore.h"
#include "timediff.h"

// ==========================================
// Forward declaration(s)
// ==========================================

errno_t info_image_streamtiming_stats_disp(double *tdiffvarray,
                                           long    NBsamples,
                                           float  *percarray,
                                           long   *percNarray,
                                           long    NBperccnt,
                                           long    percMedianIndex,
                                           long    cntdiff,
                                           long    part,
                                           long    NBpart,
                                           double  tdiffvmax,
                                           long    tdiffcntmax);

//
//
errno_t info_image_streamtiming_stats(
    const char *ID_name, int sem, long NBsamples, long part, long NBpart)
{
    static long     ID;
    long            cnt;
    struct timespec t0;
    struct timespec t1;
    struct timespec tdiff;
    double          tdiffv;

    static double *tdiffvarray;

    static long perccnt;
    static long NBperccnt;
    static long perccntMAX = 1000;
    static int  percMedianIndex;

    static float *percarray;
    static long  *percNarray;

    if (part == 0)
        {
            // printw("ALLOCATE arrays\n");
            percarray   = (float *) malloc(sizeof(float) * perccntMAX);
            percNarray  = (long *) malloc(sizeof(long) * perccntMAX);
            tdiffvarray = (double *) malloc(sizeof(double) * NBsamples);

            perccnt = 0;

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
                    if ((N > percNarray[perccnt - 1]) &&
                        (perccnt < perccntMAX - 1))
                        {
                            percNarray[perccnt] = N;
                            percarray[perccnt]  = 1.0 * N / NBsamples;
                            perccnt++;
                        }
                }

            for (p = 0.1; p < 0.41; p += 0.1)
                {
                    N = (long) (p * NBsamples);
                    if ((N > percNarray[perccnt - 1]) &&
                        (perccnt < perccntMAX - 1))
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
                    if ((N > percNarray[perccnt - 1]) &&
                        (perccnt < perccntMAX - 1))
                        {
                            percNarray[perccnt] = N;
                            percarray[perccnt]  = 1.0 * N / NBsamples;
                            perccnt++;
                        }
                }

            for (p = 0.9; p < 0.999; p = p + 0.5 * (1.0 - p))
                {
                    N = (long) (p * NBsamples);
                    if ((N > percNarray[perccnt - 1]) &&
                        (perccnt < perccntMAX - 1))
                        {
                            percNarray[perccnt] = N;
                            percarray[perccnt]  = 1.0 * N / NBsamples;
                            perccnt++;
                        }
                }

            long N1;
            for (N1 = 5; N1 > 0; N1--)
                {
                    N = NBsamples - N1;
                    if ((N > percNarray[perccnt - 1]) &&
                        (perccnt < perccntMAX - 1) && (N > 0))
                        {
                            percNarray[perccnt] = N;
                            percarray[perccnt]  = 1.0 * N / NBsamples;
                            perccnt++;
                        }
                }
            NBperccnt = perccnt;

            int                RT_priority = 80; // any number from 0-99
            struct sched_param schedpar;

            schedpar.sched_priority = RT_priority;
#ifndef __MACH__
            sched_setscheduler(0, SCHED_FIFO, &schedpar);
#endif

            ID = image_ID(ID_name);
        } // end of part=0 case

    // warmup
    for (cnt = 0; cnt < SEMAPHORE_MAXVAL; cnt++)
        {
            sem_wait(data.image[ID].semptr[sem]);
        }

    // collect timing data
    long cnt0 = data.image[ID].md[0].cnt0;

    sem_wait(data.image[ID].semptr[sem]);
    clock_gettime(CLOCK_REALTIME, &t0);

    double tdiffvmax   = 0.0;
    long   tdiffcntmax = 0;

    for (cnt = 0; cnt < NBsamples; cnt++)
        {
            sem_wait(data.image[ID].semptr[sem]);
            clock_gettime(CLOCK_REALTIME, &t1);
            tdiff            = info_time_diff(t0, t1);
            tdiffv           = 1.0 * tdiff.tv_sec + 1.0e-9 * tdiff.tv_nsec;
            tdiffvarray[cnt] = tdiffv;
            t0.tv_sec        = t1.tv_sec;
            t0.tv_nsec       = t1.tv_nsec;

            if (tdiffv > tdiffvmax)
                {
                    tdiffvmax   = tdiffv;
                    tdiffcntmax = cnt;
                }
        }
    long cntdiff = data.image[ID].md[0].cnt0 - cnt0 - 1;

    printw("Stream : %s\n", ID_name);
    info_image_streamtiming_stats_disp(tdiffvarray,
                                       NBsamples,
                                       percarray,
                                       percNarray,
                                       NBperccnt,
                                       percMedianIndex,
                                       cntdiff,
                                       part,
                                       NBpart,
                                       tdiffvmax,
                                       tdiffcntmax);

    if (part == NBpart - 1)
        {
            // printw("FREE arrays\n");
            free(percarray);
            free(percNarray);
            free(tdiffvarray);
        }

    return RETURN_SUCCESS;
}

errno_t info_image_streamtiming_stats_disp(double *tdiffvarray,
                                           long    NBsamples,
                                           float  *percarray,
                                           long   *percNarray,
                                           long    NBperccnt,
                                           long    percMedianIndex,
                                           long    cntdiff,
                                           long    part,
                                           long    NBpart,
                                           double  tdiffvmax,
                                           long    tdiffcntmax)
{
    long  perccnt;
    float RMSval = 0.0;
    float AVEval = 0.0;

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

    printw(
        "\n NBsamples = %ld  (cntdiff = %ld)   part %3ld/%3ld   NBperccnt = "
        "%ld\n\n",
        NBsamples,
        cntdiff,
        part,
        NBpart,
        NBperccnt);

    for (perccnt = 0; perccnt < NBperccnt; perccnt++)
        {
            if (perccnt == percMedianIndex)
                {
                    attron(A_BOLD);
                    printw(
                        "%6.3f% \%  %6.3f% \%  [%10ld] [%10ld]    %10.3f us\n",
                        100.0 * percarray[perccnt],
                        100.0 * (1.0 - percarray[perccnt]),
                        percNarray[perccnt],
                        NBsamples - percNarray[perccnt],
                        1.0e6 * tdiffvarray[percNarray[perccnt]]);
                    attroff(A_BOLD);
                }
            else
                {
                    if (tdiffvarray[percNarray[perccnt]] >
                        1.2 * tdiffvarray[percNarray[percMedianIndex]])
                        {
                            attron(A_BOLD | COLOR_PAIR(4));
                        }
                    if (tdiffvarray[percNarray[perccnt]] >
                        1.5 * tdiffvarray[percNarray[percMedianIndex]])
                        {
                            attron(A_BOLD | COLOR_PAIR(5));
                        }
                    if (tdiffvarray[percNarray[perccnt]] >
                        1.99 * tdiffvarray[percNarray[percMedianIndex]])
                        {
                            attron(A_BOLD | COLOR_PAIR(6));
                        }

                    printw(
                        "%6.3f% \%  %6.3f% \%  [%10ld] [%10ld]    %10.3f us   "
                        "%+10.3f us\n",
                        100.0 * percarray[perccnt],
                        100.0 * (1.0 - percarray[perccnt]),
                        percNarray[perccnt],
                        NBsamples - percNarray[perccnt],
                        1.0e6 * tdiffvarray[percNarray[perccnt]],
                        1.0e6 * (tdiffvarray[percNarray[perccnt]] -
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

    return RETURN_SUCCESS;
}
