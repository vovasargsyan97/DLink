#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define PORT 8090

int
strToInt (char *buf)
{

  int length = strlen (buf);
  for (int i = 0; i < length; i++)
    {
      if (buf[i] >= 'A' && buf[i] <= 'z')
	{
	  perror ("Invalid String!");
	  exit (1);
	}
    }
  char symbols[length];
  int numbers[length];

  int length_sim = 0, length_num = 0;
  int i = 0;
  int resoult = 0;
  while (i < length)
    {
      if (buf[i] >= '0' && buf[i] <= '9')
	{
	  int number = 0;
	  while (i < length && buf[i] >= '0' && buf[i] <= '9')
	    {
	      char sim[1];
	      sim[0] = buf[i];
	      number = number * 10 + atoi (sim);
	      i++;
	    }
	  numbers[length_num++] = number;

	}
      else if (buf[i] == '(')
	{
	  char str[length];
	  int j = 0, count_op = 1, count_cl = 0;;
	  i++;
	  while (count_op != count_cl)
	    {
	      str[j] = buf[i];
	      if (buf[i] == '(')
		count_op++;
	      if (buf[i] == ')')
		count_cl++;
	      i++;
	      j++;
	    }
	  str[j - 1] = '\0';
	  numbers[length_num++] = strToInt (str);
	}
      else
	{
	  symbols[length_sim++] = buf[i++];
	}

    }
  int t = 1;
  while (t == 1)
    {
      int b = 0;
      for (int k = 0; k < length_sim; k++)
	{
	  if (symbols[k] == '*' || symbols[k] == '/')
	    {
	      if (symbols[k] == '*')
		{
		  numbers[k] = numbers[k] * numbers[k + 1];
		}
	      if (symbols[k] == '/')
		{
		  numbers[k] = numbers[k] / numbers[k + 1];
		}
	      for (int j = k + 1; j < length_num - 1; j++)
		numbers[j] = numbers[j + 1];
	      length_num--;
	      for (int j = k; j < length_sim - 1; j++)
		symbols[j] = symbols[j + 1];

	      length_sim--;
	      b = 1;
	      break;
	    }
	}
      t = b;
    }
  t = 1;
  while (t == 1)
    {
      int b = 0;
      for (int k = 0; k < length_sim; k++)
	{
	  if (symbols[k] == '+' || symbols[k] == '-')
	    {
	      if (symbols[k] == '+')
		{
		  numbers[k] = numbers[k] + numbers[k + 1];
		}
	      if (symbols[k] == '-')
		{
		  numbers[k] = numbers[k] - numbers[k + 1];
		}
	      for (int j = k + 1; j < length_num - 1; j++)
		numbers[j] = numbers[j + 1];
	      length_num--;
	      for (int j = k; j < length_sim - 1; j++)
		symbols[j] = symbols[j + 1];
	      length_sim--;
	      b = 1;
	      break;
	    }
	}
      t = b;
    }
  resoult = numbers[0];
  return resoult;


}

// Driver code
int
main ()
{
  int server_fd, new_socket, valread;
  struct sockaddr_in address;
  char str[100];
  int addrlen = sizeof (address);
  char buffer[1024] = { 0 };
  char *hello = "Hello from server";

// Creating socket file descriptor
  if ((server_fd = socket (AF_INET, SOCK_STREAM, 0)) == 0)
    {
      perror ("socket failed");
      exit (EXIT_FAILURE);
    }

  address.sin_family = AF_INET;
  address.sin_addr.s_addr = inet_addr ("127.0.0.3");
  address.sin_port = htons (PORT);

// Forcefully attaching socket to
// the port 8090
  if (bind (server_fd, (struct sockaddr *) &address, sizeof (address)) < 0)
    {
      perror ("bind failed");
      exit (EXIT_FAILURE);
    }

// puts the server socket in passive mode
  if (listen (server_fd, 3) < 0)
    {
      perror ("listen");
      exit (EXIT_FAILURE);
    }
  if ((new_socket =
       accept (server_fd, (struct sockaddr *) &address,
	       (socklen_t *) & addrlen)) < 0)
    {
      perror ("accept");
      exit (EXIT_FAILURE);
    }

// read string send by client
  valread = read (new_socket, str, sizeof (str));

  printf ("\nString sent by client:%s\n", str);

// set q equal to result
  int x = strToInt (str);
  int *temp = &x;
  printf ("%d", *temp);

// send reversed string to client
// by send system call
  send (new_socket, temp, sizeof (temp), 0);
  printf ("\nModified string sent to client\n");

  return 0;
}

