#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAXBUF (8192)

// headers
char* call_query(char *query);

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
    char *query_response = call_query(query);
    content_length += snprintf(content + content_length, MAXBUF - content_length, "<p>%s</p>\r\n", query_response);

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

    } else if (strcmp(token, "INSERT") == 0) {

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
