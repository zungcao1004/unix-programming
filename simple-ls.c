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

// Cụm 1
int include_hidden = 0;            // -a
int display_details = 0;           // -l
int sort_by_access_time = 0;       // -u
int sort_by_modification_time = 0; // -t

// Cụm 2
int list_all_except_dot = 0;       // -A

// Cụm 3
int sort_by_size = 0;              // -S

struct dirent **namelist = NULL;

// Khai báo lại các hàm bên dưới 
void option_switches(int argc, char **argv);
void print_dir(char *directory, int include_hidden, int display_details, int sort_by_size);
void print_long_format(char *directory, struct dirent *dirp);
int compare_file_size(const void *a, const void *b);
void sort_files_by_size(const char *directory);

int main(int argc, char **argv)
{
    // Nếu chỉ gọi ./simple-ls thì tự hiểu thư mục là "." (thư mục hiện tại), còn nếu ./simple-ls [thư mục] thì directory=[thư mục]
    char *directory = (optind < argc) ? argv[optind] : ".";

    // Bật các flags theo các options được gọi tương ứng
    option_switches(argc, argv);

    // ae làm thêm function thì nhớ add vào print_dir, tham khảo các function đã làm ở dưới để code chung concept cho dễ sửa  
    print_dir(directory, include_hidden, display_details, sort_by_size);

    return 0;
}

void print_dir(char *directory, int include_hidden, int display_details, int sort_by_size)
{
    DIR *dir;
    struct dirent *dirp;
    dir = opendir(directory);
    if (dir == NULL)
    {
        perror("opendir");
        exit(EXIT_FAILURE);
    }
    // Trường hợp 1: cụm -A và -S không được gọi
    if (list_all_except_dot == 0 && sort_by_size == 0)
    {
        while ((dirp = readdir(dir)) != NULL)
        {
            // in file ẩn (có dấu '.' ở đầu)
            if (include_hidden == 0 && dirp->d_name[0] == '.')
            {
                continue;
            }
            // in stat của file
            if (display_details)
            {
                print_long_format(directory, dirp);
            }
            printf(" %s\n", dirp->d_name);
        }
    }
    // Trường hợp 2: cụm -alut và -S không được gọi
    if ((include_hidden == 0 && display_details == 0 && sort_by_access_time == 0 && sort_by_modification_time == 0) && sort_by_size == 0)
    {
        if (list_all_except_dot)
        {
            // TODO
        }
    }
    // Trường hợp 3: cụm -alut và -A không được gọi
    if ((include_hidden == 0 && display_details == 0 && sort_by_access_time == 0 && sort_by_modification_time == 0) && list_all_except_dot == 0)
    {
        if (sort_by_size)
        {
            sort_files_by_size(directory);
        }
    }

    closedir(dir);
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

    int num_files = scandir(directory, &namelist, NULL, alphasort); // dùng vscode báo lỗi thì kệ nó, compile bình thường

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
            sort_by_size = 1;
            break;
        default:
            fprintf(stderr, "Invalid option\n");
            exit(EXIT_FAILURE);
        }
    }
}