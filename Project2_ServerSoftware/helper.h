#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/fcntl.h>
#include <fcntl.h> 
#include <ctype.h>

#define MAXBUF (8192)
// Predefined block size must each be 256 
#define BLOCKSIZE 256
// Predefined name of schema file 
#define SCHEMA_FILE "schema_file"
#define MAXCOLS 16


typedef struct {
    int offset;
    int size;
} Pair;

typedef struct {
    char* name;
    char* value;
} Pair2;

FILE *f;


// Headers
char* call_query(char *query);
int add_to_schema_file(const char *table_info);
int add_data(const char *tablename, const char *data);
char* select_col(char *tablename, char *columns[], Pair2 where, char *data, int isDelete);

char* select_tablename(char* cmd);
char* strfind(const char* in, const char* val);
char** select_colnames(char *cmd);
char** select_conditional(char *cmd);

char* parse_create(const char *input);
char* table_schema(char* table_name);
char* data_string(char* schema, char* value_string);

char* extract_table_name(const char* sql);
char* extract_values_string(const char* sql);
char* extract_update_values(const char* sql);
void remove_spaces(char *str);
void replace_encoded_quotes(char *str);
int get_entry_size(const char* schema);
char* normalize_value(char* str);
Pair get_column_offset_and_size(const char* schema, const char* target_col);
int get_size(char* array[]);
void trim_trailing_ws(char *str);
char** get_all_column_names_from_schema(const char* schema);
void remove_surrounding_quotes(char *str);
