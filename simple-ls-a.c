#include <sys/types.h>
#include <errno.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <sys/stat.h>

DIR *dp;
struct dirent *dirp;

static void decode_switches(int argc, char **argv);
void print_hidden_files();
void sort_files_by_size();
int compare_file_size(const void *a, const void *b);
char *current_directory = NULL;


int main(int argc, char *argv[]) {
    int i;

    if (optind >= argc) {
        fprintf(stderr, "No directory name provided.\n");
        exit(EXIT_FAILURE);
    }

    // Open the directory and check for errors
    if ((dp = opendir(argv[optind])) == NULL) {
        fprintf(stderr, "Unable to open '%s': %s\n", argv[optind], strerror(errno));
        exit(EXIT_FAILURE);
    }

    decode_switches(argc, argv);

    return EXIT_SUCCESS;
}

static void decode_switches(int argc, char **argv) {
    int c;
    int sort_by_size = 0; // Flag to sort by file size

    while ((c = getopt(argc, argv, "alS")) != -1) {
        switch (c) {
            case 'a':
                print_hidden_files();
                break;
            case 'l':
                // TODO: Implement 'l' option
                break;
            case 'S':
                sort_by_size = 1;
                break;
            default:
                fprintf(stderr, "Usage: %s [-a] [-l] [-S] dir_name\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    // Check if there are additional arguments (i.e., the directory name)
    if (optind < argc) {
        current_directory = argv[optind];
    }

    if (sort_by_size) {
        sort_files_by_size(current_directory);
    }
}

void print_hidden_files() {
    while ((dirp = readdir(dp)) != NULL) {
        printf("%s\n", dirp->d_name);
    }
}

void sort_files_by_size(const char *directory) {
    // Open the specified directory
    DIR *dp = opendir(directory);

    if (dp == NULL) {
        fprintf(stderr, "Unable to open '%s': %s\n", directory, strerror(errno));
        return;
    }

    struct dirent **namelist;
    int num_files = 0;

    while ((dirp = readdir(dp)) != NULL) {
        if (dirp->d_name[0] != '.') {  // Skip hidden files
            num_files++;
        }
    }

    if (num_files == 0) {
        closedir(dp);
        return;
    }

    // Rewind the directory stream to the beginning
    rewinddir(dp);

    namelist = (struct dirent **)malloc(num_files * sizeof(struct dirent *));

    int i = 0;
    while ((dirp = readdir(dp)) != NULL) {
        if (dirp->d_name[0] != '.') {  // Skip hidden files
            namelist[i++] = dirp;
        }
    }

    qsort(namelist, num_files, sizeof(struct dirent *), compare_file_size);

    for (int j = 0; j < num_files; j++) {
        printf("%s\n", namelist[j]->d_name);
    }

    closedir(dp);

    // Free memory used for namelist
    free(namelist);
}



int compare_file_size(const void *a, const void *b) {
    struct dirent entry_a = *(struct dirent *)a;
    struct dirent entry_b = *(struct dirent *)b;

    struct stat stat_a, stat_b;
    stat(entry_a->d_name, &stat_a);
    stat(entry_b->d_name, &stat_b);

    return stat_b.st_size - stat_a.st_size; // Sort largest first
}