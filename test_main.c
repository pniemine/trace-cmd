/*
 * Copyright (C) 2012 Pauli Nieminen <suokkos@gmail.com>
 *
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License (not later!)
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 */
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>

#include <dirent.h>

#define _SIZE_T_DECLARED
#include <glob.h>

static int open_temp(void)
{
	char template[] = "/data/testXXXXXX";
	int r = -1;
	do {
		strcpy(template,
#ifndef ANDROID
				"/tmp/testXXXXXX"
#else
				"/data/testXXXXXX"
#endif
		      );
		r = mkstemp(template);
	} while(r < 0 && errno == EEXIST);

	if (r < 0) {
		perror("mkstemp");
		return -errno;
	}
	if (unlink(template) < 0)
		perror("unlink temp");

	return r;
}

static ssize_t read_all(int fd, char *buf, ssize_t nr)
{
	ssize_t r = 0, re = 0;
	do {
		r = read(fd, buf + re, nr - re);
		if (r < 0 && errno != EINTR) {
			perror("read");
			return -1;
		}
		re += r;
	} while(re < nr);
	return nr;
}

static ssize_t write_all(int fd, char *buf, ssize_t nr)
{
	ssize_t r, w = 0;
	do {
		r = write(fd, buf + w, nr - w);
		if (r < 0 && errno != EINTR) {
			perror("write");
			return -1;
		}
		w += r;
	} while(w < nr);
	return nr;
}

static ssize_t splice_all(int fd_in, int pipe[2], int fd_out, ssize_t nr)
{
	ssize_t r, re = 0, we = 0;
	do {
		int r = splice(fd_in, NULL, pipe[1], NULL, nr - re, 1);
		if (r < 0) {
			perror("splice");
			return -1;
		}
		re += r;

		r = splice(pipe[0], NULL, fd_out, NULL, nr - we, 1);
		if (r < 0) {
			perror("splice");
			return -1;
		}
		we += r;
	} while (we < nr);

	return nr;
}

static int fdcmp(int fd, char *buf, off_t len)
{
	void *mem;
	int r;
	if (ftruncate(fd, len) < 0) {
		perror("ftruncate");
		return -errno;
	}

	mem = mmap(NULL, len, PROT_READ, MAP_PRIVATE, fd, 0);
	if (mem == MAP_FAILED) {
		perror("mmap");
		return -errno;
	}

	r = memcmp(mem, buf, len);

	munmap(mem, len);
	return r;
}

#define ARR_SIZE(a) (sizeof (a)/sizeof (a)[0])

static int test_splice(void)
{
	char buffer[50*4096];
	struct {
		int fd_in;
		int pipe[2];
		int fd_out;
	} state = {.fd_in = 0};
	int r = 0;

	r = pipe(state.pipe);

	if (r < 0) {
		r = errno;
		perror("pipe");
	}

	state.fd_out = open("/dev/urandom", O_RDONLY);

	if (state.fd_out < 0) {
		perror("open /dev/urandom");
		r = errno;
		goto out;
	}

	r = open_temp();
	state.fd_in = r;
	if (r < 0)
		goto out;

	if (read_all(state.fd_out, buffer, ARR_SIZE(buffer)) < 0)
		goto out;

	if (write_all(state.fd_in, buffer, ARR_SIZE(buffer)) < 0)
		goto out;

	if (lseek(state.fd_in, 0, SEEK_SET) == (off_t)-1) {
		perror("lseek");
		r = errno;
		goto out;
	}

	close(state.fd_out);
	r = open_temp();
	state.fd_out = r;
	if (r < 0)
		goto out;

	if (splice_all(state.fd_in, state.pipe, state.fd_out, ARR_SIZE(buffer)) < 0)
		goto out;

	if (fdcmp(state.fd_out, buffer, ARR_SIZE(buffer)) != 0) {
		printf("Data in fd_out isn't same as reference data.\n");
		r = EFAULT;
		goto out;
	}

	printf("splice test passed\n");
out:
	close(state.fd_in);
	close(state.fd_out);
	close(state.pipe[0]);
	close(state.pipe[1]);
	return r;
}

static int test_glob(void)
{
	int i = 0;
	glob_t g = {.gl_offs = 0,};
	char path[] = "/sys/class/tty/*/power/wakeup";
	DIR *list;
	struct dirent *entity;
	int matched = 0;

	if ((i = glob(path, 0, NULL, &g)) != 0) {
		fprintf(stderr, "glob %d failed\n", i);
		return EINVAL;
	}

	list = opendir("/sys/class/tty/");

	while((entity = readdir(list))) {
		int found = 0;
		if (strcmp(entity->d_name, ".") == 0 ||
		   	strcmp(entity->d_name, "..") == 0)
			continue;
		for (i = 0; g.gl_pathv[i]; i++) {
			if (strncmp(entity->d_name, g.gl_pathv[i] + 
					strlen("/sys/class/tty/"),
					strlen(entity->d_name)) == 0) {
				found = 1;
				matched++;
				break;
			}
		}
		if (!found) {
			printf("Non matched dir %s\n", entity->d_name);
			return -EINVAL;
		}
	}

	if (matched != g.gl_pathc) {
		printf("matched %d, found %d\n", matched, g.gl_pathc);
		return -EINVAL;
	}
	closedir(list);
	globfree(&g);

	printf("glob test passed\n");
	return 0;
}

int main(void)
{
	int r = 0;
	r |= test_splice();
	r |= test_glob();
	return r;
}
