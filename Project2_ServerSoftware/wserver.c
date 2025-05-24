#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>

#include "io_helper.h"
#include "request.h"

//FILE *logfile;

#define MAXBUF 8192
#define MAX_QUEUE 1024

//COND VARIABLES for thread
pthread_cond_t fill;
pthread_cond_t empty;
pthread_mutex_t lock;



char root[] = ".";  //default working directory
char *schedule = "FIFO";    //FIFO or SFF
int buff_size = 1;          //buff
int num_threads = 1;        //number of worker threads

//SFF request in buffer has connection and file size 
typedef struct {
  int connection;
  off_t file_size;
} request_t;

//circular buffer & indeex
request_t *buff;
int head = 0;
int tail = 0;
int cnt = 0;



int enqueue(request_t *buf, int *tail, int *cnt, int buff_size, int conn, off_t size);
void *worker(void *arg);
request_t dequeue_fifo(request_t *buf, int *head, int *cnt, int buff_size);



//
//./wserver [-d <basedir>] [-p <portnum>] [-t threads] [-b buffers] [-s schedalg]
//
//main grabs client and accepts or denies. Main thread creates worker threads 
//and enqueues requests into buffer
int main(int argc, char *argv[]) {
  int c, port = 10000;
  char *root_dir = root;

  //needs [-d basedir] [-p port] [-t threads] [-b buffers] [-s schedalg]
  while ((c = getopt(argc, argv, "d:p:t:b:s:")) != -1) {
    switch (c) {
      case 'd':
        root_dir = optarg;
        break;
      case 'p':
        port = atoi(optarg);
        break;
      case 't':
        num_threads = atoi(optarg);
        break;
      case 'b':
        buff_size = atoi(optarg);
        break;
      case 's':
        schedule = optarg;
        break;
      default:
        fprintf(stderr,
                "usage: wserver [-d basedir] [-p port] [-t threads] [-b "
                "buffers] [-s schedalg]\n");
        exit(1);
    }
  }
  //Open the correct log file based on scheduling algorithm
  /*if (strcasecmp(schedule, "SFF") == 0) {
      logfile = fopen("SFF.log", "a");
  } else {
      logfile = fopen("FIFO.log", "a");
  }

  if (!logfile) {
      perror("fopen log");
      exit(1);
  }*/

  //run out of this directory
  chdir_or_die(root_dir);
  //signal(SIGINT, handle_sigint);

  //create buff & array
  buff = malloc(sizeof(request_t) * buff_size);


  if (!buff) {
    perror("malloc");
    exit(1);

  }

  //COND 
  pthread_mutex_init(&lock, NULL);
  pthread_cond_init(&fill, NULL);
  pthread_cond_init(&empty, NULL);

  pthread_t threads[num_threads];

  //num of threads CREATED
  for (int i = 0; i < num_threads; i++) {
    pthread_create(&threads[i], NULL, worker, NULL);
  }

  //now, get to work
  //master thread
  int listen_fd = open_listen_fd_or_die(port);  // uses whatever port we declare
  while (1) {

    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    int connection =
        accept_or_die(listen_fd, (struct sockaddr *)&client_addr, &client_len);


    //fprintf(stderr, "DEBUG: accept_or_die got fd=%d\n", connection);

    //fprintf(logfile, "DEBUG: accept_or_die got fd=%d\n", connection);
    //fflush(logfile);

    //sff: peek at file name and stat its size
    off_t size = 0;

    if (strcasecmp(schedule, "SFF") == 0) {
      char buf[MAXBUF + 1], *eol;

      int n = recv(connection, buf, MAXBUF, MSG_PEEK);

      if (n > 0) {
        //fprintf(stderr, "DEBUG: Raw request: %s\n", buf);

        buf[n] = '\0';
        eol = strstr(buf, "\r\n");
        if (eol) {

          *eol = '\0';
          char method[MAXBUF], uri[MAXBUF], version[MAXBUF];

          sscanf(buf, "%s %s %s", method, uri, version);

          if (!strstr(uri, "..")) {
            char filename[MAXBUF], cgiargs[MAXBUF];
            request_parse_uri(uri, filename, cgiargs);


            //printf("DEBUG: URI: %s -> filename: %s\n", uri, filename);

            struct stat sbuf;
            if (stat(filename, &sbuf) == 0) {
              size = sbuf.st_size;


              //printf("DEBUG: stat() success: %s = %ld bytes\n", filename,size);
            } else {
              //perror("DEBUG: stat() failed");




              fprintf(stderr, "SECURITY: ERROR can't use [..] \n");
            }
          }
        }
      }
    }

    pthread_mutex_lock(&lock);

    //WAIT if full thread
    //NO SPIN WAITING
    //THIS IS CONDITIONAL 
    while (cnt == buff_size) {



      fprintf(stderr, "BLOCKED: FULL THREAD\n");
    
      pthread_cond_wait(&empty, &lock);
    }

    //DEBUG print
    //fprintf(stderr, "DEBUG: Enqueuing fd=n");
    fprintf(stderr, "QUEUE: Enqueuing fd=%d, size=%ld (%s mode)\n", connection,
            size, schedule);

    //fprintf(logfile, "QUEUE: Enqueuing fd=%d, size=%ld (%s mode)\n",
    //connection, size, schedule); fflush(logfile);

    if (enqueue(buff, &tail, &cnt, buff_size, connection, size) != 0) {
      fprintf(stderr, "ERROR: buffer full can't enqueue \n");
      close_or_die(connection);
      pthread_mutex_unlock(&lock);
      continue;
    }


    pthread_cond_signal(&fill);
    pthread_mutex_unlock(&lock);
  }
  //fflush(logfile);
  //fclose(logfile);
  return 0;
}


// Enqueues request into circular buffer
int enqueue(request_t *buf, int *tail, int *cnt, int buff_size, int conn, off_t size) {
    if (*cnt == buff_size) return -1; // Buffer full
    buf[*tail].connection = conn;
    buf[*tail].file_size = size;
    *tail = (*tail + 1) % buff_size;
    (*cnt)++;
    return 0;
}


//things to add
//worker threads
//pool-of-threads approach, each thread is blocked until there is an http
//request for it to handle. static and dynamic requests supported

//creates a worker thread that runs infintely and handles the requests
void *worker(void *arg) {
  while (1) {
    pthread_mutex_lock(&lock);
    while (cnt == 0) {
      pthread_cond_wait(&fill, &lock);
    }

    request_t req;
    if (strcasecmp(schedule, "SFF") == 0) {
      //find smallest filesize
      int best = head;
      for (int i = 0; i < cnt; i++) {


        int curr = (head + i) % buff_size;
        if (buff[curr].file_size < buff[best].file_size) best = curr;
      }

      req = buff[best];


      int last = (tail - 1 + buff_size) % buff_size;
      buff[best] = buff[last];
      tail = last;
      cnt--;
    } else {


      //FIFO head
      req = dequeue_fifo(buff, &head, &cnt, buff_size);

    }

    //printf(logfile, "SERVER: Thread %ld handling %s (fd=%d, size=%ld)\n",
    //pthread_self(), schedule, req.connection, req.file_size);

    //FIFO
    fprintf(stderr, "SERVER: Thread %ld handling %s (fd=%d, size=%ld)\n",
            pthread_self(), schedule, req.connection, req.file_size);

    //fflush(logfile);

    pthread_cond_signal(&empty);
    pthread_mutex_unlock(&lock);

    request_handle(req.connection);
    close_or_die(req.connection);
  }

  return NULL;
}

//dequeues from buffer the oldest request for worker 
request_t dequeue_fifo(request_t *buf, int *head, int *cnt, int buff_size) {
    request_t req = buf[*head];
    *head = (*head + 1) % buff_size;
    (*cnt)--;
    return req;
}
