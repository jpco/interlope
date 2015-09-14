#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>

typedef struct {
    size_t interval;
    int signal;
    char *cmd;
} opts_t;

void parse_opts (int argc, char **argv, opts_t *opts_ptr)
{
    opts_ptr->interval = 4000;
    opts_ptr->signal = SIGRTMIN+1;
    opts_ptr->cmd = "ls";
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

    // set up signal
    if (signal (opts.signal, interrupt) == SIG_ERR) {
        perror("Could not establish signal");
        exit (EXIT_FAILURE);
    }

    // loop
    while (1) {
        int status;
        if ((status = system (opts.cmd))) {
            if (status == 127) {
                // fork/exec error
                fprintf (stderr, "Error fork/execing child.");
            } else {
                // error in child process
                fprintf (stderr, "Error in child process.");
            }
        }

        size_t i;
        for (i = 0; i < opts.interval; i++) {
            if (inter) break;
            usleep (1000);
        }
        inter = 0;
    }

    return 0;
}
