#include <opensocdebug.h>

#include <getopt.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include "cli.h"

static void print_help(void) {
    fprintf(stderr, "Usage: osd-cli <parameters>\n"                                        );
    fprintf(stderr, "\n"                                                                   );
    fprintf(stderr, "Parameters:\n"                                                        );
    fprintf(stderr, "  -h, --help                  Print this help\n"                      );
    fprintf(stderr, "  -s <file>, --source=<file>  Read commands from file at start\n"     );
    fprintf(stderr, "  -b <file>, --batch=<file>   Read commands from file and exit\n"     );
    fprintf(stderr, "  --python                    Interpret -s and -b as python script\n" );
}

static void print_help_commands(void) {
    fprintf(stderr, "Available commands:\n"                    );
    fprintf(stderr, "  help        Print this help\n"          );
    fprintf(stderr, "  <cmd> help  Print help for command\n"   );
    fprintf(stderr, "  quit        Exit the command line\n"    );
    fprintf(stderr, "  reset       Reset the system\n"         );
    fprintf(stderr, "  start       Start the processor cores\n");
    fprintf(stderr, "  mem         Access memory\n"            );
    fprintf(stderr, "  wait        Wait for given seconds\n"   );
}

static void print_help_reset(void) {
    fprintf(stderr, "Available parameters:\n"                           );
    fprintf(stderr, "  -halt       Halt processor cores until 'start'\n");
}

static void print_help_start(void) {
    fprintf(stderr, "Start cores after 'reset', no parameters\n");
}

static void print_help_mem(void) {
    fprintf(stderr, "Available subcommands:\n"                          );
    fprintf(stderr, "  help        Print this help\n"                   );
    fprintf(stderr, "  test        Run memory tests\n"                  );
    fprintf(stderr, "  loadelf     Load an elf to memory\n"             );
}

static void print_help_mem_loadelf(void) {
    fprintf(stderr, "Usage: mem loadelf <file> <memid>\n"   );
    fprintf(stderr, "  file   Filename to load\n"           );
    fprintf(stderr, "  memid  Module identifier of memory\n");
}

static void print_help_wait(void) {
    fprintf(stderr, "Usage: wait <n>\n"         );
    fprintf(stderr, "  n  Number of seconds\n"  );
}

#define CHECK_MATCH(input, string) \
        (input && !strncmp(input, string, strlen(string)+1))

static int interpret(struct osd_context *ctx, char *line) {
    char *cmd = strtok(line, " ");

    if (!cmd) {
        return 0;
    }

    if (CHECK_MATCH(cmd, "help") ||
            CHECK_MATCH(cmd, "h")) {
        print_help_commands();
    } else if (CHECK_MATCH(cmd, "quit") ||
            CHECK_MATCH(cmd, "q") ||
            CHECK_MATCH(cmd, "exit")) {
        return 1;
    } else if (CHECK_MATCH(cmd, "reset")) {
        int haltcpus = 0;
        char *subcmd = strtok(NULL, " ");

        if (CHECK_MATCH(subcmd, "help")) {
            print_help_reset();
        } else if (CHECK_MATCH(subcmd, "-halt")) {
            haltcpus = 1;
        }

        osd_reset_system(ctx, haltcpus);
    } else if (CHECK_MATCH(cmd, "start")) {
        char *subcmd = strtok(NULL, " ");

        if (CHECK_MATCH(subcmd, "help")) {
            print_help_start();
        } else if (subcmd) {
            fprintf(stderr, "No parameters accepted or unknown subcommand: %s", subcmd);
            print_help_start();
        } else {
            osd_start_cores(ctx);
        }
    } else if (CHECK_MATCH(cmd, "mem")) {
        char *subcmd = strtok(NULL, " ");

        if (CHECK_MATCH(subcmd, "help")) {
            print_help_mem();
        } else if (CHECK_MATCH(subcmd, "test")) {
            memory_tests(ctx);
        } else if (CHECK_MATCH(subcmd, "loadelf")) {
            subcmd = strtok(NULL, " ");

            if (CHECK_MATCH(subcmd, "help")) {
                print_help_mem_loadelf();
                return 0;
            } else if (!subcmd){
                fprintf(stderr, "Missing filename\n");
                print_help_mem_loadelf();
                return 0;
            }
            char *file = subcmd;
            char *smem = strtok(NULL, " ");

            if (!smem) {
                fprintf(stderr, "Missing memory id\n");
                print_help_mem_loadelf();
                return 0;
            }

            errno = 0;
            unsigned int mem = strtol(smem, 0, 0);
            if (errno != 0) {
                fprintf(stderr, "Invalid memory id: %s\n", smem);
                print_help_mem_loadelf();
                return 0;
            }
            osd_memory_loadelf(ctx, mem, file);
        }
    } else if (CHECK_MATCH(cmd, "wait")) {
        char *subcmd = strtok(NULL, " ");

        if (CHECK_MATCH(subcmd, "help")) {
            print_help_wait();
            return 0;
        }

        if (subcmd) {
            errno = 0;
            unsigned int sec = strtol(subcmd, 0, 0);
            if (errno != 0) {
                fprintf(stderr, "No valid seconds given: %s\n", subcmd);
                return 0;
            }

            sleep(sec);
        }
    } else {
        fprintf(stderr, "Unknown command: %s\n", cmd);
        print_help_commands();
    }

    return 0;
}

int main(int argc, char* argv[]) {
    struct osd_context *ctx;

    int c;
    char *source = NULL;
    int batch = 0;

    while (1) {
        static struct option long_options[] = {
            {"help",        no_argument,       0, 'h'},
            {"source",      required_argument, 0, 's'},
            {"batch",       required_argument, 0, 'b'},
            {"python",      no_argument,       0, '0'},
            {0, 0, 0, 0}
        };
        int option_index = 0;

        c = getopt_long(argc, argv, "hs:b:0", long_options, &option_index);
        if (c == -1) {
            break;
        }

        switch (c) {
        case 0:
            /* If this option set a flag, do nothing else now.   */
            if (long_options[option_index].flag != 0) {
                break;
            }
            break;
        case 's':
            source = optarg;
            break;
        case 'b':
            source = optarg;
            batch = 1;
            break;
        case '0':
            fprintf(stderr, "Python not supported\n");
            break;
        case 'h':
            print_help();
            return 0;
        default:
            print_help();
            return -1;
        }
    }

    osd_new(&ctx, OSD_MODE_DAEMON, 0, 0);

    if (osd_connect(ctx) != OSD_SUCCESS) {
        fprintf(stderr, "Cannot connect to Open SoC Debug daemon\n");
        exit(1);
    }

    char *line = 0;

    if (source != NULL) {
        FILE* fp = fopen(source, "r");
        if (fp == NULL) {
            fprintf(stderr, "cannot open file '%s'\n", source);
            return 1;
        }

        size_t n = 64;
        line = malloc(64);
        ssize_t len;
        while ((len = getline(&line, &n, fp)) > 0) {
            if (line[len-1] == '\n') {
                line[len-1] = 0;
            }
            printf("execute: %s\n", line);
            int rv = interpret(ctx, line);
            if (rv != 0) {
                return rv;
            }
        }

        free(line);
        line = 0;

        if (batch != 0) {
            return 0;
        }
    }

    while (1) {
        if (line) {
            free(line);
        }
        line = readline("osd> ");
        add_history(line);

        int rv = interpret(ctx, line);
        if (rv != 0) {
            return rv;
        }
    }
}
