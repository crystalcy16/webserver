#include "helper.h"

extern int test_main();

int main(int argc, char *argv[]) {

    f = fopen("output.log", "a+");  
    if (f == NULL) {
        perror("Error with logging file");
    }

    if (argc == 2 && strcmp(argv[1], "-t") == 0) {
        return test_main();  // Run your test suite and exit
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

