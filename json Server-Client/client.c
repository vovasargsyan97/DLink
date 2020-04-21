#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define PORT 8090
#define true 1

// Driver code
int main ()
{
  struct sockaddr_in address;
  int sock = 0, valread;
  struct sockaddr_in serv_addr;
  char str[100];
  const char *inp;
  char buffer[1024] = { 0 };
  FILE *fp;
	//char buffer[1024];
	struct json_object *parsed_json;
	struct json_object *input;
	size_t i;

	fp = fopen("input.json","r");
	fread(buffer, 1024, 1, fp);
	fclose(fp);

	parsed_json = json_tokener_parse(buffer);
	
	json_object_object_get_ex(parsed_json, "input", &input);
	
	inp = json_object_get_string(input);

// Creating socket file descriptor
  if ((sock = socket (AF_INET, SOCK_STREAM, 0)) < 0)
    {
      printf ("\n Socket creation error \n");
      return -1;
    }

  memset (&serv_addr, '0', sizeof (serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons (PORT);

// Convert IPv4 and IPv6 addresses from
// text to binary form 127.0.0.2 is local
// host IP address, this address should be
// your system local host IP address
  if (inet_pton (AF_INET, "127.0.0.3", &serv_addr.sin_addr) <= 0)
    {
      printf ("\nAddress not supported \n");
      return -1;
    }

// connect the socket
  if (connect (sock, (struct sockaddr *) &serv_addr, sizeof (serv_addr)) < 0)
    {
      printf ("\nConnection Failed \n");
      return -1;
    }
while(true){
int l = strlen (inp);

// send string to server side
  send (sock, inp, sizeof (inp), 0);

// read string sent by server
  int *x = &valread;
  read (sock, x, l);

  printf ("%d", valread);
  printf ("\n");
}
  return 0;
}

