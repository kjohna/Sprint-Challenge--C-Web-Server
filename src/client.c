#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "lib.h"

#define BUFSIZE 4096 // max number of bytes we can get at once

/**
 * Struct to hold all three pieces of a URL
 */
typedef struct urlinfo_t
{
  char *hostname;
  char *port;
  char *path;
} urlinfo_t;

/**
 * Tokenize the given URL into hostname, path, and port.
 *
 * url: The input URL to parse.
 *
 * Store hostname, path, and port in a urlinfo_t struct and return the struct.
*/
urlinfo_t *parse_url(char *url)
{
  // copy the input URL so as not to mutate the original
  char *hostname = strdup(url);
  char *port;
  char *path;

  urlinfo_t *urlinfo = malloc(sizeof(urlinfo_t));

  /*
    We can parse the input URL by doing the following:

    0. Use strchr to check for/remove (TODO: handle) "http://" or "https://"
    1. Use strchr to find the first slash in the URL 
    2. Set the path pointer to 1 character after the spot returned by strchr.
    3. Overwrite the slash with a '\0' so that we are no longer considering anything after the slash.
    4. Use strchr to find the first colon in the URL.
    5. Set the port pointer to 1 character after the spot returned by strchr.
    6. Overwrite the colon with a '\0' so that we are just left with the hostname.
  */
  // determine if "//" was included
  char *dbl_slash = strstr(hostname, "//");
  if (dbl_slash)
  {
    // TODO: other stuff with http/https info?
    // move hostname pointer to start after final "/"
    hostname = dbl_slash + 2;
    printf("HERE: %s", hostname);
  }
  // find the first slash in the URL (beyond http...)
  char *first_slash = strchr(hostname, *"/");
  path = first_slash + 1;
  *first_slash = *"\0";
  // see if port is specified
  char *first_colon = strchr(hostname, *":");
  if (first_colon)
  {
    // found a colon, port should be right after
    port = first_colon + 1;
    *first_colon = *"\0";
  }
  else
  {
    // no colon, assume port 80
    port = "80";
  }

  urlinfo->hostname = hostname;
  urlinfo->port = port;
  urlinfo->path = path;

  return urlinfo;
}

/**
 * Constructs and sends an HTTP request
 *
 * fd:       The file descriptor of the connection.
 * hostname: The hostname string.
 * port:     The port string.
 * path:     The path string.
 *
 * Return the value from the send() function.
*/
int send_request(int fd, char *hostname, char *port, char *path)
{
  const int max_request_size = 16384;
  char request[max_request_size];
  int rv;

  // construct request
  int request_length =
      sprintf(request,
              "GET /%s HTTP/1.1\r\n"
              "Host: %s:%s\r\n"
              "Connection: close\r\n"
              "\r\n",
              path, hostname, port);
  printf("----------\nrequest: \n----------\n%s----------\n", request);
  // send it!
  rv = send(fd, request, request_length, 0);

  return rv;
}

int main(int argc, char *argv[])
{
  int sockfd, numbytes;
  char buf[BUFSIZE];

  if (argc != 2)
  {
    fprintf(stderr, "usage: client HOSTNAME:PORT/PATH\n");
    exit(1);
  }

  /*
    1. Parse the input URL
    2. Initialize a socket by calling the `get_socket` function from lib.c
    3. Call `send_request` to construct the request and send it
    4. Call `recv` in a loop until there is no more data to receive from the server. Print the received response to stdout.
    5. Clean up any allocated memory and open file descriptors.
  */

  // parse URL
  // note: malloc's a urlinfo_t
  urlinfo_t *parsed_url = parse_url(argv[1]);
  printf("hostname: %s \nport: %s\npath: %s\n", parsed_url->hostname, parsed_url->port, parsed_url->path);

  // initialize a socket
  sockfd = get_socket(parsed_url->hostname, parsed_url->port);
  if (sockfd > 0)
  {
    // call send_request (constructs and send the request);
    int rv = send_request(sockfd, parsed_url->hostname, parsed_url->port, parsed_url->path);

    if (rv > 0)
    {
      printf("response: \n----------\n");
      while ((numbytes = recv(sockfd, buf, BUFSIZE - 1, 0)) > 0)
      {
        printf("%s", buf);
      }
      printf("\n----------\n");
    }
  }

  free(parsed_url);

  return 0;
}
