#!/bin/bash

PORT=4444
SERVER=./wserver
ROOT=.
CURL_PIDS=()


SMALL_URL="http://localhost:$PORT/small.html"
MEDIUM_URL="http://localhost:$PORT/medium.html"
LARGE_URL="http://localhost:$PORT/large.html"

SMALL_SFF="http://localhost:$PORT/spin.cgi?1"
MEDIUM_SFF="http://localhost:$PORT/spin.cgi?2"
LARGE_SFF="http://localhost:$PORT/spin.cgi?3"

BAD_URL="http://localhost:$PORT/../spin.cgi?1"

start_server_with_threads() {
    ALG=$1
    THREADS=$2
    echo -e "***Starting server with $ALG scheduling ($THREADS threads)***"
    $SERVER -d $ROOT -p $PORT -t $THREADS -b 8 -s $ALG &
    SERVER_PID=$!
    sleep 1
}

stop_server() {
      sleep 1
    if [[ -n "$SERVER_PID" && "$SERVER_PID" =~ ^[0-9]+$ ]]; then
        if ps -p $SERVER_PID > /dev/null 2>&1; then
            echo "INFO: Stopping server (PID=$SERVER_PID)..."
            kill $SERVER_PID 2>/dev/null
            wait $SERVER_PID 2>/dev/null || true
        else
            echo "BROKEN: Server PID $SERVER_PID NO run."
        fi
    else
        echo "BROKEN: No valid SERVER_PID."
    fi
    sleep 1
}


timed_curl() {
    ALG=$1
    URL=$2
    LABEL=$3

    {
        start_epoch=$(date +%s.%N)
        start_human=$(date "+%H:%M:%S")
    

        if [[ "$LABEL" == "spin1" || "$LABEL" == "spin2" || "$LABEL" == "spin3" || "$LABEL" == "spin" ]]; then
            curl --max-time 5 -s "$URL"
        else
            curl --max-time 5 -s "$URL" > /dev/null
        fi
        end_epoch=$(date +%s.%N)
        end_human=$(date "+%H:%M:%S")
        

        duration=$(awk "BEGIN {print $end_epoch - $start_epoch}")
       
    } &

    CURL_PIDS+=($!) 

}





echo " "
echo "-----[TEST1.1] MULTI-THREADING (static)-----"
start_server_with_threads NONE 6 #no scheudling 



timed_curl FIFO "$SMALL_URL" "small1"
timed_curl FIFO "$MEDIUM_URL" "medium"
timed_curl FIFO "$SMALL_URL" "small2"
timed_curl FIFO "$LARGE_URL" "large"

sleep 0.5


for pid in "${CURL_PIDS[@]}"; do
    wait "$pid"
done
CURL_PIDS=()  # Clear for next test


echo "INFO: All curl jobs completed for TEST1.1"

echo " "
echo "***EXPECTED RESULT: should be diff numbered [THREADS] ***"
echo " "
stop_server



echo " "
echo "-----[TEST1.2] MULTI-THREADING (dynamic)-----"
start_server_with_threads NONE 3 #no scheudling 



timed_curl FIFO "$SMALL_SFF" "spin1"
timed_curl FIFO "$MEDIUM_SFF" "spin2"
timed_curl FIFO "$LARGE_SFF" "spin3"

sleep 0.5


for pid in "${CURL_PIDS[@]}"; do
    wait "$pid"
done
CURL_PIDS=()  # Clear for next test


echo "INFO All curl jobs completed for TEST1.2"
echo " "
echo "***EXPECTED RESULT: should be diff numbered [THREADS] ***"
echo " "
stop_server





echo " "
echo "-----[TEST2] FIFO ORDER-----"
start_server_with_threads FIFO 1


timed_curl FIFO "$MEDIUM_URL" "medium"
sleep 0.5 
timed_curl FIFO "$SMALL_URL" "small"
sleep 0.5 
timed_curl FIFO "$LARGE_URL" "large"



sleep 0.5 #buffer time for server since scripts are super fast & file not that big



for pid in "${CURL_PIDS[@]}"; do
    wait "$pid"
done
CURL_PIDS=()  # Clear for next test

echo "INFO: All curl jobs completed for TEST2"
echo " "
echo "***EXPECTED RESULT: should [SERVER] handled  medium, small, large ***"
echo " "
stop_server

echo " "
echo "-----[TEST3] SFF ORDER-----"
start_server_with_threads SFF 1


timed_curl SFF "$LARGE_URL" "large" &
timed_curl SFF "$MEDIUM_URL" "medium" &
timed_curl SFF "$SMALL_URL" "small"

sleep 0.5

for pid in "${CURL_PIDS[@]}"; do
    wait "$pid"
done
CURL_PIDS=()  


echo "INFO: All curl jobs completed for TEST3"
echo " "
echo "***EXPECTED RESULT: should [SERVER] handled small, medium, large ***"
echo " "
stop_server

echo " "
echo "-----[TEST4] MASTER BLOCKING (buffer full)-----"
start_server_with_threads NONE 1  # 1 worker thread

# Send 4 requests, but buffer is only 2 and 1 thread works slowly
timed_curl FIFO "$SMALL_SFF" "spin1"  # fills worker
timed_curl FIFO "$MEDIUM_SFF" "spin2"  #buff 1
timed_curl FIFO "$LARGE_SFF" "spin3"   #buff 2
timed_curl FIFO "$SMALL_SFF" "spin"  
timed_curl FIFO "$SMALL_SFF" "spin"  #buff 4
timed_curl FIFO "$SMALL_SFF" "spin"  
timed_curl FIFO "$SMALL_SFF" "spin"  #buff 5
timed_curl FIFO "$SMALL_SFF" "spin"  
timed_curl FIFO "$SMALL_SFF" "spin"  #buff 7
timed_curl FIFO "$SMALL_SFF" "spin"  #buff 8
timed_curl FIFO "$SMALL_SFF" "spin1"  #blockd

for pid in "${CURL_PIDS[@]}"; do
    wait "$pid"
done
CURL_PIDS=()

echo "INFO: All curl jobs completed for TEST4"
echo " "
echo "***EXPECTED RESULT: should [BLOCK] show up ***"
echo " "
stop_server

echo " "
echo "-----[TEST5] GOING UP IN DIRECTORY-----"
start_server_with_threads SFF 1  # 1 worker thread
 
curl -v "http://localhost:4444/%2e%2e/spin.cgi?1"


for pid in "${CURL_PIDS[@]}"; do
    wait "$pid"
done
CURL_PIDS=()

echo "INFO: All curl jobs completed for TEST4"
echo " "
echo "***EXPECTED RESULT: should [SECURITY] show up ***"
echo " "
stop_server