#include <cstdlib>
#include <unistd.h>
#include <iostream>
#include <dirent.h>
#include <sys/stat.h>
#include <vector>
#include <pwd.h>
#include <grp.h>
#include <iomanip>
#include <algorithm>
#include <cstring>

void printFileInfo(const char* path, const char* name, const size_t max_links, const size_t max_usr, const size_t max_grp, const size_t max_size) 
{
    std::string full_path = std::string(path) + "/" + name;

    struct stat file_stat;
    if(stat(full_path.c_str(), &file_stat))
        return;

    bool is_link = false;
    char link[PATH_MAX];
    ssize_t len = readlink(full_path.c_str(), link, sizeof(link) - 1);
    // if(S_ISLNK(file_stat.st_mode) && ) {
    //     std::cout << "l";
    //     is_link = true;
    // }
    if(len > 0) {
        std::cout << "l";
        is_link = true;
    }
    else if(S_ISDIR(file_stat.st_mode))
        std::cout << "d";
    else if(S_ISREG(file_stat.st_mode)){
        std::cout << "-";
    }
    else if(S_ISCHR(file_stat.st_mode)){
        std::cout << "c";
    }
    else if(S_ISBLK(file_stat.st_mode)){
        std::cout << "b";
    }
    else if(S_ISFIFO(file_stat.st_mode)){
        std::cout << "p";
    }
    else if(S_ISSOCK(file_stat.st_mode)){
        std::cout << "s";
    }
    else {
        std::cout << "?";
    }

    std::cout << ((file_stat.st_mode & S_IRUSR) ? "r" : "-");
    std::cout << ((file_stat.st_mode & S_IWUSR) ? "w" : "-");
    std::cout << ((file_stat.st_mode & S_IXUSR) ? "x" : "-");
    std::cout << ((file_stat.st_mode & S_IRGRP) ? "r" : "-");
    std::cout << ((file_stat.st_mode & S_IWGRP) ? "w" : "-");
    std::cout << ((file_stat.st_mode & S_IXGRP) ? "x" : "-");
    std::cout << ((file_stat.st_mode & S_IROTH) ? "r" : "-");
    std::cout << ((file_stat.st_mode & S_IWOTH) ? "w" : "-");
    std::cout << ((file_stat.st_mode & S_IXOTH) ? "x" : "-");

    std::cout << " " << std::setw(max_links) << file_stat.st_nlink;

    passwd* pw = getpwuid(file_stat.st_uid);
    std::cout << " "  << std::setw(max_usr) << (pw ? pw->pw_name : std::to_string(file_stat.st_uid));

    group* gr = getgrgid(file_stat.st_gid);
    std::cout << " " << std::setw(max_grp) << (gr ? gr->gr_name : std::to_string(file_stat.st_gid));

    std::cout << " " << std::setw(max_size) << file_stat.st_size;

    char szTime[80];
    tm* time_info = localtime(&file_stat.st_mtime);
    if(difftime(time(NULL), mktime(time_info)) > 31556926) {
        strftime(szTime, sizeof(szTime), "%b %d %Y", time_info);
    }
    else {
        strftime(szTime, sizeof(szTime), "%b %d %H:%M", time_info);
    }
    std::cout << " " << szTime;

    if(is_link) {
        if(len != -1) {
            link[len] = '\0';
            std::cout << " " << name << " -> " << link << std::endl;
        }
        else {
            std::cout << " " << name << " -> [invalid link]" << std::endl;
        }
    }
    else {
        std::cout << " " << name << std::endl;
    }
}

void printDirectoryInfo(const char* path)
{
    dirent* entry;
    blkcnt_t totalBlocks = 1;

    DIR* dir = dir = opendir(path);
    if(!dir) {
        std::cerr << "Error: can't open directory: " << path << std::endl;
        exit(EXIT_FAILURE);
    }

    // max width of symbols for different columns
    size_t max_links = 0, max_usr = 0, max_grp = 0, max_size = 0;
    std::vector<std::string> entries;
    while (entry = readdir(dir)) {
        try{
            std::string full_path = std::string(path) + "/" + entry->d_name;

            struct stat file_stat;
            if(stat(full_path.c_str(), &file_stat))
                continue;

            if(entry->d_name[0] == '.')
                continue;

            size_t temp = std::to_string(file_stat.st_nlink).size();
            if(temp > max_links)
                max_links = temp;

            temp = std::to_string(file_stat.st_size).size();
            if(temp > max_size)
                max_size = temp;

            passwd* pw = getpwuid(file_stat.st_uid);
            temp = pw ? strlen(pw->pw_name) : std::to_string(file_stat.st_uid).size();
            if(temp > max_usr)
                max_usr = temp;

            group* gr = getgrgid(file_stat.st_gid);
            temp = gr ? strlen(gr->gr_name) : std::to_string(file_stat.st_gid).size();
            if(temp > max_grp)
                max_grp = temp;

            entries.push_back(entry->d_name);
            totalBlocks += file_stat.st_blocks;
        } catch( ... ) { continue; }
    }

    totalBlocks = totalBlocks >> 1;
    std::cout << "total " << totalBlocks <<std::endl;

    std::cout << "max_links = " << max_links << "; max_usr = " << max_usr << "; max_grp = " << max_grp << "; max_size = " << max_size << std::endl;

    while (entry = readdir(dir)) {
        try {
            std::string full_path = std::string(path) + "/" + entry->d_name;

            struct stat file_stat;
            if(stat(full_path.c_str(), &file_stat))
                continue;

            totalBlocks += file_stat.st_blocks;
        } catch ( ... ) { continue; }
    }

    std::sort(std::begin(entries), std::end(entries));
    for(const auto& file : entries) {
        printFileInfo(path, file.c_str(), max_links, max_usr, max_grp, max_size);
    }

    closedir(dir);
}

int main(int argc, char *argv[])
{
    int opt;
    bool l_opt = false;
    while((opt = getopt(argc, argv, "l")) != -1) {
        switch(opt)
        {
            case 'l':
                l_opt = true;
                break;
            case '?':
                std::cerr << "Unknown option. Usage: " << argv[0] << " -l [dictionary]" << std::endl;
                exit(EXIT_FAILURE);
            default:
                std::cerr << "Usage: " << argv[0] << " -l [dictionary]" << std::endl;
                exit(EXIT_FAILURE);
        }
    }

    if(!l_opt) {
        std::cerr << "Usage: " << argv[0] << " -l [dictionary]" << std::endl;
        exit(EXIT_FAILURE);
    }

    if(optind >= argc || argv[optind] == NULL) {
        printDirectoryInfo(".");
    }

    for (; optind < argc; optind++) {
        std::cout << argv[optind] << ": " <<std::endl;
        printDirectoryInfo(argv[optind]);
    }

    return EXIT_SUCCESS;
}