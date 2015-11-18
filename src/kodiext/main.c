#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <time.h>
#include <getopt.h>

#include "common.h"
#include "packet.h"

enum
{
  OP_CODE_EXIT,
  OP_CODE_PLAY,
  OP_CODE_PLAY_STATUS,
  OP_CODE_PLAY_STOP,
  OP_CODE_END,
};

static const char *opcode_to_str(int opcode)
{
  const char *opcode_str = NULL;
  switch (opcode)
  {
    case OP_CODE_EXIT:
      opcode_str = "OP_CODE_EXIT";
      break;
    case OP_CODE_PLAY:
      opcode_str = "OP_CODE_PLAY";
      break;
    case OP_CODE_PLAY_STATUS:
      opcode_str = "OP_CODE_PLAY_STATUS";
      break;
    case OP_CODE_PLAY_STOP:
      opcode_str = "OP_CODE_PLAY_STOP";
      break;
    case OP_CODE_END:
      opcode_str = "OP_CODE_END";
      break;
    default:
      opcode_str = "OP_CODE_UKNOWN";
      break;
  }
  return opcode_str;
}

static char *packet_to_str(const struct packet_header *ph, char *packet_str)
{
  const char *opcode_str = opcode_to_str(ph->opcode);
  const char *result_str = ph->result ? "OK" : "NOK";
  sprintf(packet_str, "ph.opcode = %s, ph.result = %s, ph.length = %d", opcode_str, result_str, ph->length);
  return packet_str;
}

static void send_message(struct packet_header *ph, char *data)
{
  char packet_str[128];
  int sockfd;
  struct sockaddr_un servaddr;

  if ((sockfd = socket(AF_LOCAL, SOCK_STREAM, 0)) < 0)
    err_sys("socket_error");

  bzero(&servaddr, sizeof(servaddr));
  servaddr.sun_family = AF_LOCAL;
  strcpy(servaddr.sun_path, "/tmp/kodiext.socket");

  if (connect(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0)
    err_sys("connect_error");

  printf("sent %s\n", packet_to_str(ph, packet_str));
  Writen(sockfd, ph , sizeof(*ph));
  Writen(sockfd, data , ph->length);
  Readn(sockfd, ph, sizeof(*ph));
  printf("received %s\n", packet_to_str(ph, packet_str));

  if (close(sockfd) < 0)
    err_sys("close_error");
}


int main(int argc, char **argv)
{
  char *purl = NULL;
  char *surl = NULL;
  char *pid = NULL;
  int stop = 0;
  int c;
  opterr = 0;

  while ((c = getopt (argc, argv, "U:P:S:T")) != -1)
    switch (c)
      {
      case 'U':
        purl = optarg;
        break;
      case 'S':
        surl = optarg;
        break;
      case 'T':
        stop = 1;
        break;
      case 'P':
        pid = optarg;
        break;
      case '?':
        if (optopt == 'U')
          fprintf (stderr, "Option -%c requires an argument.\n", optopt);
        else if (isprint (optopt))
          fprintf (stderr, "Unknown option `-%c'.\n", optopt);
        else
          fprintf (stderr,
                   "Unknown option character `\\x%x'.\n",
                   optopt);
        return 1;
      default:
        abort ();
      }

  if (!stop && (purl == NULL || pid == NULL))
  {
    printf("Usage: kodiext -U playurl -P ppid [-S subtitlesurl] [-T]\n");
    return 1;
  }

  printf("playurl = %s, subtitlesurl = %s, stop = %d\n", purl, surl, stop);

  struct packet_header ph;
  char *data = NULL;

  if (stop)
  {
    ph.opcode = OP_CODE_EXIT;
    ph.result = 0;
    ph.length = 0;
    send_message(&ph, data);
    if (!ph.result)
    {
      fprintf(stderr, "cannot exit!\n");
      return 2;
    }
    return 0;
  }

  ph.opcode = OP_CODE_PLAY;
  ph.result = 0;
  if (surl != NULL)
  {
    data = (char *) malloc((strlen(purl) + strlen(surl) + 2) * sizeof(char));
    sprintf(data, "%s\n%s", purl, surl);
  }
  else
    data = purl;
  ph.length = strlen(data);
  send_message(&ph, data);

  if (surl != NULL)
    free(data);

  if (!ph.result)
  {
    fprintf(stderr, "player didn't start!\n");
    return 3;
  }

  char configcmd[64];
  sprintf(configcmd, "config -pid %s -visible off", pid);
  system(configcmd);
  system("touch /tmp/playing.lock 2>/dev/null");
  while(ph.result)
  {
    sleep(1);
    ph.opcode = OP_CODE_PLAY_STATUS;
    ph.result = 0;
    ph.length = 0;
    send_message(&ph, NULL);
  }

  ph.opcode = OP_CODE_PLAY_STOP;
  ph.result = 0;
  ph.length = 0;
  send_message(&ph, NULL);

  if (ph.opcode == OP_CODE_PLAY_STOP && !ph.result)
  {
    fprintf(stderr, "cannot stop player!\n");
    return 4;
  }

  sprintf(configcmd, "config -pid %s -visible on", pid);
  system(configcmd);
  system("rm /tmp/playing.lock 2>/dev/null");
  return 0;
}
