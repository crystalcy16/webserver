#include "helper.h"

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
    if (strncmp(token, "CREATE", 6) == 0) {
        //Test: CREATE TABLE employees (employee_id INT,first_name VARCHAR(50),last_name VARCHAR(50);

        //individual parse for CREATE -- pass into create_table function 
        // have create table return 0 if successful -1 if not and then throw/catch error
        fprintf(f, "Query: %s \n", query);
        char *input = parse_create(query);
        fprintf(f, "parsed: %s \n", input);

        //For testing 
        //char* sql = "CREATE TABLE employee1 (employee_id INT PRIMARY KEY, first_name VARCHAR(50), last_name VARCHAR(60)";
        //char* parsed = parse_create(sql);
        //fprintf(f, "%s\n", parsed);

        //This was for testing 
        //add_to_schema_file("Hello|id:int,name:char(30),crystal:leah;");

        //integration of parsing and sql command 
        if ( add_to_schema_file(input) == -1 ) {
            fprintf(f, "ERROR adding input to schema file\n");
            perror("Error adding input to schema file\n");
        }
        snprintf(response, sizeof(response), "CREATE RUNS: %s", rest);

    } else if (strncmp(token, "INSERT", 6) == 0) {
        //Testing 
        //add_data("Hello","0003                              somenames00000004");
        //Test: INSERT%20INTO%20employees%20VALUES%20(3,%27Charlie%27,%27bob%27);

        //Parse the insert query to extract tablename 
        char* tablename = extract_table_name(query);
        char* t_scheme = table_schema(tablename);
        if (t_scheme == NULL) {
            snprintf(response, sizeof(response), "<p>Error: Table '%s' does not exist.</p>", tablename);
            return response;
        }
        fprintf(f, "Tablename from query: %s \n", tablename);
        //Parse the insert query to extract values 
        char *values = extract_values_string(query);
        replace_encoded_quotes(values);
        remove_spaces(values);
        fprintf(f, "Values list from query: %s \n", values);
        //TESTING
        //char* test = "INSERT INTO employees VALUES (3,'Charlie',25)";
        //test = extract_values_string(test);
        //fprintf(f, "Values test list: %s \n", test);

        //pass tablename into get schema to return schema 
        char *schema = table_schema(tablename);
        remove_spaces(schema);
        fprintf(f, "Schema from query %s \n", schema);
        //pass schema and values into formatting function 
        char* schema_copy = strdup(schema);
        char* values_copy = strdup(values);
        char* data_string_input = data_string(schema_copy, values_copy);

        //TESTING 
        //char* data_test = data_string("employees|employee_id:smallint,first_name:char(50),last_name:char(50);", "3,Charlie,Bob");
        //char* data_test = data_string("movies|id:smallint,title:char(30),length:int;", "2,'Hello',100");
        fprintf(f, "Data string: %s \n", data_string_input);
        //fprintf(f, "test data string %s \n", data_test);
        //pass formatted string into insert function 
        if (add_data(tablename, data_string_input) == -1) {
            perror("Error adding data string to data file");
        }
        
        snprintf(response, sizeof(response), "INSERT RUNS: %s", rest);

    } else if (strncmp(token, "UPDATE", 6) == 0) {
        //testing UPDATE employees SET employee_id = 3, first_name = 'John', last_name = 'Smith' WHERE employee_id = 3;
        fprintf(f, "Running update\n");
        //get the tablename from the query
        char* tablename = extract_table_name(query);
        fprintf(f, "Tablename: %s\n", tablename);
        
        //get the where 
        char** conditional = select_conditional(query);
        Pair2 cond = {"0", "0"};
        if (conditional != NULL) {
            cond.name = conditional[0];
            cond.value = conditional[2];
            replace_encoded_quotes(cond.value);
            remove_surrounding_quotes(cond.value);
            fprintf(f, "Where condition1: %s\n", conditional[0]);
            fprintf(f, "Where condition2: %s\n", conditional[2]);
        } 

        //get the data string 
        char* values = extract_update_values(query);
        replace_encoded_quotes(values);
        fprintf(f, "values: %s\n", values);

        //if extracted values returns error
        if (strstr(values, "ERROR") != NULL) {
            snprintf(response, sizeof(response), "<p>Error: Bad UPDATE syntax.</p>");
            return response;
        }



        //get the table schema 
        char* schema = table_schema(tablename);
        if (schema == NULL) {
            //if schema doesn't exist 
            snprintf(response, sizeof(response), "<p>Error: Table '%s' does not exist.</p>", tablename);
            return response;
        }
        remove_spaces(schema);




        
        fprintf(f, "Schema : %s\n", schema);
        char* data_string_input = data_string(schema, values);
        fprintf(f, "data string: %s\n", data_string_input);
        select_col(tablename, NULL, cond, data_string_input, 0);

        snprintf(response, sizeof(response), "UPDATE RUNS: %s", rest);

    } else if (strncmp(token, "SELECT", 6) == 0) {
        //Test SELECT employee_id, first_name FROM employees WHERE employee_id = 3;
        //Test SELECT employee_id, first_name FROM employees;
        fprintf(f,"SELECT happening");

        //Gets the array of column names 
        char** col_names = select_colnames(query);
        fprintf(f, "Column names : %s \n", col_names[0]);


        //For testing purposes 
        //char* example[] =  {"employee_id", "first_name"};
        //Pair2 pair = {.name = "employee_id", .value = "0003"};

        //get tablename from querey 
        char* tablename = extract_table_name(query);
        char* t_scheme = table_schema(tablename);
        if (t_scheme == NULL) {
            snprintf(response, sizeof(response), "<p>Error: Table '%s' does not exist.</p>", tablename);
            return response;
        }

        fprintf(f, "Tablename: %s\n", tablename);


        if (col_names[0] && strcmp(col_names[0], "*") == 0) {
            // case of select all 
            char* t_scheme = table_schema(tablename);
            remove_spaces(t_scheme);
            get_all_column_names_from_schema(t_scheme);
        }

        //get the where conditional 
        char** conditional = select_conditional(query);
        if (conditional == NULL) {
            snprintf(response, sizeof(response), "<p>Error: Missing or malformed WHERE clause.</p>");
            return response;
        }

        Pair2 cond = {"0", "0"};
        if (conditional != NULL) {
            cond.name = conditional[0];
            cond.value = conditional[2];
            replace_encoded_quotes(cond.value);
            remove_surrounding_quotes(cond.value);
            fprintf(f, "Where condition1: %s\n", conditional[0]);
            fprintf(f, "Where condition2: %s\n", conditional[2]);
        } 
        
        //Call the select 
        char* result = select_col(tablename, col_names, cond, NULL, 0);

        int result_print = snprintf(response, sizeof(response), "<p> RESULT: </p> %s \r\n", result);
        snprintf(response + result_print, sizeof(response) - result_print , " <p> SELECT RUNS: %s </p>", rest);

    } else if (strncmp(token, "DELETE", 6) == 0) {
        //testing DELETE FROM employees WHERE first_name = 'Charlie';

        fprintf(f, "Running delete\n");
        //get the tablename from the query
        char* tablename = extract_table_name(query);
        fprintf(f, "Tablename: %s\n", tablename);
        
        //get the where 
        char** conditional = select_conditional(query);
        Pair2 cond = {"0", "0"};
        if (conditional != NULL) {
            cond.name = conditional[0];
            cond.value = conditional[2];
            replace_encoded_quotes(cond.value);
            remove_surrounding_quotes(cond.value);
            fprintf(f, "Where condition1: %s\n", conditional[0]);
            fprintf(f, "Where condition2: %s\n", conditional[2]);
        } 

        //get the table schema 
        char* schema = table_schema(tablename);
        if (schema == NULL) {
            //if schema doesn't exist 
            snprintf(response, sizeof(response), "<p>Error: Table '%s' does not exist.</p>", tablename);
            return response;
        }
        remove_spaces(schema);
        fprintf(f, "Schema : %s\n", schema);

        //Deletes all data row if the where condition is true 
        select_col(tablename, NULL, cond, NULL, 1);

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
  * selects columns by sequentially loading each block 
  * looks for columns that are specified by the where condition if one is provided 
  */
 char* select_col(char* tablename, char *columns[], Pair2 where, char *data, int isDelete) {
    int skipwhere = 0;
    if (strcmp( where.name, "0") == 0 && strcmp(where.value, "0") == 0) {
        skipwhere = 1;
    }

    //Open the table file 
    fprintf(f,"starting select\n");
    int fd = open(tablename, O_RDWR | O_CREAT, 0644);  
    //Add error catching for opening the first time !
    if (fd == -1) {
        perror("A problem occured trying to open the table's data file for select ");
    }


    char *values = malloc(4000);  //string to return from tables 
    int cnt_values = 0; 

    if (0) {

    } else {
        
        //Check every block and add the column to string to return 

        char buffer[BLOCKSIZE]; 
        size_t bread; 
        int offset = 0; 

        // change to be the length of an entry based on the table name 
        char* t_scheme = table_schema(tablename);
        int chunk_size = get_entry_size(t_scheme);
        int chunks = 252/chunk_size;
        int isTwice = 0;

        int num_cols = 0;
        if (columns != NULL) {
            num_cols = get_size(columns);
        }


        // fix this so that it calls a function that returns a list of offsets for each column wanted based 
        // on the schema table and the size of the column 
        //Pair col_offsets[] = { {0,4}, {4,39} };
        Pair col_offsets[num_cols];  // same size as columns[]

        for (int i = 0; i < num_cols; i++) {
            col_offsets[i] = get_column_offset_and_size(t_scheme, columns[i]);
            fprintf(f, "column : %s \n", columns[i]);
            fprintf(f, "column offset : %d \n", col_offsets[i].offset);
            fprintf(f, "column size : %d \n", col_offsets[i].size);

        }

        // fix it so that it calls a function that returns the offset and size of the where column 
        // pass in where pair 
        Pair where_col = get_column_offset_and_size(t_scheme, where.name); 

        fprintf(f, "num columns %d",  num_cols);
        
        // for each block in the file 
        while ((bread = pread(fd, buffer, BLOCKSIZE, offset)) > 0) {
            fprintf(f, "Reading block for select\n");
            // for each chunk in the file 
            for (int j = 0; j < chunks; j++ ) {
                // for each column 
                fprintf(f, "Reading chunk %d for select\n", j);
                //create temp to hold the column value for testing 
              
                char temp[where_col.size + 1]; 
                // copy from buffer to the temp 
                if (!skipwhere) {
                    memcpy(temp, buffer + (j*chunk_size + where_col.offset ), where_col.size);
                    temp[where_col.size] = '\0';  // null 
                    fprintf(f, "Where value: %s", where.value);
                    fprintf(f, "temp value: %s", normalize_value(temp));
                }

                if (strcmp(normalize_value(where.value), normalize_value(temp) ) == 0  || skipwhere ) {
                    fprintf(f,"where conditional is true\n");
                    // For each column that is selected 
                    for (int i = 0; i < num_cols; i ++) {
                        fprintf(f, "Reading column for select\n");
                        // copy that column from the buffer over to the values 
                        if ( buffer [j*chunk_size + col_offsets[i].offset] == '.' && isTwice == 1 ) { 
                            goto outside;
                        } else if ( buffer [j*chunk_size + col_offsets[i].offset] == '.') {
                            goto skip;
                            isTwice = 1; 
                        }

                        memcpy(values + cnt_values, buffer + (j*chunk_size + col_offsets[i].offset), col_offsets[i].size);
                        // add a new line for formatting 
                        // increment the last updated spot for values 
                        fprintf(f, "cnt_values :  %d \n", cnt_values);

                        cnt_values = cnt_values + col_offsets[i].size; 

                        if ( (i+1) != num_cols ) {
                            values[cnt_values] = ',';
                            cnt_values++;
                        }
                        skip:
                        fprintf(f, "cnt_values :  %d \n", cnt_values);
                        fprintf(f, "values : %s\n", values);
                    
                    }
                    if (data) {
                        // replace the element in the buffer with the new data if the where is true and there is data 
                        fprintf(f, "Replacing data for chunk %d\n", j);
                        memcpy(buffer + (j*chunk_size) , data, strlen(data));
                        fprintf(f, "Buffer: %s\n", buffer);
                    } else if (isDelete == 1) {
                        // delete the specified entry in the table 
                        memset(buffer + (j*chunk_size), '.', chunk_size);
                    }

                }
                values[cnt_values] = '\n';
                cnt_values ++;
            } 

            // write edits back to file if there were edits (if it is update or delete )
            if (data || isDelete == 1) {
                fprintf(f, "Writing data badk to file");
                ssize_t size_written = write(fd, buffer, BLOCKSIZE);
                if(size_written != BLOCKSIZE) {
                    perror("A problem occured writing data buffer to data file");
                    close(fd);
                }
            }   

            offset += bread; // increment the offset by the block size (how many bytes were actually read though)

        }

        
    }
    outside: 
    fprintf(f, "values : %s", values);

    return values; 
 }

 
 //find word ie: substring matcher
 char* strfind(const char* in, const char* val) {
    size_t len = strlen(val);
    while (*in) {
        if (strncasecmp(in, val, len) == 0)
            return (char*)in;
        in++;
    }
    return NULL;
}
 
 //table name as char*
 char* select_tablename(char* cmd) {
     static char name[128];
     name[0] = '\0';
     
     char *from = strfind(cmd, "FROM");
     if (!from){
         return NULL;
 
     }
     from += 4; // Move past "FROM"
 
     //skips from
     while (isspace(*from)){
         from++;
     }
 
     //until space or ;
     int i = 0;
     while (*from && !isspace(*from) && *from != ';' && i < sizeof(name) - 1) {
         name[i++] = *from++;
     }
 
     name[i] = '\0';
     return name;
 }
 
 //grabs col name from cmd and makes a list
 char** select_colnames(char *cmd) {
    char **col = malloc((MAXCOLS + 1) * sizeof(char*));
    if (!col) return NULL;

    for (int i = 0; i <= MAXCOLS; i++) col[i] = NULL;

    char *select_start = strfind(cmd, "SELECT");
    char *from_start = strfind(cmd, "FROM");
    if (!select_start || !from_start || from_start <= select_start) return NULL;

    select_start += 6; 

    int len = from_start - select_start;
    if (len <= 0) return NULL;

    char temp[256];
    strncpy(temp, select_start, len);
    temp[len] = '\0';
    int count = 0;
    char *token = strtok(temp, ",");
    while (token && count < MAXCOLS) {
        while (isspace(*token)) token++;        
        trim_trailing_ws(token);                
        col[count++] = strdup(token);           
        token = strtok(NULL, ",");
    }
    col[count] = NULL; 
    return col;
}
 
 //returns array of strong for where
 //ex: WHERE length >= 20
 //{length, >=, 20}
 char** select_conditional(char *cmd) {
     static char *res[4] = {NULL, NULL, NULL, NULL};
 
     for (int i = 0; i < 3; i++) {
         if (res[i]) {
             free(res[i]);
             res[i] = NULL;
         }
     }
 
     char *where = strfind(cmd, "WHERE");
     if (!where){
         return NULL;
     }
 
     where += 5;
     while (isspace(*where)){
         where++;
 
     } 
 
     //operations 
     char *op_start = NULL;
     char *operators[] = {"<=", ">=", "!=", "=", "<", ">", NULL};
 
     for (int i = 0; operators[i]; i++) {
         char *found = strstr(where, operators[i]);
         if (found && (!op_start || found < op_start)) {
             op_start = found;
         }
     }
 
     if (!op_start){
         return NULL;
     }
 
     //grabs col name
     int col_len = op_start - where;
     while (col_len > 0 && isspace(where[col_len - 1])) col_len--;
 
     char col[MAXBUF];
     strncpy(col, where, col_len);
     col[col_len] = '\0';
 
     //grabs operation 
     char op[3];
     if (strncmp(op_start, "<=", 2) == 0 || strncmp(op_start, ">=", 2) == 0 || strncmp(op_start, "!=", 2) == 0) {
         strncpy(op, op_start, 2);
         op[2] = '\0';
         op_start += 2;
     } else {
         op[0] = *op_start;
         op[1] = '\0';
         op_start += 1;
     }
 
     //if value
     while (isspace(*op_start)){
         op_start++;
 
     } 
     int val_len = strcspn(op_start, " ;\t\r\n");
 
     char val[MAXBUF];
     strncpy(val, op_start, val_len);
     val[val_len] = '\0';
 
     
     res[0] = strdup(col);
     res[1] = strdup(op);
     res[2] = strdup(val);
     res[3] = NULL;
 
     return res;
 }

/**
 *  Returns a pointer to a static buffer containing the parsed CREATE statement
 */
char* parse_create(const char *input) {
    static char result[MAXBUF]; 
    char table_name[MAXBUF];
    char columns[MAXBUF];
    char *start_paren;
    char *end_paren;
    result[0] = '\0'; 

    sscanf(input, "CREATE TABLE %s", table_name);
    char *paren = strchr(table_name, '(');
    if (paren) *paren = '\0';

    start_paren = strchr(input, '(');
    end_paren = strrchr(input, ')');
    if (!start_paren || !end_paren) {
        snprintf(result, MAXBUF, "Error: missing parentheses");
        return result;
    }
    start_paren++; 
    int len = end_paren - start_paren;
    strncpy(columns, start_paren, len);
    columns[len] = '\0';

    snprintf(result, MAXBUF, "%s", table_name);  
    strncat(result, "|", MAXBUF - strlen(result) - 1); 

    char *token = strtok(columns, ",");
    while (token != NULL) {
        char col_name[MAXBUF];
        char col_type[MAXBUF];
        int char_size = 0;

        while (isspace(*token)) token++;

        sscanf(token, "%s %s", col_name, col_type);

        if (strncasecmp(col_type, "INT", 3) == 0) {
            snprintf(col_type, sizeof(col_type), "smallint");
        } else if (strncasecmp(col_type, "VARCHAR", 7) == 0) {
            char *start = strchr(token, '(');
            char *end = strchr(token, ')');
            if (start && end && start < end) {
                sscanf(start + 1, "%d", &char_size);
            }
            if (char_size > 0) {
                snprintf(col_type, sizeof(col_type), "char(%d)", char_size);
            } else {
                snprintf(col_type, sizeof(col_type), "char(50)");
            }
        }
        strncat(result, col_name, MAXBUF - strlen(result) - 1);
        strncat(result, ":", MAXBUF - strlen(result) - 1);
        strncat(result, col_type, MAXBUF - strlen(result) - 1);

        token = strtok(NULL, ",");
        if (token != NULL) {
            strncat(result, ",", MAXBUF - strlen(result) - 1);
        }
    }

    strncat(result, ";", MAXBUF - strlen(result) - 1);
    return result;
}

//Testing checkpoing 

// parse schema.txt for a table schema (needs for insert & update) 
char* table_schema(char* table_name) {
    static char found[MAXBUF];
    char buf[BLOCKSIZE + 1]; // block + null terminator
    int fd; //checks if fil opend
    ssize_t bytes_read;
    char prefix[MAXBUF];
    
    snprintf(prefix, sizeof(prefix), "%s|", table_name);
    found[0] = '\0'; 

    fd = open("schema_file", O_RDONLY);
    if (fd < 0) {
        perror("can't opn file");
        return NULL;
    }

    while ((bytes_read = read(fd, buf, BLOCKSIZE)) > 0) {
        buf[bytes_read] = '\0'; // terminate buffer

        // Now simply split by semicolon `;`
        char *piece = strtok(buf, ";");
        while (piece != NULL) {
            //nam bfor = tablnam
            if (strstr(piece, prefix) == piece) {
                snprintf(found, sizeof(found), "%s;", piece); // re-add the ';'
                close(fd);
                return found;
            }
            piece = strtok(NULL, ";");
        }
    }

    close(fd);
    return NULL;
}



// parse the schema for size of padding needed (insert) 
// pars schma and rturn propr data string 
//to get size, simply do siz of rturnd char*

//value_string is just string of values (not yet in list format)
char* data_string(char* schema, char* value_string) {
    static char result[256];
    result[0] = '\0';

    //split up th columns
    char* columns = strchr(schema, '|');
    if (!columns) return NULL;
    columns++;

    // Remove ;
    char* semi = strchr(columns, ';');
    if (semi) *semi = '\0';


    //value formatted but can remove section if value param is in list format
    //i intend to format it into list bfore passing in so this is just extra for testing purposes
    //make sure to remove ' ' in char
    char* value_list[16];
    int vcount = 0;

    char *ptr = value_string;
    while (*ptr && vcount < 16) {
        while (*ptr == ' ') ptr++; // skip spaces

        if (*ptr == '\'') {
            ptr++; // skip opening quote
            value_list[vcount++] = ptr;
            while (*ptr && *ptr != '\'') ptr++;
            if (*ptr) {
                *ptr = '\0'; 
                ptr++;
            }
            if (*ptr == ',') ptr++; // skip comma
        } else {
            value_list[vcount++] = ptr;
            while (*ptr && *ptr != ',') ptr++;
            if (*ptr) {
                *ptr = '\0';
                ptr++;
            }
        }
    }
    // end 

    // go through each column
    char* col_token = strtok(columns, ",");
    int col_index = 0;

    while (col_token && col_index < vcount) {
        char name[64], type[64];
        int char_size = 0;

        // ky value pair
        sscanf(col_token, "%[^:]:%s", name, type);

        //smallint = 4
        if (strncasecmp(type, "smallint", 8) == 0) {
            char tmp[5];
            snprintf(tmp, sizeof(tmp), "%04d", atoi(value_list[col_index]));
            strncat(result, tmp, sizeof(result) - strlen(result) - 1);

        //int = 8
        } else if (strncasecmp(type, "int", 3) == 0) {
            char tmp[9];
            snprintf(tmp, sizeof(tmp), "%08d", atoi(value_list[col_index]));
            strncat(result, tmp, sizeof(result) - strlen(result) - 1);

        //char n = n
        } else if (strncasecmp(type, "char", 4) == 0) {
            sscanf(type, "char(%d)", &char_size);
            char padded[char_size + 1];

            // left padding 
            int val_len = strlen(value_list[col_index]);
            int pad = char_size - val_len;
            if (pad < 0) pad = 0; 

            memset(padded, ' ', char_size);                   
            if (val_len > 0) {
                strncpy(padded + pad, value_list[col_index], val_len); 
            }
            padded[char_size] = '\0';                         

            strncat(result, padded, sizeof(result) - strlen(result) - 1);
        }

        col_token = strtok(NULL, ",");
        col_index++;
    }

    return result;
}


char* extract_table_name(const char* sql) {
    static char table_name[128];
    table_name[0] = '\0';

    const char* ptr = NULL;

    if (strncasecmp(sql, "INSERT", 6) == 0) {
        ptr = strstr(sql, "INTO");
        if (!ptr) return NULL;
        ptr += 4;
    } else if (strncasecmp(sql, "SELECT", 6) == 0) {
        ptr = strstr(sql, "FROM");
        if (!ptr) return NULL;
        ptr += 4;
    } else if (strncasecmp(sql, "UPDATE", 6) == 0) {
        ptr = sql + 6; 
    } else if (strncasecmp(sql, "DELETE", 6) == 0) {
        ptr = strstr(sql, "FROM");
        if (!ptr) return NULL;
        ptr += 4;
    } else {
        return NULL; 
    }

    while (*ptr == ' ') ptr++;
    sscanf(ptr, "%127s", table_name);

    for (int i = 0; table_name[i]; i++) {
        if (!isalnum(table_name[i]) && table_name[i] != '_') {
            table_name[i] = '\0';
            break;
        }
    }

    return table_name;
}

// Last left off for testing 

char* extract_values_string(const char* sql) {
    const char* start = strchr(sql, '(');
    const char* end = strchr(sql, ')');
    if (!start || !end || end <= start) return NULL;

    static const int MAX_RESULT_LEN = 512;
    char* result = malloc(MAX_RESULT_LEN);
    if (!result) return NULL;

    result[0] = '\0';

    char temp[512];
    int len = end - start - 1;
    strncpy(temp, start + 1, len);
    temp[len] = '\0';

    char* ptr = temp;
    int first = 1;

    while (*ptr) {
        while (*ptr == ' ' || *ptr == ',') ptr++;  

        char value[128];
        int v = 0;

        if (*ptr == '\'') {
            ptr++;  
            while (*ptr && *ptr != '\'' && v < 127)
                value[v++] = *ptr++;
            if (*ptr == '\'') ptr++;  
        } else {
            while (*ptr && *ptr != ',' && *ptr != ' ' && v < 127)
                value[v++] = *ptr++;
        }

        value[v] = '\0';

        if (!first) strncat(result, ",", MAX_RESULT_LEN - strlen(result) - 1);
        strncat(result, value, MAX_RESULT_LEN - strlen(result) - 1);
        first = 0;
    }

    return result;
}

void remove_spaces(char *str) {
    char *read = str;
    char *write = str;

    while (*read) {
        if (*read != ' ') {
            *write++ = *read;
        }
        read++;
    }

    *write = '\0'; 
}

void replace_encoded_quotes(char *str) {
    char *read = str;
    char *write = str;

    while (*read) {
        if (strncmp(read, "%27", 3) == 0) {
            *write++ = '\'';  
            read += 3;
        } else {
            *write++ = *read++;
        }
    }
    *write = '\0';  
}

char* extract_update_values(const char* sql) {
    static char result[512];
    result[0] = '\0';

    const char* set_ptr = strstr(sql, "SET");
    const char* where_ptr = strstr(sql, "WHERE");

    if (!set_ptr || !where_ptr || set_ptr > where_ptr) {
        snprintf(result, sizeof(result), "ERROR: Invalid SQL format.");
        return result;
    }

    set_ptr += 3;
    while (*set_ptr == ' ') set_ptr++;

    char set_clause[512];
    int len = where_ptr - set_ptr;
    strncpy(set_clause, set_ptr, len);
    set_clause[len] = '\0';

    char* token = strtok(set_clause, ",");
    int first = 1;

    while (token != NULL) {
        char* equal = strchr(token, '=');
        if (equal) {
            char* value = equal + 1;
            while (*value == ' ') value++;

            if (!first) strncat(result, ",", sizeof(result) - strlen(result) - 1);
            strncat(result, value, sizeof(result) - strlen(result) - 1);
            first = 0;
        }
        token = strtok(NULL, ",");
    }

    return result;
}

int get_entry_size(const char* schema) {
    const char* columns = strchr(schema, '|');
    if (!columns) return -1;
    columns++; 

    char temp[512];
    strncpy(temp, columns, sizeof(temp) - 1);
    temp[sizeof(temp) - 1] = '\0';
    char* semi = strchr(temp, ';');
    if (semi) *semi = '\0';

    char* token = strtok(temp, ",");
    int total_size = 0;

    while (token) {
        char name[128], type[128];
        int char_size = 0;

        if (sscanf(token, "%[^:]:%s", name, type) == 2) {
            if (strncasecmp(type, "smallint", 8) == 0) {
                total_size += 4;
            } else if (strncasecmp(type, "int", 3) == 0 || strncasecmp(type, "integer", 7) == 0) {
                total_size += 8;
            } else if (strncasecmp(type, "char", 4) == 0) {
                if (sscanf(type, "char(%d)", &char_size) == 1) {
                    total_size += char_size;
                }
            }
        }

        token = strtok(NULL, ",");
    }

    return total_size;
}


char* normalize_value(char* str) {
    while (isspace(*str)) str++;
    char* end = str + strlen(str) - 1;
    while (end > str && isspace(*end)) {
        *end = '\0';
        end--;
    }
    char* p = str;
    int is_numeric = 1;
    while (*p) {
        if (!isdigit(*p)) {
            is_numeric = 0;
            break;
        }
        p++;
    }
    if (is_numeric) {
        while (*str == '0' && *(str + 1) != '\0') str++;
    }
    return str;
}

Pair get_column_offset_and_size(const char* schema, const char* target_col) {
    Pair result = { -1, -1 }; 
    const char* columns = strchr(schema, '|');
    if (!columns) return result;
    columns++;  

    char temp[512];
    strncpy(temp, columns, sizeof(temp) - 1);
    temp[sizeof(temp) - 1] = '\0';

    char* semi = strchr(temp, ';');
    if (semi) *semi = '\0';

    char* token = strtok(temp, ",");
    int offset = 0;

    while (token) {
        char name[128], type[128];
        int col_size = 0;

        if (sscanf(token, "%[^:]:%s", name, type) == 2) {
            for (int i = 0; name[i]; i++) {
                name[i] = tolower(name[i]);
            }

            if (strncasecmp(type, "smallint", 8) == 0) {
                col_size = 4;
            } else if (strncasecmp(type, "int", 3) == 0 || strncasecmp(type, "integer", 7) == 0) {
                col_size = 8;
            } else if (strncasecmp(type, "char", 4) == 0) {
                sscanf(type, "char(%d)", &col_size);
            }

            if (strcmp(name, target_col) == 0) {
                result.offset = offset;
                result.size = col_size;
                return result;
            }

            offset += col_size;
        }

        token = strtok(NULL, ",");
    }

    return result;  
}

int get_size(char* array[]) {
    int count = 0;
    while (array[count] != NULL) {
        count++;
    }
    return count; 
}

void trim_trailing_ws(char *str) {
    int len = strlen(str);
    while (len > 0 && isspace(str[len - 1])) {
        str[len - 1] = '\0';
        len--;
    }
}

char** get_all_column_names_from_schema(const char* schema) {
    char** col_names = malloc((MAXCOLS + 1) * sizeof(char*));
    if (!col_names) return NULL;
    for (int i = 0; i <= MAXCOLS; i++) col_names[i] = NULL;
    const char* pipe = strchr(schema, '|');
    if (!pipe) return NULL;
    pipe++;  
    char temp[512];
    strncpy(temp, pipe, sizeof(temp) - 1);
    temp[sizeof(temp) - 1] = '\0';
    char* semi = strchr(temp, ';');
    if (semi) *semi = '\0';
    int count = 0;
    char* token = strtok(temp, ",");
    while (token && count < MAXCOLS) {
        while (isspace(*token)) token++;

        char* colon = strchr(token, ':');
        if (colon) {
            *colon = '\0';  
            col_names[count++] = strdup(token);
        }

        token = strtok(NULL, ",");
    }

    col_names[count] = NULL;
    return col_names;
}

void remove_surrounding_quotes(char *str) {
    size_t len = strlen(str);
    if (len >= 2 && str[0] == '\'' && str[len - 1] == '\'') {
        memmove(str, str + 1, len - 2);  
        str[len - 2] = '\0';            
    }
}