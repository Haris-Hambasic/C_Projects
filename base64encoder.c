#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>

#include <stdint.h> // typedef uint8_t
// Check that uint8_t type exists
#ifndef UINT8_MAX
#error "No support for uint8_t"
#endif

int main(int argc, char *argv[]) {

    // Base64 alphabet
    static char const alphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                   "abcdefghijklmnopqrstuvwxyz"
                                   "0123456789+/=";

    int input_fd = STDIN_FILENO;
    int output_fd = STDOUT_FILENO;

    // New line tracker
    size_t bytes_read = 0;

    // Input and Output data
    uint8_t in[3], out[4];

    // Amount of bytes read, for current iteration
    size_t nread = 0;
    ssize_t input_content;

    // Argument checking ERROR
    if (argc == 1 || argc > 2) {
        fprintf(stderr, "ERROR: Incorrect number of arguments passed.\n1 argument required but you passed %d arguments.\n\n", argc - 1);
        exit(1);
    }

    // Argument checking NO ERRORS
    if (argc > 0) {
        // Check if user is entering text into console in place of giving path to file
        if (strcmp(argv[1], "-") == 0) {
            // If there is no argument or the argument is "-", then use STDIN_FILENO as your file descriptor number for reading from
            input_content = input_fd;
        }

        // Check if user passed in path to file
        if (strcmp(argv[1], "-") != 0) {
            //write(output_fd, "We're opening the file...\n", 32);

            // Open the file
            input_content = open(argv[1], O_RDONLY, 0600); // remember to close()

            // Check if open() was successful... or not
            if (input_content < 0) {
                perror("ERROR: Could not open input file...\n");
                exit(1);
            } else {
                // Set file pointer to beginning of file
                lseek(input_content, 0, SEEK_SET);
            }
        }
    } else {
        perror("You did not enter any arguments. 1 argument required.");
        exit(1);
    }

    while (1) {
        // Use a for loop to read in 3 bytes of data, at a time
        for (size_t offset = 0; offset < sizeof in;) {
            // Holds number of bytes read from file, after reading that amount from the supplied file
            ssize_t n = read(input_content, offset + (char *) in, sizeof in - offset);

            // Error checking
            if (n < 0) {
                perror("ERROR: There was an error reading the file.");
                exit(1);
            }

            // No additional data was read (not as a result of an error... just no data to read)
            // ***ALSO INDICATES POTENTIALLY PARTIALLY FILLED "in" array
            if (n == 0) break;

            // Set offset to reflect amount of bytes read
            offset += n;

            // Set nread to "remember" number of bytes read.. for external use outside of this for loop scope
            nread = offset / sizeof *in;
        }

        // Check if you've reached the end of the file... and no more data will be read
        if (nread == 0) break;

        // Handle converting to base64 and store each base64 character in appropriate "out" index position
        /*
        ******************************
        */
        out[0] = alphabet[in[0] >> 2];
        out[1] = alphabet[((in[0] & 0x03) << 4) | (in[1] >> 4)];
        out[2] = alphabet[((in[1] & 0x0F) << 2) | (in[2] >> 6)];
        out[3] = alphabet[(in[2] & 0x3F)];

        // Adds padding, if needed by checking if nread is full; if not full then adds padding
        for (size_t i = 3; i > nread; i--) out[i] = alphabet[64];

        // Handle writing data out to user
        for (size_t offset = 0; offset < sizeof out;) {
            // Holds number of bytes written --> tries to write all bytes at once
            ssize_t n = write(output_fd, offset + (char *) out, sizeof out - offset);
            // Error checking
            if (n < 0) {
                perror("ERROR: There was an error writing the data.");
                exit(1);
            }

            // Set how much data was written
            offset += n;
            bytes_read += n;

            // Insert a line break, if needed
            if (bytes_read >= 76) {
                fflush(stdout);
                printf("\n");
                bytes_read = 0;
                fflush(stdout);
            }
        }

        memset(in, '\0', sizeof in);
        memset(out, '\0', sizeof out);

        if (nread < sizeof in / sizeof *in) break;

        nread = 0;
    }

    printf("\n");

    // Close file descriptors
    close(input_content);

    // Successful program exit
    exit(EXIT_SUCCESS);
}
