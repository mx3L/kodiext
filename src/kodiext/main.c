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
  OP_CODE_SWITCH_TO_ENIGMA2,
  OP_CODE_SWITCH_TO_KODI,
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
    case OP_CODE_SWITCH_TO_ENIGMA2:
      opcode_str = "OP_CODE_SWITCH_TO_ENIGMA2";
      break;
    case OP_CODE_SWITCH_TO_KODI:
      opcode_str = "OP_CODE_SWITCH_TO_KODI";
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

static void send_message(struct packet_header *ph, char *datain, char **dataout)
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

  //fprintf(stderr,"sent %s\n", packet_to_str(ph, packet_str));
  Writen(sockfd, ph , sizeof(*ph));
  Writen(sockfd, datain , ph->length);
  Readn(sockfd, ph, sizeof(*ph));
  if (ph->length > 0 && dataout != NULL)
  {
    *dataout = (char *) malloc(ph->length * sizeof(char) + 1);
    Readn(sockfd, *dataout, ph->length);
    *(*dataout + ph->length) = '\0';
  }
  //fprintf(stderr, "received %s\n", packet_to_str(ph, packet_str));

  if (close(sockfd) < 0)
    err_sys("close_error");
}


int main(int argc, char **argv)
{
  char *purl = NULL;
  char *surl = NULL;
  char *pid = NULL;
  int stop = 0;
  int tokodi = 0;
  int toenigma2 = 0;
  int c;
  opterr = 0;

  while ((c = getopt (argc, argv, "U:P:S:TEK")) != -1)
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
      case 'E':
        toenigma2 = 1;
        break;
      case 'K':
        tokodi = 1;
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

  if (!(stop || ((tokodi || toenigma2) && pid != NULL) || (purl != NULL && pid != NULL)))
  {
    fprintf(stderr, "Usage: kodiext -U playurl -P ppid [-S subtitlesurl] [-T] [-E] [-K]\n");
    return 1;
  }

  fprintf(stderr, "playurl = %s, subtitlesurl = %s, stop = %d, toenigma2 = %d, tokodi = %d\n", purl, surl, stop, toenigma2, tokodi);

  struct packet_header ph;
  char configcmd[64];
  char *data = NULL;

  if (stop)
  {
    ph.opcode = OP_CODE_EXIT;
    ph.result = 0;
    ph.length = 0;
    send_message(&ph, data, NULL);
    if (!ph.result)
    {
      fprintf(stderr, "cannot exit!\n");
      return 2;
    }
    return 0;
  }

  if (toenigma2)
  {
    ph.opcode = OP_CODE_SWITCH_TO_ENIGMA2;
    ph.result = 0;
    ph.length = 0;
    send_message(&ph, data, NULL);
    if (!ph.result)
    {
      fprintf(stderr, "cannot switch to enigma2!\n");
      return 2;
    }
    sprintf(configcmd, "config -pid %s -visible off", pid);
    system(configcmd);

    return 0;
  }

  if (tokodi)
  {
    ph.opcode = OP_CODE_SWITCH_TO_KODI;
    ph.result = 0;
    ph.length = 0;
    send_message(&ph, data, NULL);
    if (!ph.result)
    {
      fprintf(stderr, "cannot switch to kodi!\n");
      return 2;
    }
    sprintf(configcmd, "config -pid %s -visible on", pid);
    system(configcmd);
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
  send_message(&ph, data, NULL);

  if (surl != NULL)
    free(data);

  if (!ph.result)
  {
    fprintf(stderr, "player didn't start!\n");
    return 3;
  }

  sprintf(configcmd, "config -pid %s -visible off", pid);
  system(configcmd);
  system("touch /tmp/playing.lock 2>/dev/null");

  char *dataout = NULL;
  while(ph.result)
  {
    sleep(1);

    ph.opcode = OP_CODE_PLAY_STATUS;
    ph.result = 0;
    ph.length = 0;
    send_message(&ph, data, &dataout);
    if (dataout != NULL)
    {
      fputs(dataout, stdout);
      fputc('\n', stdout);
      fflush(stdout);
      free(dataout);
      dataout = NULL;
    }
  }

  ph.opcode = OP_CODE_PLAY_STOP;
  ph.result = 0;
  ph.length = 0;
  send_message(&ph, NULL, NULL);

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
