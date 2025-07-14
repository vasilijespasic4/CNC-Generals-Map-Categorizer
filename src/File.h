#pragma once

#ifdef _WIN32
    #include <windows.h>
    #include <direct.h>
    #define MKDIR(path) _mkdir(path)
#else
    #include <dirent.h>
    #include <unistd.h>
    #include <sys/stat.h>
    #define MKDIR(path) mkdir(path, 0777)
#endif

int copy_single_file(const char* src_path, const char* dest_path);
int copy_folder(const char* src_folder, const char* dest_folder);