#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h> 

#define MAXBUF (8192)
#define BLOCK_SIZE (256)


// headers
char* call_query(char *query);
char* parse_create(const char *input);
char* table_schema(char* table_name);
char* data_string(char* schema, char* value_string);


#include <stdio.h>
#include <string.h>
#include <ctype.h>




int main(int argc, char *argv[]) {
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
    char *n = table_schema("movies");
    //note  Lyle, Lyle, Crocodile may not work well due to commas
    char *v = strdup("2, 'Lyle, Lyle, Crocodile', 100");
    char *query_response = data_string(n, v);
    //content_length += snprintf(content + content_length, MAXBUF - content_length, "<p>%s</p>\r\n", n);
    //content_length += snprintf(content + content_length, MAXBUF - content_length, "<p>%s</p>\r\n", v);
    content_length += snprintf(content + content_length, MAXBUF - content_length, "<pre>%s</pre>\r\n", query_response);


    free(v);

    // Generate the HTTP response 
    printf("Content-Length: %d\r\n", content_length);
    printf("Content-Type: text/html\r\n\r\n");
    printf("%s", content);
    fflush(stdout);

    exit(0);
}

/**
 * checks what query it is and calls the specific function for that query 
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

    char *pop = parse_create(query); //tester

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
        //create_table(rest)
        snprintf(response, sizeof(response), "CREATE RUNS: %s", rest);
        snprintf(response, sizeof(response), "YO PRUNT %s", pop);
        

    } else if (strcmp(token, "INSERT") == 0) {

        //insert_into(rest)
        // add parsing so value is a list 
        // char *v = strdup("2, 'Lyle, Lyle, Crocodile', 100");
        //call data_string 
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


void create_table(char * line){

}



//Parsing functions



/**
 *  Returns a pointer to a static buffer containing the parsed CREATE statement
 */
char* parse_create(const char *input) {
    static char result[MAXBUF]; 
    char table_name[MAXBUF];
    char columns[MAXBUF];
    char *start_paren;
    char *end_paren;
    result[0] = '\0'; // Clear result buffer

    //table name
    sscanf(input, "CREATE TABLE %s", table_name);
    char *paren = strchr(table_name, '(');
    if (paren) *paren = '\0';

    //() grab insides 
    start_paren = strchr(input, '(');
    end_paren = strrchr(input, ')');
    if (!start_paren || !end_paren) {
        snprintf(result, MAXBUF, "Error: missing parentheses");
        return result;
    }
    start_paren++; // skip (
    int len = end_paren - start_paren;
    strncpy(columns, start_paren, len);
    columns[len] = '\0';

    // Start building the result
    snprintf(result, MAXBUF, "%s", table_name);  
    strncat(result, "|", MAXBUF - strlen(result) - 1); 


    // format column names
    char *token = strtok(columns, ",");
    while (token != NULL) {
        char col_name[MAXBUF];
        char col_type[MAXBUF];
        int char_size = 0;

        // Clean spaces
        while (isspace(*token)) token++;

        // Parse column name and type
        sscanf(token, "%s %s", col_name, col_type);

        // Normalize type
        if (strncasecmp(col_type, "INT", 3) == 0) {
            snprintf(col_type, sizeof(col_type), "smallint");
        } else if (strncasecmp(col_type, "VARCHAR", 7) == 0) {
            // Find '(' after "VARCHAR"
            char *start = strchr(token, '(');
            char *end = strchr(token, ')');
            if (start && end && start < end) {
                sscanf(start + 1, "%d", &char_size);
            }
            if (char_size > 0) {
                snprintf(col_type, sizeof(col_type), "char(%d)", char_size);
            } else {
                snprintf(col_type, sizeof(col_type), "char(50)"); // Default safe fallback if sscanf fails
            }
        }
        

        // add to result
        strncat(result, col_name, MAXBUF - strlen(result) - 1);
        strncat(result, " ", MAXBUF - strlen(result) - 1);
        strncat(result, col_type, MAXBUF - strlen(result) - 1);

     
        token = strtok(NULL, ",");
        if (token != NULL) {
            strncat(result, ", ", MAXBUF - strlen(result) - 1);
        }
    }

    strncat(result, ";", MAXBUF - strlen(result) - 1);
    return result;
}


// parse schema.txt for a table schema (needs for insert & update) 
char* table_schema(char* table_name) {
    static char found[MAXBUF];
    char buf[BLOCK_SIZE + 1]; // block + null terminator
    int fd; //checks if fil opend
    ssize_t bytes_read;
    char prefix[MAXBUF];
    
    snprintf(prefix, sizeof(prefix), "%s|", table_name);
    found[0] = '\0'; 

    fd = open("schema.txt", O_RDONLY);
    if (fd < 0) {
        perror("can't opn file");
        return NULL;
    }

    while ((bytes_read = read(fd, buf, BLOCK_SIZE)) > 0) {
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

//update



