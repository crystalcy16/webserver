#include "helper.h"

void run_tests() {

    // data_string unit testing 
    char schema_[] = "employees|employee_id:smallint,first_name:char(10),last_name:char(10);";
    char values[] = "3,'Charlie','Smith'";

    char *formatted = data_string(schema_, values);
    assert(formatted != NULL);
    assert(strncmp(formatted, "0003", 4) == 0);
    assert(strstr(formatted, "Charlie") != NULL);
    assert(strstr(formatted, "Smith") != NULL);
    assert(strlen(formatted) == 4 + 10 + 10);
    
    // extract_table_name unit testing 
    assert(strcmp(extract_table_name("INSERT INTO employees VALUES (1, 'A', 'B');"), "employees") == 0);
    assert(strcmp(extract_table_name("SELECT * FROM employees;"), "employees") == 0);
    assert(strcmp(extract_table_name("UPDATE employees SET id = 1;"), "employees") == 0);
    assert(strcmp(extract_table_name("DELETE FROM employees WHERE id = 1;"), "employees") == 0);
    assert(extract_table_name("DROP TABLE test;") == NULL);

    // extract_values_string unit testing
    char *result1 = extract_values_string("INSERT INTO employees VALUES (3, 'Charlie', 25);");
    assert(strcmp(result1, "3,Charlie,25") == 0);
    free(result1); 

    char *result2 = extract_values_string("INSERT INTO employees VALUES ('001', 'Alice', '42');");
    assert(strcmp(result2, "001,Alice,42") == 0);
    free(result2);

    char *result3 = extract_values_string("INSERT INTO employees VALUES ( 'a' , 'b', 'c');");
    assert(strcmp(result3, "a,b,c") == 0);
    free(result3);

    // remove_spaces unit testing
    char str1[] = "  hello world  ";
    remove_spaces(str1);
    assert(strcmp(str1, "helloworld") == 0);

    char str2[] = "nospaces";
    remove_spaces(str2);
    assert(strcmp(str2, "nospaces") == 0);

    // replace_encoded_quotes unit testing
    char str3[] = "hello%27world%27again";
    replace_encoded_quotes(str3);
    assert(strcmp(str3, "hello'world'again") == 0);

    char str4[] = "%27abc";
    replace_encoded_quotes(str4);
    assert(strcmp(str4, "'abc") == 0);

    // extract_update_values unit testing
    char *update1 = extract_update_values("UPDATE employees SET id = 3, name = 'Bob' WHERE id = 3;");
    remove_spaces(update1);
    assert(strcmp(update1, "3,'Bob'") == 0);

    char *update2 = extract_update_values("UPDATE employees SET name = 'A', age = 22 WHERE name = 'A';");
    remove_spaces(update2);
    assert(strcmp(update2, "'A',22") == 0);

    // get_entry_size unit testing
    int size1 = get_entry_size("employees|id:smallint,name:char(20),age:int;");
    assert(size1 == (4 + 20 + 8)); 

    int size2 = get_entry_size("table|col1:char(10),col2:char(5);");
    assert(size2 == 15);

    int size3 = get_entry_size("badformat");
    assert(size3 == -1);


    // normalize_value unit testing 
    char val1[] = "   0003   ";
    assert(strcmp(normalize_value(val1), "3") == 0);

    char val2[] = "   John   ";
    assert(strcmp(normalize_value(val2), "John") == 0);

    char val3[] = "000";
    assert(strcmp(normalize_value(val3), "0") == 0);

    // get_column_offset_and_size unit testing 
    const char* schema = "employees|employee_id:smallint,first_name:char(50),last_name:char(50);";
    Pair p1 = get_column_offset_and_size(schema, "employee_id");
    assert(p1.offset == 0 && p1.size == 4);

    Pair p2 = get_column_offset_and_size(schema, "first_name");
    assert(p2.offset == 4 && p2.size == 50);

    Pair p3 = get_column_offset_and_size(schema, "last_name");
    assert(p3.offset == 54 && p3.size == 50);

    Pair p4 = get_column_offset_and_size(schema, "not_found");
    assert(p4.offset == -1 && p4.size == -1);

    //get_size unit testing 
    char* col_array[] = {"id", "name", "age", NULL};
    assert(get_size(col_array) == 3);

    char* empty_array[] = {NULL};
    assert(get_size(empty_array) == 0);

    // trim_trailing_ws unit testing 
    char str6[] = "hello   ";
    trim_trailing_ws(str6);
    assert(strcmp(str6, "hello") == 0);

    char str7[] = "no_spaces";
    trim_trailing_ws(str7);
    assert(strcmp(str7, "no_spaces") == 0);

    //get_all_column_names_from_schema unit testing 
    char** names = get_all_column_names_from_schema(schema);
    assert(names != NULL);
    assert(strcmp(names[0], "employee_id") == 0);
    assert(strcmp(names[1], "first_name") == 0);
    assert(strcmp(names[2], "last_name") == 0);
    assert(names[3] == NULL);

    for (int i = 0; names[i] != NULL; i++) free(names[i]);
    free(names);

    // remove_surrounding_quotes unit testing 
    char q1[] = "'Charlie'";
    remove_surrounding_quotes(q1);
    assert(strcmp(q1, "Charlie") == 0);

    char q2[] = "John";
    remove_surrounding_quotes(q2);
    assert(strcmp(q2, "John") == 0); 
}

//Testing for create table 
void run_add_to_schema_file_tests() {
    //const char *test_schema_path = "schema_file";
    //remove(test_schema_path);  
    const char *test_table_name = "test_table";

    const char *table_str = "test_table|id:smallint,name:char(10);";
    int res = add_to_schema_file(table_str);
    assert(res == 0);

    char *schema = table_schema("test_table");
    assert(schema != NULL);
    assert(strcmp(schema, table_str) == 0);

    remove(test_table_name);
    printf("add_to_schema_file and table_schema passed\n");
}

//Testing create more 
void test_create() {
    char query[] = "CREATE TABLE friends (id INT, name VARCHAR(10));";
    char *res = call_query(query);
    assert(strstr(res, "CREATE RUNS:") != NULL);
    printf("CREATE test passed\n");
}

//Testing insert data
void test_insert() {
    call_query("CREATE TABLE friends (id INT, name VARCHAR(10));");
    char query[] = "INSERT INTO friends VALUES (1, 'Alice');";
    char *res = call_query(query);
    assert(strstr(res, "INSERT RUNS:") != NULL);
    printf("INSERT test passed\n");
}

//Testing select statements 
void test_select() {
    call_query("INSERT INTO friends VALUES (2, 'Bob');");
    char query[] = "SELECT id, name FROM friends;";
    char *res = call_query(query);
    assert(strstr(res, "SELECT RUNS:") != NULL || strstr(res, "RESULT:") != NULL);
    printf("SELECT test passed\n");
    remove("schema_file");
    remove("friends");
}

int test_main() {
    run_tests();
    run_add_to_schema_file_tests();
    test_create();
    test_insert();
    test_select();
    printf("All tests passed :) \n");
    return 0;
}
