#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <assert.h>

#include "bench.h"

int
main(int argc, char *argv[])
{
  int size, length, i;
  int *data;
  int *target;
  char sync;
  int fds[2];

  double bs, as, br, ar;

  if (argc > 1) {
    size = atoi(argv[1]);
    length = size / sizeof(int);
    size = sizeof(int) * length;
  } else
    exit(1);

  if (socketpair(AF_UNIX, SOCK_STREAM, 0, fds) == -1) {
    perror("socketpair");
    exit(1);
  }

  log_perf("before init, size=%d", size);
  data = malloc(size);
  for(i=0; i<length; i++)
    data[i] = i;
  log_perf("after init");

  if (!fork()) {  /* child */
    free(data);
    target = malloc(size);
    internal_send(fds[1], &sync, sizeof(char));
    br = log_perf("start read");
    internal_recv(fds[1], target, size);
    ar = log_perf("done read");

    internal_send(fds[1], &br, sizeof(double));
    internal_send(fds[1], &ar, sizeof(double));

    for(i=0; i<length; i++)
      assert(target[i] == i);
    
  } else { /* parent */
    internal_recv(fds[0], &sync, sizeof(char));
    bs = log_perf("start write");
    internal_send(fds[0], data, size);
    as = log_perf("done write");

    internal_recv(fds[0], &br, sizeof(double));
    internal_recv(fds[0], &ar, sizeof(double));
    printf("TIME: %f\n", max(as,ar) - min(bs,br) );

    /* wait for child to die */
    wait(NULL); 
  }

  return 0;
}
