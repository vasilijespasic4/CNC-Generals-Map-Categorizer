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

#include "RefPack.h"
#include "File.h"

#define INPUT_MAX 32
#define PATH_MAX 512

struct map_entry
{
	char file_path[PATH_MAX];
	struct map_entry* next;
	char categorized;
};

struct map
{
	struct map_entry entry[8];
};

struct map name_map;

int is_player_start_string(const unsigned char* candidate) {
    if (memcmp(candidate, "Player_", 7) != 0)
        return 0;
	
	int ret = 0;
    
    if (candidate[7] < '1' || candidate[7] > '8')
        return 0;
    else
		ret = candidate[7]-'0';

    if (memcmp(candidate + 8, "_Start", 6) != 0)
        return 0;
    
    if (candidate[8 + 6] != 3)
        return 0;

    return ret;
}

int read_map(const char* path)
{
	FILE* file = fopen(path, "rb");
    if (file == NULL) 
	{
        fprintf(stderr, "ERROR: Cannot open file: %s\n", path);
        return -1;
    }
	
    unsigned char header[8];
    if (fread(header, 1, 8, file) != 8) 
	{
        fprintf(stderr, "ERROR: Cannot read header of 8 bytes on file: %s\n", path); 
		fclose(file); 
		return -1;
    }

	int is_compressed = 1;
    if (memcmp(header, "EAR\0", 4) != 0) 
	{
        is_compressed = 0;
		if(memcmp(header, "CkMp", 4) != 0)
		{
			fprintf(stderr, "ERROR: File \"%s\" doesn't start with EAR and is not readable.\n", path); 
			fclose(file); 
			return -1;
		}
    }
    
	unsigned char* decompressed_buffer;
	uint32_t decompressed_size;
	if(is_compressed)
	{
		decompressed_size = *(uint32_t*)(header + 4);

		if (decompressed_size == 0 || decompressed_size > 50000000) 
		{
			 fprintf(stderr, "ERROR: Unreal decompressed size on file: %s\n", path); 
			 fclose(file); 
			 return -1;
		}

		long compressed_data_start = ftell(file);
		fseek(file, 0, SEEK_END);
		long compressed_size = ftell(file) - compressed_data_start;
		fseek(file, compressed_data_start, SEEK_SET);

		unsigned char* compressed_buffer = (unsigned char*)malloc(compressed_size);
		decompressed_buffer = (unsigned char*)malloc(decompressed_size);
		if (!compressed_buffer || !decompressed_buffer) 
		{ 
			fprintf(stderr, "ERROR: Failed to allocate memory for compressed and decompressed buffers on file: %s\n", path); 
			fclose(file); 
			return -1; 
		}
		
		if (fread(compressed_buffer, 1, compressed_size, file) != compressed_size) 
		{
			fprintf(stderr, "ERROR: Compressed map file wasn't fully read on file: %s\n", path); 
			fclose(file); 
			free(compressed_buffer); 
			free(decompressed_buffer); 
			return -1;
		}
		fclose(file);
		
		int compressed_bytes_read = 0;
		int result_size = REF_decode(decompressed_buffer, compressed_buffer, &compressed_bytes_read, compressed_size);
		free(compressed_buffer);

		if (result_size != decompressed_size) 
		{
			fprintf(stderr, "ERROR: Decompression unsuccessful. Expected %u, got %d bytes.\n", decompressed_size, result_size);
			free(decompressed_buffer);
			return -1;
		}
	}
	else
	{
		long compressed_data_start = ftell(file);
		fseek(file, 0, SEEK_END);
		decompressed_size = ftell(file) - compressed_data_start;
		fseek(file, compressed_data_start, SEEK_SET);
		
		decompressed_buffer = (unsigned char*)malloc(decompressed_size);
		if (fread(decompressed_buffer, 1, decompressed_size, file) != decompressed_size) 
		{
			fprintf(stderr, "ERROR: Decompressed map file wasn't fully read on file: %s\n", path); 
			fclose(file); 
			free(decompressed_buffer); 
			return -1;
		}
		fclose(file);
	}
	
	const unsigned char* search_start = decompressed_buffer;
    const unsigned char* buffer_end = decompressed_buffer + decompressed_size;
	
	int max_num = 0;
	
	while(search_start < buffer_end)
	{
		void* found_p = memchr(search_start, 'P', buffer_end - search_start);
		
		if (found_p == NULL)
            break;
		
		const unsigned char* candidate = (const unsigned char*)found_p;
		
		if (candidate + 15 <= buffer_end) 
		{
			int num;
            if (num = is_player_start_string(candidate)) 
				max_num = max(max_num, num);
        }

        search_start = candidate + 1;
	}
    
    free(decompressed_buffer);
    return max_num;
}

int read_data(const char* path)
{
	char search_path[MAX_PATH];
    snprintf(search_path, MAX_PATH, "%s\\*.*", path);
	
	WIN32_FIND_DATAA find_file_data;
	
    HANDLE hFind = FindFirstFileA(search_path, &find_file_data);
	
	if (hFind == INVALID_HANDLE_VALUE)
        return -1;
	
	int map_found_in_folder = 1;
	
    do {
		if (strcmp(find_file_data.cFileName, ".") == 0 || strcmp(find_file_data.cFileName, "..") == 0) 
		{
            continue;
        }
		
		char full_path[MAX_PATH];
        snprintf(full_path, MAX_PATH, "%s\\%s", path, find_file_data.cFileName);
		
		if (find_file_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) 
		{
            if(read_data(full_path) == 1)
			{
				printf("Folder doesn't contain a map file: %s\n", full_path);
			}
        } 
		else 
		{
            const char *point = strrchr(find_file_data.cFileName, '.');
            if (point != NULL && strcmp(point, ".map") == 0) 
			{
				int players = read_map(full_path);
				
				if(players == -1)
				{
					
				}
				else if(players == 0)
				{
					//printf("Mission: %s\n", full_path);
				} 
				else if(players > 1 && players < 9)
				{
					//printf("%d Player Map: %s\n", players, full_path);
				} 
				else if(players == 1)
				{
					printf("Map with 1 player? What the helly: %s\n", full_path);
				}
				if(players == 0 || (players > 1 && players < 9))
				{
					if(players > 1)
						players--;
					
					struct map_entry* new_entry = (struct map_entry*)malloc(sizeof(struct map_entry));
					
					strcpy(new_entry->file_path,full_path);
					new_entry->next = NULL;
					new_entry->categorized = 0;
					
					struct map_entry* entry = &name_map.entry[players];
					
					while(entry->next)
						entry = entry->next;
					
					entry->next = new_entry;
					
					map_found_in_folder = 2;
				}
            }
        }
    } while (FindNextFileA(hFind, &find_file_data) != 0);

    FindClose(hFind);
	
	return map_found_in_folder;
}

void print_commands()
{
    printf("Commands:\n\th: See commands.\n");
    printf("\tex: Exit program.\n");
    printf("\tind <dirname>: Index all maps from the specified directory (type . in dirname for current folder, .. for back folder).\n");
    printf("\tind -s: Show all indexed maps and info.\n");
	printf("\tcat: Categorize all indexed maps into folders in this directory.\n");
}

int main()
{
	printf("Welcome to Map Reader program by Vindirect\n");
	print_commands();
	
	char input[INPUT_MAX + PATH_MAX];
	
	while (1)
	{
		fgets(input, INPUT_MAX + PATH_MAX, stdin);
		int i;
		for(i = 0; input[i] != '\0'; i++);
		input[i-1] = 0;
		
		if(strcmp(input, "ex") == 0)
			break;
		else if(strcmp(input, "h") == 0)
			print_commands();
		else if(strcmp(input, "cat") == 0)
		{
			char* folder_cat_name = "Missions";
			int count = 0;
			
			for(int i = 0; i < 8; i++)
			{
				if(i>0)
				{
					folder_cat_name = (char*)malloc(15*sizeof(char));
					sprintf(folder_cat_name, "%d Player Maps", i+1);
				}
				struct map_entry* cur_entry = name_map.entry[i].next;
				if(cur_entry)
				{
					MKDIR(folder_cat_name);
					printf("%s:\n", folder_cat_name);
				}
				
				while(cur_entry)
				{
					if(cur_entry->categorized)
					{
						cur_entry = cur_entry->next;
						continue;
					}
					
					count++;
					
					char* file_path = cur_entry->file_path;
					char* containing_folder = strdup(cur_entry->file_path);
					char* ptr_end = NULL;
					char* temp = containing_folder;
					while((temp = strchr(temp, '\\')) != NULL)
					{
						ptr_end = temp;
						temp++;
					}
					
					if (ptr_end != NULL)
						*ptr_end = '\0';
					
					char* just_folder = strdup(containing_folder);
					char* ptr_end_2 = NULL;
					temp = just_folder;
					while((temp = strchr(temp, '\\')) != NULL)
					{
						ptr_end_2 = temp;
						temp++;
					}
					
					char path_of_folder[PATH_MAX];
					strcpy(path_of_folder, folder_cat_name);
					strcat(path_of_folder, "\\");
					if(ptr_end_2 + 1 != NULL)
						strcat(path_of_folder, ptr_end_2+1);
					printf("%s\n", path_of_folder);
					MKDIR(path_of_folder);
					
					copy_folder(containing_folder, path_of_folder);
						//printf("Copy Success: %s\n", ptr_end+1);
					
					free(containing_folder);
					free(just_folder);
					
					cur_entry->categorized = 1;
					cur_entry = cur_entry->next;
				}
			}
			
			printf("Categorization successful. Maps categorized: %d\n", count);
		} 
		else // Commands with args
		{
			char* blank = strchr(input, ' ');
			if(blank != NULL)
				*blank = '\0';
			else
				blank = input + strlen(input)-1;
			
			
			if(strcmp(input, "ind") == 0)
			{
				
				if(strlen(blank+1) < 1)
				{
					printf("Argument not specified.\n");
					continue;
				}
				
				char* token_string = strdup(blank+1);
				char* token = strtok(token_string, " ");
				
				if(token[0] == '-')
				{
					if(strcmp(token+1, "s") == 0)
					{
						char* folder_name = "Missions";
						int count = 0;
						for(int i = 0; i < 8; i++)
						{
							if(i>0)
							{
								folder_name = (char*)malloc(15*sizeof(char));
								sprintf(folder_name, "%d Player Maps", i+1);
							}
							
							struct map_entry* cur_entry = name_map.entry[i].next;
							
							while(cur_entry)
							{
								count++;
								printf("%s:\t%s \n", folder_name, cur_entry->file_path);
								cur_entry = cur_entry->next;
							}
						}
						printf("Maps Indexed: %d\n", count);
					}
					else
					{
						printf("Invalid argument.\n");
					}
				}
				else
				{
					char filename[PATH_MAX];
					strcpy(filename,blank+1);
					printf("Reading data from folder: %s ... Please Wait\n", filename);
					
					if(read_data(filename) == -1)
					{
						printf("Directory not found.\n");
						continue;
					}
					
					printf("Indexing done.\n");
				}
			}
		}
		
	}
		
    return 0;
}