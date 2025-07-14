/*
**	Command & Conquer Generals/Zero Hour Map Categorizer
**	Copyright 2025 Vasilije Spasic <vasa.spasic@gmail.com>
**
**	This program is free software: you can redistribute it and/or modify
**	it under the terms of the GNU General Public License as published by
**	the Free Software Foundation, either version 3 of the License, or
**	(at your option) any later version.
**
**	This program is distributed in the hope that it will be useful,
**	but WITHOUT ANY WARRANTY; without even the implied warranty of
**	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**	GNU General Public License for more details.
**
**	You should have received a copy of the GNU General Public License
**	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "File.h"

int copy_single_file(const char* src_path, const char* dest_path) {
    FILE *src_file, *dest_file;
    char buffer[4096];
    size_t bytes_read;

    src_file = fopen(src_path, "rb");
    if (src_file == NULL) {
		printf("The error file source: %s\n", src_path);
        perror("Error opening source file");
        return 0;
    }

    dest_file = fopen(dest_path, "wb");
    if (dest_file == NULL) {
		printf("The error file dest: %s\n", dest_path);
        perror("Error opening destination file");
        fclose(src_file);
        return 0;
    }

    while ((bytes_read = fread(buffer, 1, sizeof(buffer), src_file)) > 0) {
        fwrite(buffer, 1, bytes_read, dest_file);
    }

    fclose(src_file);
    fclose(dest_file);

    //printf("Copied file: %s -> %s\n", src_path, dest_path);
    return 1;
}

int copy_folder(const char* src_folder, const char* dest_folder) {
#ifdef _WIN32
    char search_path[2048];
    snprintf(search_path, sizeof(search_path), "%s\\*.*", src_folder);

    WIN32_FIND_DATA find_file_data;
    HANDLE hFind = FindFirstFile(search_path, &find_file_data);

    if (hFind == INVALID_HANDLE_VALUE) {
        fprintf(stderr, "Error: Could not find first file in %s\n", src_folder);
        return 0;
    }

    do {
        if (strcmp(find_file_data.cFileName, ".") == 0 || strcmp(find_file_data.cFileName, "..") == 0) {
            continue;
        }

        char src_path[2048];
        char dest_path[2048];
        snprintf(src_path, sizeof(src_path), "%s\\%s", src_folder, find_file_data.cFileName);
        snprintf(dest_path, sizeof(dest_path), "%s\\%s", dest_folder, find_file_data.cFileName);

        if (find_file_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
			MKDIR(dest_path);
            copy_folder(src_path, dest_path);
        } else {
            copy_single_file(src_path, dest_path);
        }
    } while (FindNextFile(hFind, &find_file_data) != 0);

    FindClose(hFind);

#else
    DIR *d = opendir(src_folder);
    if (d == NULL) {
        fprintf(stderr, "Error: Cannot open directory %s\n", src_folder);
        return 0;
    }

    struct dirent *dir_entry;
    while ((dir_entry = readdir(d)) != NULL) {
        if (strcmp(dir_entry->d_name, ".") == 0 || strcmp(dir_entry->d_name, "..") == 0) {
            continue;
        }

        char src_path[1024];
        char dest_path[1024];
        snprintf(src_path, sizeof(src_path), "%s/%s", src_folder, dir_entry->d_name);
        snprintf(dest_path, sizeof(dest_path), "%s/%s", dest_folder, dir_entry->d_name);

        struct stat path_stat;
        stat(src_path, &path_stat);

        if (S_ISDIR(path_stat.st_mode)) {
            copy_folder(src_path, dest_path);
        } else {
            copy_single_file(src_path, dest_path);
        }
    }
    closedir(d);
#endif
    return 1;
}