#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/fcntl.h>

#define MAXBUF (8192)
// Predefined block size must each be 256 
#define BLOCKSIZE 256
// Predefined name of schema file 
#define SCHEMA_FILE "schema_file"

FILE *f;


// Headers
char* call_query(char *query);
int add_to_schema_file(const char *table_info);
int add_data(const char *tablename, const char *data);

int main(int argc, char *argv[]) {
    f = fopen("output.log", "a+");  
    if (f == NULL) {
        perror("Error with logging file");
    }

    char *query;
    char content[MAXBUF];
    int content_length = 0; // Track length 

    // Get the query 
    query = getenv("QUERY_STRING");
    if (query == NULL) {
        query = "No query received.";
    }

    // Fix format 
    if (strncmp(query, "query=", 6) == 0) {
        query += 6;
    }

    // Make the response body 
    content_length += snprintf(content + content_length, MAXBUF - content_length, "<p>SQL CGI Program</p>\r\n");
    content_length += snprintf(content + content_length, MAXBUF - content_length, "<p>Received SQL Command:</p>\r\n");
    content_length += snprintf(content + content_length, MAXBUF - content_length, "<p>%s</p>\r\n", query);

    //print out if call_query works
    char *query_response = call_query(query);
    content_length += snprintf(content + content_length, MAXBUF - content_length, "<p>%s</p>\r\n", query_response);

    // Generate the HTTP response 
    printf("Content-Length: %d\r\n", content_length);
    printf("Content-Type: text/html\r\n\r\n");
    printf("%s", content);
    //fflush(stdout);

    exit(0);
}

/**
 * Checks what query it is and calls the specific function for that query 
 */
char* call_query(char *statement) {
    static char response[MAXBUF]; 
    char line[MAXBUF];
    char query[MAXBUF];
    int j = 0; // query index

   //20% = space
    for (int i = 0; statement[i] != '\0'; i++) {
        if (statement[i] == '%' && statement[i+1] == '2' && statement[i+2] == '0') {
            query[j++] = ' ';
            i += 2; // skip over 20
        } else {
            query[j++] = statement[i];
        }
    }
    query[j] ='\0';


    strncpy(line, query, MAXBUF);
    line[MAXBUF - 1] = '\0'; 

    char *token = strtok(line, " ");
    if (token == NULL) {
        snprintf(response, sizeof(response), "Empty query.");
        return response;
    }

    // Skips cmd word & grabs rest of line
    char *rest = query + strlen(token);
    while (*rest == ' ') rest++; // skip extra spaces

    // Check what query
    if (strcmp(token, "CREATE") == 0) {
        //individual parse for CREATE -- pass into create_table function 
        // have create table return 0 if successful -1 if not and then throw/catch error
        //create_table(rest)
        add_to_schema_file("Hello|id:int,name:char(30),crystal:leah;");
        snprintf(response, sizeof(response), "CREATE RUNS: %s", rest);

    } else if (strcmp(token, "INSERT") == 0) {
        add_data("Hello","0003                              somenames00000004");
        //insert_into(rest)
        snprintf(response, sizeof(response), "INSERT RUNS: %s", rest);

    } else if (strcmp(token, "UPDATE") == 0) {

        //update_table(rest)
        snprintf(response, sizeof(response), "UPDATE RUNS: %s", rest);

    } else if (strcmp(token, "SELECT") == 0) {

        //select_from(rest)
        snprintf(response, sizeof(response), "SELECT RUNS: %s", rest);

    } else if (strcmp(token, "DELETE") == 0) {

        //delete_table(rest)
        snprintf(response, sizeof(response), "DELETE RUNS: %s", rest);

    } else {
        snprintf(response, sizeof(response), "Invalid query: %s", token);
    }

    return response;
}

/**
 * Create_table function adds the new table to the schema file and creates a new file under the name of the table
 * for the data to be entered into 
 * 
 * 
*/ 
int add_to_schema_file(const char *table_info) {

    //open schema file if exists - if not create schema file r/w allowed
    int fd = open(SCHEMA_FILE, O_RDWR | O_CREAT, 0644);  
    // if failure return -1 
    if (fd == -1) {
        perror("A problem occured trying to open schema file");
        return -1;
    }

    off_t filesize = lseek(fd, 0, SEEK_END);
    if (filesize == -1) {
        perror("A problem occured with lseek ");
        close(fd);
        return -1;
    }

    if (filesize == 0 ) {
        //Initialize the first 2 blocks one time if the file is empty 
        char buffer[BLOCKSIZE*2];
        memset(buffer, '.', BLOCKSIZE*2);

        //Add indexes for the first 2 blocks
        memcpy(&buffer[0], table_info, strlen(table_info)); 
        memcpy(&buffer[252], "0001", 4);
        memcpy(&buffer[508], "XXXX", 4); 

        //Write back to the file 
        ssize_t size_written = write(fd, buffer, BLOCKSIZE*2);
        if(size_written != BLOCKSIZE*2) {
            perror("A problem occured writing buffer to file");
            close(fd);
            return -1; 
        }

        fprintf(f, "Filesize is 0, so initiated file with %d characters\n", BLOCKSIZE*2); 
    } else {
        fprintf(f, "File already exists and is larger than 0 \n"); 
        
        //Go to the end of the file and go back 256 B to find the last 
        //in use block 
        int file_offset = filesize - BLOCKSIZE*2;
        lseek(fd, file_offset, SEEK_SET); 
        
        //Read the last incomplete block into the memory buffer for parsing
        char buffer[BLOCKSIZE];
        //Or you could do a regular read after lseek to the file_offset 
        ssize_t b_read = pread(fd, buffer, BLOCKSIZE, file_offset);
        if (b_read == -1) {
            perror("A problem occured reading the file to the buffer");
            close(fd);
            return -1;
        }
        //Record the last 4 bytes of the file as the index 
        char index[5]; //Stores 4 + \0
        strncpy(index, buffer + BLOCKSIZE - 4, 4);
        index[4] = '\0';
        fprintf(f, "Index: %s \n", index);

        //Move from end of buffer to last character that is not '.' and count each byte
        int filled = 252;
        int i = 251;

        while ( i >= 0 ) {
            if (buffer[i] != '.') {
                break;
            }
            i--;
            filled --;
        }
        int space_left = 252 - filled;
         //Check to make sure the new string fits in the remaining space (the count)
        if ( strlen(table_info) > space_left ) {
            // doesn't fit 
            //If it doesnt, find end of file again and read the last block into memory
            
            file_offset = filesize - BLOCKSIZE;
            lseek(fd, file_offset, SEEK_SET); 
            b_read = pread(fd, buffer, BLOCKSIZE, file_offset);
            if (b_read == -1) {
                perror("A problem occured reading the file to the buffer");
                close(fd);
                return -1;
            }

            //Edit the last 4 bytes to reference the next block (index +1)
            int number = atoi(index);
            number += 1; 
            sprintf(index, "%04d", number);
            memcpy(&buffer[BLOCKSIZE-4], index, 4); 

            //Add the string to the beginning of this block 
            //Copy block back into file 
            memcpy(&buffer[0], table_info, strlen(table_info)); 
            ssize_t size_written = write(fd, buffer, BLOCKSIZE);
            if(size_written != BLOCKSIZE) {
                perror("A problem occured writing buffer to file");
                close(fd);
                return -1; 
            }

            //Initialize a new 256 B block in the file with the last 4 B as XXXX
            lseek(fd, 0, SEEK_END);
            memset(buffer, '.', BLOCKSIZE); 
            memcpy(&buffer[252], "XXXX", 4);
            size_written = write(fd, buffer, BLOCKSIZE);
            if(size_written != BLOCKSIZE) {
                perror("A problem occured writing buffer to file");
                close(fd);
                return -1; 
            }
            


        } else {
            // fits 
            //If it does, write the string in the first available space
            memcpy(&buffer[filled], table_info, strlen(table_info)); 
            //Overwrite the new block into the file 
            ssize_t size_written = write(fd, buffer, BLOCKSIZE);
            if(size_written != BLOCKSIZE) {
                perror("A problem occured writing buffer to file");
                close(fd);
                return -1; 
            }


        }



    }

     
    //Create a new file for the table data to be stored under
    int h = 0; 
    char table_name[100]; // 100 is max length of a table name 
    while (h <= 100 ) { 
        if (table_info[h] != '|') {
            table_name[h] = table_info[h];
            h++;
        } else {
            break;
        }
    }
    table_name[h] = '\0';
    fprintf(f,"Creating file for %s", table_name);
    int new_table_fd = open(table_name, O_RDWR | O_CREAT, 0644);  
    if (new_table_fd == -1) {
        perror("A problem occured trying to create table file");
        return -1;
    }
    //lseek(new_table_fd, 0, SEEK_SET);  //should be initialized here
    //Initialie one block in file 
    char first_block[BLOCKSIZE]; 
    memset(first_block, '.' ,BLOCKSIZE); 
    memcpy(&first_block[252], "XXXX", 4);
    ssize_t size_written = write(new_table_fd, first_block, BLOCKSIZE);
    if(size_written != BLOCKSIZE) {
        perror("A problem occured writing buffer to file");
        close(new_table_fd);
        return -1; 
    }

    close(new_table_fd);   
    
    close(fd);
    return 0;
}


/**
 * Function to add data to a datablock for a particular file, given the string of data to enter,  the name of the table to enter in
 * (basically an insert) 
 */

 int add_data(const char* tablename, const char* data) {
    fprintf(f, "adding data to a block\n");
    //Find the correct file based on the file name
    int fd = open(tablename, O_RDWR | O_CREAT, 0644);  
    //Add error catching for opening the first time !
    if (fd == -1) {
        perror("A problem occured trying to open the table's data file ");
        return -1;
    }
    //Lseek to end of file - BLOCKSIZE (goes to the beginning of the last block)
    ssize_t offset = lseek(fd, BLOCKSIZE*-1, SEEK_END);
    //Load block into buffer 
    char buffer[BLOCKSIZE];
    ssize_t b_read = pread(fd, buffer, BLOCKSIZE, offset);
            if (b_read == -1) {
                perror("A problem occured reading the data file to the buffer");
                close(fd);
                return -1;
            }
    //Read the index from the last 4 bytes and store it for incrementing 
    char index[5]; //Stores 4 + \0
    strncpy(index, buffer + BLOCKSIZE - 4, 4);
    index[4] = '\0';

    int size = strlen(data); 
    fprintf(f, "Size of the string: %d\n", size);
    int chunks = (BLOCKSIZE-4) / size; 
    fprintf(f, "Number of chunks: %d\n", chunks);
    int addBlock = 0;
    //Find first available space by using length of data string to look @ char at index of length * i where i < BLOCKSIZE/size of string until blank found 
    //Begin from start, then when space is found paste new data into buffer and write buffer to file 
    for (int i = 0; i < chunks; i++) {
        if (buffer[size*i] == '.') { //should be the byte of each start
            // empty space -- put data here
            memcpy(&buffer[size*i], data, strlen(data));
            if ( i == (chunks - 1) ) { // if this is the last chunk of space 
                // add a new block and change the index 
                fprintf(f, "Adding new block: %d\n", i);

                addBlock = 1;
                int number = atoi(index);
                number += 1; 
                sprintf(index, "%04d", number);
                memcpy(&buffer[252], index, 4);
            }
            break;

        } else { 
            // this is taken space 
            continue;
        }
    }

    ssize_t size_written = write(fd, buffer, BLOCKSIZE);
    if(size_written != BLOCKSIZE) {
        perror("A problem occured writing data buffer to data file");
        close(fd);
        return -1; 
    }
    
    if(addBlock) {
        fprintf(f, "Adding a new block because last was filled\n");
        //Initialize a new 256 B block in the file with the last 4 B as XXXX
        lseek(fd, 0, SEEK_END);
        memset(buffer, '.', BLOCKSIZE); 
        memcpy(&buffer[252], "XXXX", 4);
        size_written = write(fd, buffer, BLOCKSIZE);
        if(size_written != BLOCKSIZE) {
            perror("A problem occured writing new blank buffer to data file");
            close(fd);
            return -1; 
        }
    }

    close(fd);
    return 0; 
 }

 /**
  * selects columns by seequentially loading each block 
  * looks for columns that are specified by the where condition if one is provided 
  */
 char* select_col(char* tablename, char *columns[], char* where) {
    if (where != NULL) {
        //There is a where phrase
        //parse the where to get the column and the condition 

    } else {
        //No where clause 
        //Check every block and add the column to string to return 

        char values[1024];  //string to return from tables 
        values[0] = '\0';


        //Open the table file 
        int fd = open(tablename, O_RDWR | O_CREAT, 0644);  
        //Add error catching for opening the first time !
        if (fd == -1) {
            perror("A problem occured trying to open the table's data file for select ");
            return -1;
        }

        //Lseek to start
        ssize_t offset = lseek(fd, 0, SEEK_SET);
        //Load block into buffer 
        char buffer[BLOCKSIZE];
        ssize_t b_read = pread(fd, buffer, BLOCKSIZE, offset);
        if (b_read == -1) {
            perror("A problem occured reading the data file to the buffer");
            close(fd);
            return -1;
        } 




    }

 }