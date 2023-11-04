#include <sys/stat.h>
#include <sys/types.h>

#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <stdlib.h>
#include <getopt.h>
#include <time.h>
#include <unistd.h>
#include <pwd.h>
#include <grp.h>
#include <linux/limits.h>

int include_hidden = 0;            // -a
int display_details = 0;           // -l
int sort_by_access_time = 0;       // -u
int sort_by_modification_time = 0; // -t
int list_all_except_dot = 0;       // -A
int sort_by_file_size = 0;         // -S

void option_switches(int argc, char **argv);
void print_dir(char *directory, int include_hidden, int display_details);
void print_long_format(char *directory, struct dirent *dirp);

int main(int argc, char **argv)
{
    // Get the directory (if provided)
    char *directory = (optind < argc) ? argv[optind] : ".";

    option_switches(argc, argv);

    print_dir(directory, include_hidden, display_details);

    return 0;
}

void print_dir(char *directory, int include_hidden, int display_details)
{
    DIR *dir;
    struct dirent *dirp;
    dir = opendir(directory);
    if (dir == NULL)
    {
        perror("opendir");
        exit(EXIT_FAILURE);
    }
    while ((dirp = readdir(dir)) != NULL)
    {
        if (!include_hidden && dirp->d_name[0] == '.')
        {
            continue;
        }
        if (display_details)
        {
            print_long_format(directory, dirp);
        }
        printf(" %s\n", dirp->d_name);
    }

    closedir(dir);
}

void print_long_format(char *directory, struct dirent *dirp)
{
    char filename[PATH_MAX];
    snprintf(filename, sizeof(filename), "%s/%s", directory, dirp->d_name);
    struct stat file_stat;
    if (stat(filename, &file_stat) == -1)
    {
        perror("stat");
        exit(EXIT_FAILURE);
    }

    // File type and permissions
    printf((S_ISDIR(file_stat.st_mode)) ? "d" : "-");
    printf((file_stat.st_mode & S_IRUSR) ? "r" : "-");
    printf((file_stat.st_mode & S_IWUSR) ? "w" : "-");
    printf((file_stat.st_mode & S_IXUSR) ? "x" : "-");
    printf((file_stat.st_mode & S_IRGRP) ? "r" : "-");
    printf((file_stat.st_mode & S_IWGRP) ? "w" : "-");
    printf((file_stat.st_mode & S_IXGRP) ? "x" : "-");
    printf((file_stat.st_mode & S_IROTH) ? "r" : "-");
    printf((file_stat.st_mode & S_IWOTH) ? "w" : "-");
    printf((file_stat.st_mode & S_IXOTH) ? "x" : "-");

    // Number of links
    printf(" %ld", file_stat.st_nlink);

    // Owner name
    struct passwd *pw = getpwuid(file_stat.st_uid);
    if (pw != NULL)
    {
        printf(" %s", pw->pw_name);
    }
    else
    {
        printf(" %d", file_stat.st_uid);
    }

    // Group name
    struct group *gr = getgrgid(file_stat.st_gid);
    if (gr != NULL)
    {
        printf(" %s", gr->gr_name);
    }
    else
    {
        printf(" %d", file_stat.st_gid);
    }

    // File size
    printf(" %lld", (long long)file_stat.st_size);

    // File modification time
    struct tm *modified_time = localtime(&file_stat.st_mtime);
    char time_str[80];
    strftime(time_str, sizeof(time_str), "%b %d %H:%M", modified_time);
    printf(" %s", time_str);
}

void option_switches(int argc, char **argv)
{
    int option;
    while ((option = getopt(argc, argv, "alut:A:S")) != -1)
    {
        switch (option)
        {
        case 'a':
            include_hidden = 1;
            break;
        case 'l':
            display_details = 1;
            break;
        case 'u':
            sort_by_access_time = 1;
            break;
        case 't':
            sort_by_modification_time = 1;
            break;
        case 'A':
            list_all_except_dot = 1;
            break;
        case 'S':
            sort_by_file_size = 1;
            break;
        default:
            fprintf(stderr, "Invalid option\n");
            exit(EXIT_FAILURE);
        }
    }
}
