/*

Author:
    Haris Hambasic

Year:
    2022

Project Description:
    This project is an implementation of a file archiver
    that gives users the ability to 1) archive files they
    specify via command-line arguments or 2) unpack an
    archive file.

Program Invocation Specification:
    1) To archive files, simply compile the program and enter the paths
        of the file you want to archive, as command-line arguments, of
        the program. The last argument supplied must be the file to that
        will act as the "destination", or archive file.

    2) To unpack an archived file, compile the program and invoke it with
        a single command-line argument that is a path to the archive file.
        The program will unpack the archive file and store the unpacked files
        in the directory where the program exists.

*/

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <stdbool.h>
#include <libgen.h>
#include <unistd.h>

int pack(char *path, FILE *output_archive_file) {
    struct stat path_stats;

    // Include immediate check if current file/directory is "other" --> .exe, etc.
    int r = stat(path, &path_stats);
    if (1 == 1) {
        char *bn = basename(path);

        // Print file path or directory size and name
        if (S_ISREG(path_stats.st_mode)) fprintf(output_archive_file, "%zu:%s", strlen(bn), bn);
        else fprintf(output_archive_file, "%zu:%s", strlen(bn)+1, bn);

        if (r == -1) {
            fprintf(stderr, "Path error\n");
            exit(1);
        } else if (S_ISREG(path_stats.st_mode)) {
            // Print file content size
            fprintf(output_archive_file, "%lu:", path_stats.st_size);
            // Print file content
            FILE *current_entry_file = fopen(path, "rb");
            fseek(current_entry_file, 0, SEEK_SET);
            if (current_entry_file == NULL) {
                perror("ERROR: There was an error opening the file.");
                exit(1);
            }
            int next_character;
            while (1) {
                next_character = fgetc(current_entry_file);
                if (feof(current_entry_file)) break;
                fprintf(output_archive_file, "%c", next_character);
            }
        } else if (S_ISDIR(path_stats.st_mode)) {
            fprintf(output_archive_file, "/");

            DIR *opened_directory;

            if ((opened_directory = opendir(path)) == NULL) {
                //printf("\nAttempting to open: %s", path);
                perror("Cannot open directory -->");
                exit(1);
            }

            struct dirent *directory_entry;
            struct stat directory_entry_stats;

            while ((directory_entry = readdir(opened_directory)) != NULL) {
                if (strcmp(directory_entry->d_name, ".") == 0 || strcmp(directory_entry->d_name, "..") == 0) {
                    ;
                } else {
                    stat(directory_entry->d_name, &directory_entry_stats);
                    if (S_ISREG(directory_entry_stats.st_mode)) {
                        pack(directory_entry->d_name, output_archive_file);
                    } else {
                        char new_working_directory[256];
                        chdir(directory_entry->d_name);
                        getcwd(new_working_directory, sizeof new_working_directory);
                        pack(new_working_directory, output_archive_file);
                        fprintf(output_archive_file, "\n0:");
                        chdir("..");
                    }
                }
            }
        }
    }
    return 0;
}

int unpack(char *path) {
    FILE *archive_file = fopen(path, "rb");
    int next_character;

    while (1) {
        if (feof(archive_file)) break;

        // Get entry name size
        char entry_name_size[256];
        memset(entry_name_size, '\0', sizeof entry_name_size);

        char *string_to_num_ptr;
        size_t entry_name_length = 0;
        while (1) {
            if (feof(archive_file)) break;
            next_character = fgetc(archive_file);
            if (next_character == ':') {
                entry_name_length = strtol(entry_name_size, &string_to_num_ptr, 10);
                break;
            } else {
                entry_name_size[strlen(entry_name_size)] = next_character;
            }
        }
        // Check if need to exit current directory
        if (entry_name_length == 0) {
            chdir("..");
        } else {
            // Get entry name
            char entry_name[entry_name_length];
            memset(entry_name, '\0', sizeof entry_name_size);
            for (size_t offset=0; offset<entry_name_length;) {
                next_character = fgetc(archive_file);
                entry_name[offset] = next_character;
                offset += 1;
            }
            //printf("Entry Name: %s\n", entry_name);

            /* CHECK IF ENTRY IS DIRECTORY OR FILE */
            if (entry_name[entry_name_length-1] == '/') {
                char new_dir[256] = "./";
                strcat(new_dir, entry_name);
                mkdir(entry_name);
                chdir(new_dir);
                memset(entry_name, '\0', sizeof entry_name_size);
            } else {
                // Get size of content
                char entry_content_size[256];
                memset(entry_content_size, '\0', sizeof entry_content_size);
                size_t entry_content_length = 0;
                while (1) {
                    if (feof(archive_file)) break;
                    next_character = fgetc(archive_file);
                    if (next_character == ':') {
                        entry_content_length = strtol(entry_content_size, &string_to_num_ptr, 10);
                        break;
                    } else {
                        entry_content_size[strlen(entry_content_size)] = next_character;
                    }
                }
                //printf("Content length: %lli\n", entry_content_length);

                FILE *output_file = fopen(entry_name, "wb");
                    // Get content
                    //char entry_content[entry_content_length];
                    if (entry_content_length != 0) {
                        for (size_t offset=0; offset<entry_content_length;) {
                            next_character = fgetc(archive_file);
                            //entry_content[offset] = next_character;
                            fputc(next_character, output_file);
                            offset += 1;
                        }
                        fclose(output_file);
                    };
                //printf("Entry content: %s\n\n", entry_content);
            }
        }
    }
    fclose(archive_file);

    return 0;
}

int main(int argc, char *argv[]) {
    if (argc == 1) {
        fprintf(stderr, "ERROR: You passed no arguments. At least 2 arguments required.");
        exit(1);
    }

    if (argc == 2) {
        unpack(argv[1]);
        return 0;
    }

    if (argc > 2) {
        for (int file_index=1; file_index<argc-1;) {
            char *filepath = argv[file_index];
            struct stat file_stats;
            int r = stat(filepath, &file_stats);
            if (r == -1) {
                fprintf(stderr, "File error\n");
                exit(1);
            } else if (S_ISREG(file_stats.st_mode)) {
                FILE *file_to_print_to = fopen(argv[argc-1], "ab");
                pack(filepath, file_to_print_to);
                fprintf(file_to_print_to, "0:");
                fclose(file_to_print_to);
            } else {
                char current_dir[256];
                chdir(argv[file_index]);
                getcwd(current_dir, sizeof current_dir);
                FILE *file_to_print_to = fopen(argv[argc-1], "ab");
                pack(filepath, file_to_print_to);
                fprintf(file_to_print_to, "0:");
                fclose(file_to_print_to);
            }

            file_index += 1;
        }
        /*
        FILE *file_to_print_to = fopen(argv[argc-1], "ab");
        fprintf(file_to_print_to, "0:");
        fclose(file_to_print_to);
        */
        return 0;
    }
}
