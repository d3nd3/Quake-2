#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <linux/soundcard.h>
#include <stdio.h>

#include "../client/client.h"
#include "../client/snd_loc.h"

int audio_fd;
int snd_inited;

cvar_t *sndbits;
cvar_t *sndspeed;
cvar_t *sndchannels;
cvar_t *snddevice;

#include <stdint.h>

uint8_t to_big_endian(uint8_t value) {
    uint8_t result = 0;
    int i;

    for (i = 0; i < 8; i++) {
        result |= ((value >> i) & 1) << (7 - i);
    }

    return result;
}

static int tryrates[] = { 11025, 22051, 44100, 8000 };
struct audio_buf_info info;
qboolean SNDDMA_Init(void)
{

	int rc;
    int fmt;
	int tmp;
    int i;
    char *s;
	
	int caps;
	extern uid_t saved_euid;

	if (snd_inited)
		return;

	if (!snddevice) {
		sndbits = Cvar_Get("sndbits", "16", CVAR_ARCHIVE);
		sndspeed = Cvar_Get("sndspeed", "0", CVAR_ARCHIVE);
		sndchannels = Cvar_Get("sndchannels", "2", CVAR_ARCHIVE);
		snddevice = Cvar_Get("snddevice", "/dev/dsp", CVAR_ARCHIVE);
	}

// open /dev/dsp, confirm capability to mmap, and get size of dma buffer

	if (!audio_fd) {
		seteuid(saved_euid);

		audio_fd = open(snddevice->string, O_RDWR);

		seteuid(getuid());

		if (audio_fd < 0)
		{
			perror(snddevice->string);
			Com_Printf("Could not open %s\n", snddevice->string);
			return 0;
		}
	}

    rc = ioctl(audio_fd, SNDCTL_DSP_RESET, 0);
    if (rc < 0)
	{
		perror(snddevice->string);
		Com_Printf("Could not reset %s\n", snddevice->string);
		close(audio_fd);
		return 0;
	}

	if (ioctl(audio_fd, SNDCTL_DSP_GETCAPS, &caps)==-1)
	{
		perror(snddevice->string);
        Com_Printf("Sound driver too old\n");
		close(audio_fd);
		return 0;
	}

/*
	if (!(caps & DSP_CAP_TRIGGER) || !(caps & DSP_CAP_MMAP))
	{
		Com_Printf("Sorry but your soundcard can't do this\n");
		close(audio_fd);
		return 0;
	}
*/

    if (ioctl(audio_fd, SNDCTL_DSP_GETOSPACE, &info)==-1)
    {   
        perror("GETOSPACE");
		Com_Printf("Um, can't do GETOSPACE?\n");
		close(audio_fd);
		return 0;
    }

    Com_Printf("bytes: %i\n",info.bytes);
    Com_Printf("fragments: %i\n",info.fragments);
    Com_Printf("fragstotal: %i\n",info.fragstotal);
    Com_Printf("fragsize: %i\n\n",info.fragsize);


/*
	if fragment size is lowered, application respond quicker
*/
    
    // http://manuals.opensound.com/developer/SNDCTL_DSP_SETFRAGMENT.html

    // lower this the better
    int frag_size = 8;
    int max_fragments = info.bytes/frag_size;

    frag_size = (int)round(log2f(info.bytes/max_fragments));
 	int fragments = (max_fragments << 16) | (frag_size);

    if (ioctl(audio_fd, SNDCTL_DSP_SETFRAGMENT, &fragments) < 0) {
        perror("Error setting sound parameters");
        return -1;
    }

    if (ioctl(audio_fd, SNDCTL_DSP_GETOSPACE, &info)==-1)
    {   
        perror("GETOSPACE");
		Com_Printf("Um, can't do GETOSPACE?\n");
		close(audio_fd);
		return 0;
    }

    Com_Printf("bytes: %i\n",info.bytes);
    Com_Printf("fragments: %i\n",info.fragments);
    Com_Printf("fragstotal: %i\n",info.fragstotal);
    Com_Printf("fragsize: %i\n",info.fragsize);
    
// set sample bits & speed

    dma.samplebits = (int)sndbits->value;
	if (dma.samplebits != 16 && dma.samplebits != 8)
    {
        ioctl(audio_fd, SNDCTL_DSP_GETFMTS, &fmt);
        if (fmt & AFMT_S16_LE) dma.samplebits = 16;
        else if (fmt & AFMT_U8) dma.samplebits = 8;
    }

	dma.speed = (int)sndspeed->value;
	if (!dma.speed) {
        for (i=0 ; i<sizeof(tryrates)/4 ; i++)
            if (!ioctl(audio_fd, SNDCTL_DSP_SPEED, &tryrates[i])) break;
        dma.speed = tryrates[i];
    }

	dma.channels = (int)sndchannels->value;
	if (dma.channels < 1 || dma.channels > 2)
		dma.channels = 2;
	
	dma.samples = info.bytes / (dma.samplebits/8);
	dma.submission_chunk = 1;

// memory map the dma buffer

	// dma.samples * dma.samplebits/8
	// MAP_FILE
	if (!dma.buffer)
		dma.buffer = (unsigned char *) mmap(NULL, info.bytes, PROT_WRITE, MAP_ANONYMOUS|MAP_SHARED, -1, 0);
	if (!dma.buffer)
	{
		perror(snddevice->string);
		Com_Printf("Could not mmap %s\n", snddevice->string);
		close(audio_fd);
		return 0;
	}

	tmp = 0;
	if (dma.channels == 2)
		tmp = 1;
    rc = ioctl(audio_fd, SNDCTL_DSP_STEREO, &tmp);
    if (rc < 0)
    {
		perror(snddevice->string);
        Com_Printf("Could not set %s to stereo=%d", snddevice->string, dma.channels);
		close(audio_fd);
        return 0;
    }
	if (tmp)
		dma.channels = 2;
	else
		dma.channels = 1;

	Com_Printf("speed before : %i\n",dma.speed);
    rc = ioctl(audio_fd, SNDCTL_DSP_SPEED, &dma.speed);
    if (rc < 0)
    {
		perror(snddevice->string);
        Com_Printf("Could not set %s speed to %d", snddevice->string, dma.speed);
		close(audio_fd);
        return 0;
    }

    Com_Printf("speed after : %i\n",dma.speed);

    if (dma.samplebits == 16)
    {
        rc = AFMT_S16_LE;
        rc = ioctl(audio_fd, SNDCTL_DSP_SETFMT, &rc);
        if (rc < 0)
		{
			perror(snddevice->string);
			Com_Printf("Could not support 16-bit data.  Try 8-bit.\n");
			close(audio_fd);
			return 0;
		}
    }
    else if (dma.samplebits == 8)
    {
        rc = AFMT_U8;
        rc = ioctl(audio_fd, SNDCTL_DSP_SETFMT, &rc);
        if (rc < 0)
		{
			perror(snddevice->string);
			Com_Printf("Could not support 8-bit data.\n");
			close(audio_fd);
			return 0;
		}
    }
	else
	{
		perror(snddevice->string);
		Com_Printf("%d-bit sound not supported.", dma.samplebits);
		close(audio_fd);
		return 0;
	}

// toggle the trigger & start her up

    // tmp = 0;
    // rc  = ioctl(audio_fd, SNDCTL_DSP_SETTRIGGER, &tmp);
	// if (rc < 0)
	// {
	// 	perror(snddevice->string);
	// 	Com_Printf("Could not toggle.\n");
	// 	close(audio_fd);
	// 	return 0;
	// }
    // tmp = PCM_ENABLE_OUTPUT;
    // rc = ioctl(audio_fd, SNDCTL_DSP_SETTRIGGER, &tmp);
	// if (rc < 0)
	// {
	// 	perror(snddevice->string);
	// 	Com_Printf("Could not toggle.\n");
	// 	close(audio_fd);
	// 	return 0;
	// }

	dma.samplepos = 0;

	snd_inited = 1;
	return 1;

}

int SNDDMA_GetDMAPos(void)
{

	struct count_info count;

	if (!snd_inited) return 0;

	if (ioctl(audio_fd, SNDCTL_DSP_GETOPTR, &count)==-1)
	{
		perror(snddevice->string);
		Com_Printf("Uh, sound dead.\n");
		close(audio_fd);
		snd_inited = 0;
		return 0;
	}
//	dma.samplepos = (count.bytes / (dma.samplebits / 8)) & (dma.samples-1);
//	fprintf(stderr, "%d    \r", count.ptr);
	dma.samplepos = count.ptr / (dma.samplebits / 8);

	return dma.samplepos;

}

void SNDDMA_Shutdown(void)
{
	if (snd_inited)
	{
		// close(audio_fd);
		snd_inited = 0;
	}
}



/*
Why can't we write more directly, when dma.buffer is written to.
like in client/snd_mix.c?
	void S_TransferStereo16 (unsigned long *pbuf, int endtime)
*/
extern int soundtime;

/*
==============
SNDDMA_Submit

Send sound to device if buffer isn't really the dma buffer
===============
*/
void SNDDMA_Submit(void)
{
	// Com_Printf("SNDDMA_Submit\n");
	// check if audio_fd is open
	/*int flags = fcntl(audio_fd, F_GETFL);
	if (flags == -1) {
	   perror("fcntl");
	   return 1;
	}
	if (flags & O_RDONLY) {
	   Com_Printf("File descriptor is open for reading\n");
	} else if (flags & O_WRONLY) {
	   Com_Printf("File descriptor is open for writing\n");
	} else if (flags & O_RDWR) {
	   Com_Printf("File descriptor is open for reading and writing\n");
	} else {
	   Com_Printf("File descriptor is not open\n");
	}*/

	if (ioctl(audio_fd, SNDCTL_DSP_GETOSPACE, &info)==-1)
    {   
        perror("GETOSPACE");
		Com_Printf("Um, can't do GETOSPACE?\n");
		close(audio_fd);
		Sys_Error(ERR_FATAL,"Fail SNDCTL_DSP_GETOSPACE\n");
    }

    // Don't write if no fragments
    if ( !(info.fragments > 0) ) return;

	int write_count = 0;
	int totalToWrite = info.bytes;
	int num_written = 0;
	// int chunksize = info.fragsize;
	int chunksize = info.bytes;
	while (num_written < totalToWrite) {
	    int remaining_bytes = totalToWrite - num_written;
	    int result = write(audio_fd, dma.buffer + num_written, chunksize);
	    write_count +=1;
	    if (result == -1) {
	        perror("write");
	        exit(EXIT_FAILURE);
	    } else {
	    	
	        num_written += result;
	    }
	}
	// Com_Printf("Write Count = %i\n",write_count);

/*
	int dbfd = open("dmabuffer.txt", O_WRONLY | O_CREAT, 0666);
	if (dbfd == -1) {
	    perror("open");
	    exit(1);
	}

	ssize_t written = 0;
	ssize_t remaining = info.bytes;
	while (remaining > 0) {
	    written = write(dbfd, dma.buffer, remaining);
	    if (written == -1) {
	        perror("write");
	        close(dbfd);
	        exit(1);
	    }
	    remaining -= written;
	}

	if (close(dbfd) == -1) {
	    perror("close");
	    exit(1);
	}
*/
}

void SNDDMA_BeginPainting (void)
{
	/*
	fd_set write_fds;
    struct timeval timeout;

	FD_ZERO(&write_fds);
	FD_SET(audio_fd, &write_fds);

	while (1) {
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;
        int num_ready = select(audio_fd+1, NULL, &write_fds, NULL, &timeout);
        if (num_ready == -1) {
            perror("select");
            exit(EXIT_FAILURE);
        } else if (num_ready == 0) {
            continue;
        }
    }
    */
}

