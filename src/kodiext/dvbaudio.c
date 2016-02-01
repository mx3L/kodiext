#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/dvb/audio.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>

int audioSetBypassToRaw()
{
	int fd;
	if ((fd = open("/dev/dvb/adapter0/audio0", O_RDWR)) < 0)
	{
		perror("failed to open audio device:");
		return -1;
	}
	if (ioctl(fd, AUDIO_SET_BYPASS_MODE, 0x30) < 0)
	{
		perror("AUDIO_SET_BYPASS_MODE to 0x30, error:");
	}
	if (close(fd) < 0)
	{
		perror("failed to close audio device:");
		return -1;
	}
	return 0;
}
