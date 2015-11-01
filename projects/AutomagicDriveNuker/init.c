/* Automagic USB Nuker
 * Copyright (C) 2013 Calvin Owens
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of version 2 of the GNU General Public License as published
 * by the Free Software Foundation.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE COPYRIGHT HOLDER(S) OR AUTHOR(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THIS SOFTWARE. */

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <stdint.h>
#include <inttypes.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/times.h>
#include <sys/mman.h>
#include <time.h>
#include <linux/reboot.h>
#include <signal.h>
#include <sys/syscall.h>

/* The GPLv2 disclaimer */
#include "static.h"

#define fatal(args...) \
do { \
	printf(args); \
	exit(-1); \
} while (0)

#ifdef DEBUG
#define debug(args...) \
do { \
	printf(args); \
} while (0)
#define PROGRESS_LIST_DELAY 30
#else
#define debug(args...) \
do { \
} while (0)
#define PROGRESS_LIST_DELAY 1
#endif

#define CHUNK_SIZE 1048576
#define MAX_DEVS 16

struct block_dev {
	int fd;
	char *name;
	char *num;
	char *model;
	uint64_t size;
};

void *ecalloc(int num, int size)
{
	debug("Allocating %i blocks of %i bytes (%i): ", num, size, num * size);
	void *ret = calloc(num, size);
	debug("%p\n", ret);
	
	if (ret == NULL)
		fatal("Out of memory - is MAX_DEVS too high? (%i)\n", MAX_DEVS);

	return ret;
}

void efree(void *ptr)
{
	debug("Freeing memory at %p\n", ptr);
	free(ptr);
}

void do_mounts(void)
{
	if(mount("none", "/proc", "proc", NULL, NULL) == -1)
		goto err;
	if(mount("none", "/sys", "sysfs", NULL, NULL) == -1)
		goto err;
	if(mount("none", "/dev", "devtmpfs", NULL, NULL) == -1)
		goto err;
	return;
err:
	fatal("Couldn't mount necessary things: %s\nThis should never happen\n", 
							strerror(errno));
}

dev_t makedev_s(char *devnum)
{
	int maj, min;
	
	char *cmin = index(devnum, (int) ':') + 1;
	*(cmin - 1) = '\0';
	
	maj = atoi(devnum);
	min = atoi(cmin);

	*(cmin - 1) = ':';
	return makedev(maj, min);
}

/* You can pass the same pointer as both arguments */
void read_sysfs_file(char **dst, char *filename)
{
	/* Truncate to 30 characters. Get rid of the \n */
	char *file_contents = ecalloc(1, 31);
	int fd, ret;
	debug("Reading file %s (%p) into memory %p\n", filename, filename, dst);	
	fd = open(filename, O_RDONLY);
	if(fd < 0)
		goto err;
	
	ret = read(fd, file_contents, 30);
	if(ret <= 0)
		goto err;
	
	file_contents[ret - 1] = '\0';
	debug("Read %s in read_sysfs_file\n", file_contents);
out:	
	if(*dst == filename)
		efree(filename);
	close(fd);
	*dst = file_contents;
	return;
err:
	strcpy(file_contents, "(error)");
	goto out;
}

/* Because I'm too lazy to fuck with <stdarg.h> */
#define fread_sysfs_file(pointer, format, args...) \
do { \
	if (asprintf(&pointer, format, args) == -1) \
		fatal("-ENOMEM returned from asprintf\n"); \
	read_sysfs_file(&pointer, pointer); \
} while (0)

uint64_t read_dev_size(char *devname)
{
	char *asc_len = NULL;
	fread_sysfs_file(asc_len, "/sys/block/%s/size", devname);
	uint64_t ret = (uint64_t) atoll(asc_len);
	return ret;
}

int get_dev_fd(char *devnum)
{
	char *fname = NULL;
	int ret;

	debug("Getting dev fd for %s\n",devnum);
	if (asprintf(&fname, "/block/%s", devnum) == -1)
		fatal("-ENOMEM returned from asprintf at (__LINE__)\n");
	if (mknod(fname, S_IFBLK|0666, makedev_s(devnum)) == -1)
		fatal("Error creating %s: %s", fname, strerror(errno));
	ret = open(fname, O_WRONLY|O_SYNC);
	if (ret == -1)
		fatal("Error opening %s: %s", fname, strerror(errno));
	efree(fname);
	return ret;
}

int is_implied(struct dirent *dirp)
{
	if ((!strcmp((const char *) &dirp->d_name, ".")) || 
				(!strcmp((const char *) &dirp->d_name, "..")))
		return 1;
	return 0;
}

int fill_dev(struct block_dev *dev, DIR *sdir)
{
	struct dirent *dirp;
again:
	dirp = readdir(sdir);
	if (!dirp)
		return 0;
	if (is_implied(dirp))
		goto again;
	
	dev->name = ecalloc(1, strlen(dirp->d_name));
	strcpy(dev->name, dirp->d_name);
	fread_sysfs_file(dev->num, "/sys/block/%s/dev", dirp->d_name);
	fread_sysfs_file(dev->model, "/sys/block/%s/device/model", 
							dirp->d_name);
	dev->size = read_dev_size(dirp->d_name);
	dev->fd = get_dev_fd(dev->num);
	debug("Filled %s\n", dev->name);
	return 1;
}

int get_devs(struct block_dev **devs)
{
	*devs = ecalloc(MAX_DEVS, sizeof(**devs));
	struct block_dev *dev_arr = *devs;	
	DIR *sdir = opendir("/sys/block");
	int n = 0;

	while(fill_dev(&dev_arr[n++], sdir) && n < MAX_DEVS)
		;
	
	return n;
}

void print_pretty_dev_list(struct block_dev *d, int ndevs)
{
	int i = 0;
	debug("In print_pretty_dev_list\n");
	while(i < ndevs) {
		printf("\t%i) %s [%s] %" PRIu64 "\"%s\"\n", i + 1, d[i].name, 
					d[i].num, d[i].size, d[i].model);
		i++;
	}
}

int devs_done_zeroing(struct block_dev *d, uint64_t *tp, int n)
{
	while(n) {
		if(d[n - 1].size != tp[n - 1] && tp[n - 1] != (uint64_t) -1)
			return 0;
		n--;
	}
	
	return 1;
}

void zero_dev(struct block_dev* dev, uint64_t* shr_wrt_prg)
{
	char *zeroblock = ecalloc(1, CHUNK_SIZE);
	int ret;

	while(*shr_wrt_prg < dev->size) {
		ret = write(dev->fd, zeroblock, CHUNK_SIZE);
		if(ret <= 0)
			goto err;
		*shr_wrt_prg += ret;
	}

	exit(0);
err:
	*shr_wrt_prg = (uint64_t) -1;
	debug("ERROR writing to %s (%s)\n", dev->name, strerror(errno));
	exit(-1);
}

int main(void)
{
	struct block_dev *devs;
	int ndevs, ret, i;
	uint64_t *thread_progress;
	char input[5];
	
	sigblock(sigmask(SIGCHLD));
	setbuf(stdout, NULL); /* Don't buffer printf() output */
	do_mounts();
	
	printf("%s\n\n", gretting_message);
	
	printf("Polling for devices...\n");
	ndevs = get_devs(&devs);
	if(!ndevs)
		fatal("No devices were found!");
	printf("Found %i devices:\n", ndevs);
	print_pretty_dev_list(devs, ndevs);
	
	input[4] = '\0';
	while(1) {
		printf("Type \"NUKE\" and press [ENTER] to begin: ");
		ret = read(0, input, 4);
		if(!strcmp(input, "NUKE"))
			break;
	}
	
	thread_progress = mmap(NULL, ndevs << 3, PROT_READ|PROT_WRITE, 
						MAP_SHARED|MAP_ANONYMOUS, 0, 0);
	if(thread_progress == MAP_FAILED)
		fatal("mmap() failed: %s\n", strerror(errno));
	
	/* Fork off for each device 
	 * Maybe consider only doing NR_CPUS at a time? */
	i = 0;
	while(i < ndevs) {
		ret = fork();
		if(!ret)
			zero_dev(&devs[i], thread_progress + (i << 3)); /* Doesn't return */
		if(ret == -1)
			fatal("fork() #%i failed: %s", i, strerror(errno));
		debug("Forked %s to PID %i\n", devs[i].name, ret);
	}
	
	/* Now the threads are chugging along. */
	while(!devs_done_zeroing(devs, thread_progress, ndevs)) {
		i = 0;
		while(i < ndevs)
			printf("%s: %" PRIu64 "/%" PRIu64 "\n", devs[i].name, 
					thread_progress[i], devs[i++].size);
		sleep(PROGRESS_LIST_DELAY);
		printf("\n");
	}
	
	printf("\nAutomagic nukification accomplished\n");
	ret = reboot(LINUX_REBOOT_CMD_POWER_OFF);
	if(ret == -1)
		fatal("COULDN'T REBOOT: %s", strerror(errno));
	
	return -2;
}
