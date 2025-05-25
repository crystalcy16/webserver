#!/bin/bash
PORT=4444
SERVER=./wserver
ROOT=.
CGI_SCRIPT=sql.cgi
BASE_URL="http://localhost:$PORT/$CGI_SCRIPT"

#start server
echo "Server starts"
$SERVER -d $ROOT -p $PORT &
SERVER_PID=$!

#server to start
sleep 1

echo " "
echo "Test 1: Invalid Query => insert123"
curl "$BASE_URL?query=insert123"
echo -e "\n---\n"

echo " "
echo "Test 2: Empty Query =>  "
curl "$BASE_URL"
echo -e "\n---\n"

echo ""
echo "Test 3: CREATE TABLE => CREATE TABLE employees(employee_id INT, first_name VARCHAR(50), last_name VARCHAR(50))"
curl "$BASE_URL?query=CREATE%20TABLE%20employees%20(employee_id%20INT,%20first_name%20VARCHAR(50),%20last_name%20VARCHAR(50))"
echo -e "\n---\n"

echo "Test 4: INSERT INTO [non-existent table] => INSERT INTO WOMP VALUES (1, 'NO', 'SIR')"
curl "$BASE_URL?query=INSERT%20INTO%20womp%20VALUES%20(1,%27no%27,%27sir%27)"
echo -e "\n---\n"

echo "Test 5: INSERT INTO employees => INSERT INTO employees VALUES (1, 'A', 'Please')"
curl "$BASE_URL?query=INSERT%20INTO%20employees%20VALUES%20(1,%27A%27,%27Please%27)"
echo -e "\n---\n"

echo "Test 6: SELECT FROM [non-existent table] => SELECT * FROM womp"
curl "$BASE_URL?query=SELECT%20*%20FROM%20womp"
echo -e "\n---\n"

echo "Test 7: SELECT FROM employees => SELECT employee_id, first_name FROM employees WHERE employee_id = 0001"
curl "$BASE_URL?query=SELECT%20employee_id,%20first_name%20FROM%20employees%20WHERE%20employee_id%20=%201"
echo -e "\n---\n"

echo "Test 8: UPDATE employees => UPDATE employees SET first_name = 'Amazon', last_name = 'Thanks' WHERE employee_id = 1"
curl "$BASE_URL?query=UPDATE%20employees%20SET%20first_name%20=%20%27Alicia%27,%20last_name%20=%20%27Smythe%27%20WHERE%20employee_id%20=%201"
echo -e "\n---\n"

echo "Test 9: DELETE FROM employees => DELETE FROM employees WHERE employee_id = 1"
curl "$BASE_URL?query=DELETE%20FROM%20employees%20WHERE%20employee_id%20=%201"
echo -e "\n---\n"

echo "Test 10: SELECT after DELETE => SELECT * FROM employees WHERE employee_id = 1"
curl "$BASE_URL?query=SELECT%20*%20FROM%20employees%20WHERE%20employee_id%20=%201"
echo -e "\n---\n"

echo -e "\nAll tests complete. Server killed."
kill $SERVER_PID
