#include <sys/stat.h>
#include <sys/types.h>

#include <errno.h>
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

int include_hidden = 0;  // -a
int display_details = 0; // -l
int list_all_except_dot = 0; // -A
int sort_by_size = 0; // -S

struct dirent **namelist = NULL;

void print_dir(char *directory);
void option_switches(int argc, char **argv);
void print_all_hidden(DIR *dir, struct dirent *dirp);
void print_long_format(char *directory, DIR *dir, struct dirent *dirp);
void print_all_except_dot(DIR *dir, struct dirent *dirp);
void sort_files_by_size(const char *directory);
int compare_file_size(const void *a, const void *b);

int main(int argc, char **argv)
{
    char *directory = (optind < argc) ? argv[optind] : ".";
    option_switches(argc, argv);

    print_dir(directory);

    return 0;
}

void print_dir(char *directory)
{
    DIR *dir;
    struct dirent *dirp;
    dir = opendir(directory);
    if (dir == NULL)
    {
        perror("opendir");
        exit(EXIT_FAILURE);
    }

    if (include_hidden)
    {
        print_all_hidden(dir, dirp);
    }

    if (display_details)
    {
        print_long_format(directory, dir, dirp);
    }

    if (list_all_except_dot)
    {
        print_all_except_dot(dir, dirp);
    }

    if (sort_by_size)
    {
        sort_files_by_size(directory);
    }

    else
    {
        while ((dirp = readdir(dir)) != NULL)
        {
            if (dirp->d_name[0] == '.')
            {
                continue;
            }
            printf(" %s\n", dirp->d_name);
        }
    }

    closedir(dir);
}

void print_all_hidden(DIR *dir, struct dirent *dirp)
{
    while ((dirp = readdir(dir)) != NULL)
    {
        printf(" %s\n", dirp->d_name);
    }
}

void print_all_except_dot(DIR *dir, struct dirent *dirp)
{
    while ((dirp = readdir(dir)) != NULL)
    {
        if (dirp->d_name[0] == '.' && (dirp->d_name[1] == '\0' || (dirp->d_name[1] == '.' && dirp->d_name[2] == '\0')))
        {
            continue;
        }

        printf(" %s\n", dirp->d_name);
    }
}

void print_long_format(char *directory, DIR *dir, struct dirent *dirp)
{
    while ((dirp = readdir(dir)) != NULL)

    {
        if (dirp->d_name[0] == '.')
        {
            continue;
        }
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
        printf(" %s\n", dirp->d_name);
    }
}

int compare_file_size(const void *a, const void *b)
{
    struct dirent *entry_a = *(struct dirent **)a;
    struct dirent *entry_b = *(struct dirent **)b;

    char path_a[PATH_MAX], path_b[PATH_MAX];
    snprintf(path_a, PATH_MAX, "%s/%s", namelist[0]->d_name, entry_a->d_name);
    snprintf(path_b, PATH_MAX, "%s/%s", namelist[0]->d_name, entry_b->d_name);

    struct stat stat_a, stat_b;
    stat(path_a, &stat_a);
    stat(path_b, &stat_b);

    return stat_b.st_size - stat_a.st_size; // Sort largest first
}

void sort_files_by_size(const char *directory)
{

    int num_files = scandir(directory, &namelist, NULL, alphasort); // dùng ms-vscode báo lỗi thì kệ nó, compile bình thường

    if (num_files < 0)
    {
        fprintf(stderr, "Unable to scan '%s': %s\n", directory, strerror(errno));
        return;
    }

    // Filter out hidden files and directories (those starting with '.')
    int visible_files = 0;
    for (int i = 0; i < num_files; i++)
    {
        if (namelist[i]->d_name[0] != '.')
        {
            visible_files++;
        }
    }

    // Create an array to store the visible files
    struct dirent **visible_files_list = (struct dirent **)malloc(visible_files * sizeof(struct dirent *));
    if (visible_files_list == NULL)
    {
        perror("malloc");
        return;
    }

    int visible_idx = 0;
    for (int i = 0; i < num_files; i++)
    {
        if (namelist[i]->d_name[0] != '.')
        {
            visible_files_list[visible_idx++] = namelist[i];
        }
    }

    // Custom sorting based on file size in descending order
    qsort(visible_files_list, visible_files, sizeof(struct dirent *), compare_file_size);

    // Print the sorted list with file name and size
    for (int i = 0; i < visible_files; i++)
    {
        struct dirent *file = visible_files_list[i];
        struct stat st;
        char path[PATH_MAX];
        snprintf(path, PATH_MAX, "%s/%s", directory, file->d_name);
        if (stat(path, &st) == -1)
        {
            perror("stat");
            continue;
        }
        printf("%s %lld\n", file->d_name, (long long)st.st_size);
    }

    for (int i = 0; i < num_files; i++)
    {
        free(namelist[i]);
    }
    free(namelist);
}

void option_switches(int argc, char **argv)
{
    int option;
    // làm thêm cái nào thì add case vào, không cần sửa dòng bên dưới
    while ((option = getopt(argc, argv, "abcdfghiklmnopqrstuvw:xABCDFGHI:LNQRST:UXZ1")) != -1)
    {
        switch (option)
        {
        case 'a':
            include_hidden = 1;
            break;
        case 'l':
            display_details = 1;
            break;
        case 'A':
            list_all_except_dot = 1;
            break;
        case 'S':
            sort_by_size = 1;
            break;
        default:
            fprintf(stderr, "Invalid option\n");
            exit(EXIT_FAILURE);
        }
    }
}
