#include <sys/types.h>
#include <errno.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>

DIR *dp;
struct dirent *dirp;

static void decode_switches(int argc, char **argv);
void print_hidden_files();

int main(int argc, char* argv[]) {
    int i;

    if (optind >= argc) {
        fprintf(stderr, "No directory name provided.\n");
        exit(EXIT_FAILURE);
    }

    // char* dir_name = argv[optind];
    if ((dp = opendir(argv[1])) == NULL) {
        fprintf(stderr, "Unable to open '%s': %s\n", argv[1], strerror(errno));
        exit(EXIT_FAILURE);
    }

    decode_switches(argc, argv);

    return EXIT_SUCCESS;
}

static void decode_switches(int argc, char **argv) {
    int c;
    // làm option nào thì tự thêm dô, ví dụ như dưới là có sẵn a, l
    while ((c = getopt(argc, argv, "al")) != -1) {
        switch (c) {
            case 'a':
                //TODO
                print_hidden_files();
                break;
            case 'l':
                //TODO
                break;
            default:
                fprintf(stderr, "Usage: %s [-a] [-l] dir_name\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }
}

void print_hidden_files(){
    int show_hidden = 1;

    while ((dirp = readdir(dp)) != NULL) {
        if (!show_hidden && dirp->d_name[0] == '.') {
            continue; 
        }
        printf("%s\n", dirp->d_name);
    }
}