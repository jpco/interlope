#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <string.h>

typedef struct {
    size_t interval;
    int signal;
    char *cmd;
} opts_t;

void print_help (void)
{
    printf ("Interlope loop interruptor 0.1\n"
            "Allows a command to loop periodically, or immediately when desired.\n"
            "To set an immediate loop, use 'pkill -RTMIN+[s argument] interlope\n\n"
            "Usage: interlope [-i interval] [-s signal] command\n"
            "Options:\n"
            " -i interval   The time interval (in milliseconds) on which to loop. "
            "If not given, will only loop on receipt of the given signal.\n"
            " -s n          The number, plus SIGRTMIN, to immediately loop the "
            "passed command.\n\n"
            "Example:\n"
            "$ interlope -i 3000 -s 2 ls\n"
            "$ pkill -RTMIN+2\n\n"
            );

    exit (EXIT_SUCCESS);
}

void parse_opts (int argc, char **argv, opts_t *opts_ptr)
{
    opts_ptr->interval = 0;
    opts_ptr->signal = -1;
    opts_ptr->cmd = NULL;

    int i;
    for (i = 1; i < argc; i++) {
        if (!strcmp (argv[i], "--help") || !strcmp (argv[i], "-h")) {
            print_help();
        } else if (!strcmp (argv[i], "-i")) {
            if (i >= argc - 1) {
                // last arg
                fprintf (stderr, "No '-i' argument given, ignoring.\n");
            } else {
                long int tmp_l = atol (argv[++i]);
                if (tmp_l <= 0) {
                    fprintf (stderr, "Bad interval argument given. Setting to 0.\n");
                } else {
                    opts_ptr->interval = (size_t)tmp_l;
                }
            }
        } else if (!strcmp (argv[i], "-s")) {
            if (i >= argc - 1) {
                // last arg
                fprintf (stderr, "No '-s' argument given, ignoring");
            } else {
                char *end;
                long int tmp_l = strtol (argv[++i], &end, 10);
                if (end == argv[i] || tmp_l < 0 || SIGRTMIN+tmp_l > SIGRTMAX) {
                    fprintf (stderr, "Bad signal argument given. Aborting.\n");
                    exit (EXIT_FAILURE);
                } else {
                    if (*end != '\0') {
                        fprintf (stderr, "Bad signal argument given? Using '%li'"
                                         " and hoping for the best...\n", tmp_l);
                    }
                    opts_ptr->signal = (int)tmp_l;
                }
            }
        } else {
            // time to parse cmd
            break;
        }
    }

    if (i == argc) {
        fprintf (stderr, "No command given; we're done here!\n");
        exit (EXIT_SUCCESS);
    } else {
        // TODO: don't require quotes around cmd argument(s)
        opts_ptr->cmd = argv[i];
    }
    
    printf ("interval: %lu\n", opts_ptr->interval);
    printf ("signal: %d\n", opts_ptr->signal);
    printf ("command: %s\n", opts_ptr->cmd);
}

char inter;

void interrupt (int waste)
{
    inter = 1;
}

int main(int argc, char **argv)
{
    // check if a shell is usable
    if (!system (NULL)) {
        fprintf (stderr, "No usable shell could be found.");
        exit (EXIT_FAILURE);
    }

    // parse opts
    opts_t opts;
    parse_opts (argc, argv, &opts);

    // set up signals
    // we have to ignore all of the RT signals except our own
    size_t i;
    for (i = SIGRTMIN; i < SIGRTMAX; i++) {
        void *status;
        fprintf (stderr, "Signalling %lu...\n", i);
        if (i == SIGRTMIN+opts.signal) {
            status = signal (i, interrupt);
        } else {
            status = signal (i, SIG_IGN);
        }
        if (status == SIG_ERR) {
            fprintf (stderr, "for i=%lu: ", i);
            perror("Could not establish signal");
            exit (EXIT_FAILURE);
        }
    }

    // loop
    while (1) {
        int status;
        if ((status = system (opts.cmd))) {
            if (status == 127) {
                // fork/exec error
                fprintf (stderr, "interlope: Error fork/execing child.\n");
            } else {
                // error in child process
                fprintf (stderr, "interlope: Error in child process.\n");
            }
        }

        if (opts.interval > 0) {
            for (i = 0; i < opts.interval; i++) {
                if (inter) break;
                usleep (1000);
            }
            inter = 0;
        } else pause();
    }

    return 0;
}
