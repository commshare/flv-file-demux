#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <fcntl.h>

#include "mp_msg.h"

/* maximum message length of mp_msg */
#define MSGSIZE_MAX 3072

//int mp_msg_levels[MSGT_MAX]; // verbose level of this module. inited to -2
//int mp_msg_level_all = MSGL_STATUS;
int g_verbose   = 0;

FILE * g_fd = 0;

void mp_msg_init(int argc, char** argv)
{
    int i;
    for (i = 0; i < argc;   i++)
    {
        if (strncmp(argv[i], "-msglevel", 9) == 0)
        {
            i++;
            if (i < argc)
            {
                char*   ptr     = argv[i];
                int     len     = strlen(ptr);
                int     j       = 0;
                char*   pStart  = ptr;
                char*   pEnd    = NULL;
                char*   pEqual  = NULL;
                for (;  j < len;    j++)
                {
                    if (ptr[j] == ':' || j == len - 1)
                    {
                        pEnd = ptr + j - (ptr[j] == ':' ? 1 : 0);
                        pEqual = pStart;
                        while (*pEqual != '=' && pEqual < pEnd)
                        {
                            pEqual++;
                        }
                        if (*pEqual != '=')
                        {
                            fprintf(stderr, "invalid value for msglevel, skipped\n");
                            pStart = pEnd + 2;
                            pEnd = NULL;
                            continue;
                        }
                        //
                        // the string between pStart and pEqual is module id;
                        // the string between pEqual and pEnd is msglevel
                        // current we just support global msglevel.
                        //
                        // extract module and verbose level
                        //fprintf(stderr, "1 pStart = %s \n",pStart);

                        if (strncmp(pStart, "all", 3) == 0)
                        {
                            pStart = pEqual + 1;
                            //fprintf(stderr, "2\n");
                            if (*pStart == '-')
                            {
                                if (pEnd != pStart && *pEnd == '1')
                                {
                                    g_verbose = -1;
                                }
                            }
                            else if (pEnd == pStart)
                            {
                                g_verbose = *pEnd - '0';
                                //fprintf(stderr, "0\n");
                            }
                            else
                            {
                                fprintf(stderr, "invalid value for msglevel, skipped\n");
                            }
                            //fprintf(stderr, "3\n");
                            break;
                        }
                        //fprintf(stderr, "4\n");
                        pStart = pEnd + 2;
                        pEnd = NULL;
                    }
                }
            }
        }
    }

    if(g_verbose > 5)
    {
        g_fd = fopen("mplayer.log","w+");
    }
    //for(i=0;i<MSGT_MAX;i++) mp_msg_levels[i] = -2;
    //mp_msg_levels[MSGT_IDENTIFY] = -1; // no -identify output by default
}

int mp_msg_test(int mod, int lev)
{
    return lev <= g_verbose;
    //return lev <= (mp_msg_levels[mod] == -2 ? mp_msg_level_all + verbose : mp_msg_levels[mod]);
}

void mp_msg(int mod, int lev, const char* format, ...)
{
    va_list va;
    char    tmp[MSGSIZE_MAX];

    va_start(va, format);
    vsnprintf(tmp, MSGSIZE_MAX, format, va);
    va_end(va);
    tmp[MSGSIZE_MAX - 2] = '\n';
    tmp[MSGSIZE_MAX - 1] = 0;

//    if (lev <= MSGL_WARN)
//    {
//        fprintf(stderr, "%s", tmp);fflush(stderr);
//    }
//    else
    printf("%s", tmp);
    fflush(stdout);
}
