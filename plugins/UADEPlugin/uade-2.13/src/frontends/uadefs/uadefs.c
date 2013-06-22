/*
 * uadefs decodes Amiga songs transparently into WAV files
 *
 * The code was forked from fusexmp example.
 *
 * Copyright (C) 2001-2007  Miklos Szeredi <miklos@szeredi.hu>
 * Copyright (C) 2008       Heikki Orsila <heikki.orsila@iki.fi>
 * 
 * This program can be distributed under the terms of the GNU GPL.
 * See the file COPYING.
 */

#define _GNU_SOURCE

#define FUSE_USE_VERSION 26

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef linux
/* For pread()/pwrite() */
#define _XOPEN_SOURCE 500
#endif

#include <pthread.h>
#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>
#include <sys/time.h>
#ifdef HAVE_SETXATTR
#include <sys/xattr.h>
#endif
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <assert.h>
#include <time.h>

#include "uadeconf.h"
#include "uadestate.h"
#include "eagleplayer.h"
#include "songdb.h"
#include "uadeconfig.h"
#include "ossupport.h"


#define WAV_HEADER_LEN 44

#define CACHE_BLOCK_SHIFT 12  /* 4096 bytes per cache block */
#define CACHE_BLOCK_SIZE (1 << CACHE_BLOCK_SHIFT)
#define CACHE_LSB_MASK (CACHE_BLOCK_SIZE - 1)
#define CACHE_SECONDS 512
#define SND_PER_SECOND (44100 * 4)

#define NSTASHES 4
#define STASH_CACHE_BLOCKS 2
#define STASH_SIZE (CACHE_BLOCK_SIZE * STASH_CACHE_BLOCKS)
#define STASH_TIME 30

#define DEBUG(fmt, args...) if (debugmode) { fprintf(stderr, fmt, ## args); }

#define LOG(fmt, args...) if (debugfd != -1) { \
        char debugmsg[4096]; \
        int debuglen; \
        DEBUG(fmt, ## args); \
        debuglen = snprintf(debugmsg, sizeof debugmsg, fmt, ## args); \
        xwrite(debugfd, debugmsg, debuglen); \
    }

#define DIE(fmt, args...) do {fprintf(stderr, fmt, ## args); exit(1); } while (0)

#define LOGDIE(fmt, args...) do {LOG(fmt, ## args); abort();} while (0)

#define MAX(x, y) (x >= y) ? (x) : (y)
#define MIN(x, y) (x <= y) ? (x) : (y)


struct cacheblock {
	unsigned int bytes; /* 0 <= bytes <= CACHE_BLOCK_SIZE */
	void *data;
};

struct sndctx {
	int normalfile;       /* if non-zero, the file is not decoded */
	int pipefd;           /* pipefd from which to read sound data */
	pid_t pid;            /* pid of the decoding process */
	char fname[PATH_MAX]; /* filename of the song being played */

	size_t nblocks;
	size_t end_bi;
	struct cacheblock *blocks;
};

struct stash {
	/* Add time invalidation */
	char fname[PATH_MAX];          /* File name for the stash */
	char data[STASH_SIZE];
	time_t created;               /* Timestamp for validity checks */
};


static char *srcdir = NULL;
static int debugfd = -1;
static int debugmode;
static struct uade_state uadestate;
static time_t mtime = 0;
static pthread_mutex_t readmutex = PTHREAD_MUTEX_INITIALIZER;

int nextstash;
struct stash stashes[NSTASHES];


static ssize_t get_file_size(const char *path);

/*
 * xread() is the same as the read(), but it automatically restarts read()
 * operations with a recoverable error (EAGAIN and EINTR). xread()
 * DOES NOT GUARANTEE that "len" bytes is read even if the data is available.
 */
static ssize_t xread(int fd, void *buf, size_t count)
{
	ssize_t nr;
	while (1) {
		nr = read(fd, buf, count);
		if ((nr < 0) && (errno == EAGAIN || errno == EINTR))
			continue;
		return nr;
	}
}

static ssize_t xwrite(int fd, const void *buf, size_t count)
{
	ssize_t nr;
	while (1) {
		nr = write(fd, buf, count);
		if ((nr < 0) && (errno == EAGAIN || errno == EINTR))
			continue;
		return nr;
	}
}

ssize_t read_in_full(int fd, void *buf, size_t count)
{
	char *p = buf;
	ssize_t total = 0;

	while (count > 0) {
		ssize_t loaded = xread(fd, p, count);
		if (loaded <= 0)
			return total ? total : loaded;
		count -= loaded;
		p += loaded;
		total += loaded;
	}

	return total;
}

static char *uadefs_get_path(int *isuade, const char *path)
{
	char *realpath;
	char *sep;
	struct stat st;

	if (isuade)
		*isuade = 0;

	if (asprintf(&realpath, "%s%s", srcdir, path) < 0)
		LOGDIE("No memory for path name: %s\n", path);

	if (!lstat(realpath, &st))
		goto out;

	/* File doesn't exist */
	sep = strrchr(realpath, '.');
	if (sep == NULL || strcmp(sep, ".wav") != 0)
		goto out;

	*sep = 0;

	if (uade_is_our_file(realpath, 1, &uadestate)) {
		if (isuade)
			*isuade = 1;
	} else {
		/* Not an UADE file -> restore .wav postfix */
		*sep = '.';
	}
out:
	return realpath;
}

static int spawn_uade(struct sndctx *ctx)
{
	int fds[2];

	LOG("Spawn UADE %s\n", ctx->fname);

	if (pipe(fds)) {
		LOG("Can not create a pipe\n");
		return -errno;
	}

	ctx->pid = fork();
	if (ctx->pid == 0) {
		char *argv[] = {"uade123", "-c", "-k0", "--stderr", "-v",
				ctx->fname, NULL};
		int fd;

		close(0);
		close(2);
		close(fds[0]);

		fd = open("/dev/null", O_RDWR);
		if (fd < 0)
			LOGDIE("Can not open /dev/null\n");

		dup2(fd, 0);
		dup2(fds[1], 1);
		dup2(fd, 2);

		DEBUG("Execute %s\n", UADENAME);

		execv(UADENAME, argv);

		LOGDIE("Could not execute %s\n", UADENAME);
	} else if (ctx->pid == -1) {
		LOG("Can not fork\n");
		close(fds[0]);
		close(fds[1]);
		return -errno;
	}

	ctx->pipefd = fds[0];
	close(fds[1]);

	return 0;
}

static ssize_t cache_block_read(struct sndctx *ctx, char *buf, size_t offset,
				size_t size)
{
	size_t toread;
	size_t offset_bi;
	size_t lsb;
	struct cacheblock *cb;

	offset_bi = offset >> CACHE_BLOCK_SHIFT;

	if (offset_bi >= ctx->nblocks) {
		LOG("Too much sound data: %zu >= %zu: %s\n", offset_bi, ctx->nblocks, ctx->fname);
		return 0;
	}

	cb = &ctx->blocks[offset_bi];

	if (!cb->bytes)
		return -1;

	lsb = offset & CACHE_LSB_MASK;

	if ((lsb + size) > CACHE_BLOCK_SIZE)
		LOGDIE("lsb + size (%zd) failed: %zd %zd\n", lsb + size, offset, size);

	if (lsb >= cb->bytes)
		return 0;

	toread = MIN(size, cb->bytes - lsb);

	memcpy(buf, ((char *) cb->data) + lsb, toread);

	return toread;
}

static void cache_init(struct sndctx *ctx)
{
	ctx->end_bi = 0;
	ctx->nblocks = (SND_PER_SECOND * CACHE_SECONDS + CACHE_BLOCK_SIZE - 1) >> CACHE_BLOCK_SHIFT;
	ctx->blocks = calloc(1, ctx->nblocks * sizeof(ctx->blocks[0]));
	if (ctx->blocks == NULL)
		LOGDIE("No memory for cache\n");
}

static size_t cache_read(struct sndctx *ctx, char *buf, size_t offset,
			 size_t size)
{
	size_t offset_bi;
	struct cacheblock *cb;
	ssize_t res;

	offset_bi = offset >> CACHE_BLOCK_SHIFT;

	if (offset_bi >= STASH_CACHE_BLOCKS && ctx->pid == -1) {
		char buf[CACHE_BLOCK_SIZE];
		int i;

		if (spawn_uade(ctx))
			return 0;

		for (i = 0; i < STASH_CACHE_BLOCKS; i++) {
			res = read_in_full(ctx->pipefd, buf, sizeof buf);
			if (res < sizeof buf)
				return 0;
		}
	}

	/* The requested block is already in cache, copy it directly */
	res = cache_block_read(ctx, buf, offset, size);
	if (res >= 0)
		return (size_t) res;

	if (offset_bi >= ctx->nblocks) {
		LOG("Too much sound data: %s\n", ctx->fname);
		return 0;
	}

	/*
	 * Read cache blocks in sequence until the requested cache block
	 * has been read. ctx->end_bi is increased every time a new block
	 * is read. ctx->end_bi points to the first block that is not cached.
	 */
	while (ctx->end_bi <= offset_bi) {
		cb = &ctx->blocks[ctx->end_bi];

		cb->data = malloc(CACHE_BLOCK_SIZE);
		if (cb->data == NULL) {
			LOG("Out of memory: %s\n", ctx->fname);
			break;
		}

		res = read_in_full(ctx->pipefd, cb->data, CACHE_BLOCK_SIZE);
		if (res <= 0) {
			free(cb->data);
			cb->data = NULL;
			DEBUG("Read code %d at %zd: %s\n", (int) res, ctx->end_bi << CACHE_BLOCK_SHIFT, ctx->fname);
			break;
		}

		cb->bytes = res;

		ctx->end_bi++;

		if (res < CACHE_BLOCK_SIZE)
			break;
	}

	res = cache_block_read(ctx, buf, offset, size);
	if (res >= 0)
		return (size_t) res;

	return 0;
}

int cache_prefill(struct sndctx *ctx, char *data)
{
	int i;

	if (data == NULL)
		LOGDIE("Prefill segfault: %s\n", ctx->fname);

	ctx->end_bi = STASH_CACHE_BLOCKS;

	for (i = 0; i < STASH_CACHE_BLOCKS; i++) {
		ctx->blocks[i].data = malloc(CACHE_BLOCK_SIZE);
		if (ctx->blocks[i].data == NULL) {
			LOG("Prefill OOM: %s\n", ctx->fname);
			break;
		}
		ctx->blocks[i].bytes = CACHE_BLOCK_SIZE;
		memcpy(ctx->blocks[i].data, data, CACHE_BLOCK_SIZE);
		data += CACHE_BLOCK_SIZE;
	}

	if (i < STASH_CACHE_BLOCKS) {
		/* Free cache blocks from the beginning to the first NULL */
		for (i = 0; i < STASH_CACHE_BLOCKS; i++) {
			if (ctx->blocks[i].data == NULL)
				break;
			free(ctx->blocks[i].data);
			ctx->blocks[i].data = NULL;
		}
		return -1;
	}

	return 0;
}

static void write_le_32(char *data, int32_t v)
{
	data[0] = v         & 0xff;
	data[1] = (v >> 8)  & 0xff;
	data[2] = (v >> 16) & 0xff;
	data[3] = (v >> 24) & 0xff;
}

static void cache_invasive_write(struct sndctx *ctx, size_t offset,
				 char *data, size_t len)
{
	size_t offset_bi = offset >> CACHE_BLOCK_SHIFT;
	size_t lsbpos = offset & CACHE_LSB_MASK;
	struct cacheblock *cb;

	if (offset_bi >= ctx->end_bi) {
		LOG("Invasive cache write: %zu\n", offset);
		return;
	}

	cb = &ctx->blocks[offset_bi];

	if ((lsbpos + len) <= cb->bytes)
		memcpy(((char *) cb->data) + lsbpos, data, len);
}

static struct sndctx *create_ctx(const char *path)
{
	struct sndctx *ctx;

	ctx = calloc(1, sizeof ctx[0]);
	if (ctx == NULL)
		return NULL;

	strlcpy(ctx->fname, path, sizeof ctx->fname);

	ctx->pipefd = -1;
	ctx->pid = -1;

	return ctx;
}

static void destroy_cache(struct sndctx *ctx)
{
	size_t cbi;

	if (ctx->blocks != NULL) {
		for (cbi = 0; cbi < ctx->nblocks; cbi++) {
			ctx->blocks[cbi].bytes = 0;
			free(ctx->blocks[cbi].data);
			ctx->blocks[cbi].data = NULL;
		}

		free(ctx->blocks);
		ctx->blocks = NULL;
		ctx->end_bi = 0;
		ctx->nblocks = 0;
	}
}

static void kill_child(struct sndctx *ctx)
{
	if (ctx->pid != -1) {
		kill(ctx->pid, SIGINT);
		while (waitpid(ctx->pid, NULL, 0) <= 0);
		ctx->pid = -1;
	}
}

static void set_no_snd_file(struct sndctx *ctx)
{
	ctx->normalfile = 1;

	destroy_cache(ctx);

	kill_child(ctx);

	close(ctx->pipefd);
	ctx->pipefd = -1;
}

static void destroy_ctx(struct sndctx *ctx)
{
	ctx->fname[0] = 0;

	if (ctx->normalfile == 0)
		set_no_snd_file(ctx);

	free(ctx);
}

static inline struct sndctx *get_uadefs_file(struct fuse_file_info *fi)
{
	return (struct sndctx *) (uintptr_t) fi->fh;
}

static int check_stash(const char *fname, struct stash *stash, time_t t)
{
	if (strcmp(fname, stash->fname) != 0)
		return 0;

	/* Reject old stashes */
	if (t >= (stash->created + STASH_TIME))
		return 0;

	return 1;
}

int warm_up_cache(struct sndctx *ctx)
{
	char crapbuf[STASH_SIZE];
	ssize_t s;
	int i;
	struct stash *stash;
	size_t offs;
	time_t created;

	created = time(NULL);
	if (created == ((time_t) -1)) {
		LOG("Clock failed\n");
		created = 0;
	}

	for (i = 0; i < NSTASHES; i++) {
		if (check_stash(ctx->fname, &stashes[i], created)) {
			LOG("Found stash for %s\n", ctx->fname);
			if (cache_prefill(ctx, stashes[i].data))
				return -EIO;
			break;
		}
	}

	/* Start uade iff no stash found */
	if (i == NSTASHES) {
		int ret = spawn_uade(ctx);
		if (ret)
			return ret;
	}

	for (offs = 0; offs < sizeof crapbuf; offs += CACHE_BLOCK_SIZE) {
		if (cache_read(ctx, &crapbuf[offs], offs, CACHE_BLOCK_SIZE) < CACHE_BLOCK_SIZE) {
			DEBUG("File is not playable: %s\n", ctx->fname);
			set_no_snd_file(ctx);
			return -EIO;
		}
	}

	s = get_file_size(ctx->fname);
	if (s > 0 &&
	    memcmp(&crapbuf[0], "RIFF", 4) == 0 &&
	    memcmp(&crapbuf[8], "WAVE", 4) == 0    ) {
		/* Fix WAV header */
		char data[4];

		write_le_32(data, (int32_t) (s - 8));
		cache_invasive_write(ctx, 4, data, 4);

		write_le_32(data, (int32_t) (s - 44));
		cache_invasive_write(ctx, 40, data, 4);
	}

	if (i == NSTASHES) {
		/* We found no stash -> create one from crapbuf */
		stash = &stashes[nextstash++];
		if (nextstash >= NSTASHES)
			nextstash = 0;

		if (sizeof(stash->data) != sizeof(crapbuf))
			LOGDIE("Stash data != crapbuf\n");

		stash->created = created;
		strlcpy(stash->fname, ctx->fname, sizeof stash->fname);
		memcpy(stash->data, crapbuf, sizeof stash->data);

		LOG("Allocated stash for %s\n", ctx->fname);
	}

	return 0;
}

static struct sndctx *open_file(int *success, const char *path, int isuade)
{
	int ret;
	struct sndctx *ctx;
	struct stat st;

	ctx = create_ctx(path);
	if (ctx == NULL) {
		ret = -ENOMEM;
		goto err;
	}

	if (stat(path, &st)) {
		ret = -errno;
		goto err;
	}

	if (!S_ISREG(st.st_mode) || !isuade) {
		set_no_snd_file(ctx);
		goto out;
	}

	cache_init(ctx);

	ret = warm_up_cache(ctx);
	if (ret < 0)
		goto err;
 out:
	*success = 0;
	return ctx;

 err:
	if (ctx)
		destroy_ctx(ctx);

	*success = ret;
	return NULL;
}

static void load_content_db(void)
{
	struct stat st;
	char name[PATH_MAX] = "";
	char *home;
	int ret;

	home = getenv("HOME");
	if (home)
		snprintf(name, sizeof name, "%s/.uade2/contentdb", home);

	/* User database has priority over global database, so we read it
	 * first */
	if (name[0]) {
		if (stat(name, &st) == 0) {
			if (mtime < st.st_mtime) {
				ret = uade_read_content_db(name);
				if (stat(name, &st) == 0)
					mtime = st.st_mtime;
				if (ret)
					return;
			}
		} else {
			FILE *f = fopen(name, "w");
			if (f)
				fclose(f);
			uade_read_content_db(name);
		}
	}

	snprintf(name, sizeof name, "%s/contentdb.conf", uadestate.config.basedir.name);
	if (stat(name, &st) == 0 && mtime < st.st_mtime) {
		uade_read_content_db(name);
		if (stat(name, &st) == 0)
			mtime = st.st_mtime;
	}
}

/*
 * If the file is an uade song, return a heuristic wav file size, a positive
 * integer. Otherwise, return zero.
 */
static ssize_t get_file_size(const char *path)
{
	int64_t msecs;

	if (!uade_is_our_file(path, 1, &uadestate))
		return 0;

	/*
	 * HACK HACK. Use playlength stored in the content database
	 * or lie about the time.
	 */
	load_content_db();
	if (!uade_alloc_song(&uadestate, path))
		return -1;

	msecs = uadestate.song->playtime;
	uade_unalloc_song(&uadestate);

	if (msecs > 3600000)
		return -1;

	if (msecs <= 0)
		msecs = 1000 * CACHE_SECONDS;

	return WAV_HEADER_LEN + (((msecs * SND_PER_SECOND) / 1000) & ~0x3);
}

static int uadefs_getattr(const char *fpath, struct stat *stbuf)
{
	int res;
	char *path = uadefs_get_path(NULL, fpath);
	ssize_t s;

	res = lstat(path, stbuf);
	if (res == -1) {
		free(path);
		return -errno;
	}

	s = get_file_size(path);

	free(path);
	path = NULL;

	if (s > 0) {
		stbuf->st_size = s; /* replace the lstat() value */
		stbuf->st_blocks = stbuf->st_size / 512;
	} else if (s < 0) {
		return -ENOMEM;
	}
	/* For s == 0, the result of lstat() is returned */
	return 0;
}

static int uadefs_access(const char *fpath, int mask)
{
	int res;
	char *path = uadefs_get_path(NULL, fpath);

	res = access(path, mask);

	free(path);

	if (res == -1)
		return -errno;

	return 0;
}

static int uadefs_readlink(const char *fpath, char *buf, size_t size)
{
	int res;
	char *path = uadefs_get_path(NULL, fpath);

	res = readlink(path, buf, size - 1);

	free(path);

	if (res == -1)
		return -errno;

	buf[res] = '\0';
	return 0;
}


static void gen_uade_name(char *name, size_t maxname, mode_t mode,
			  const char *dirname, const char *filename)
{
	char fullname[PATH_MAX];

	snprintf(name, maxname, "%s", filename);

	if (!S_ISREG(mode))
		return;

	snprintf(fullname, sizeof fullname, "%s/%s", dirname, filename);

	if (!uade_is_our_file(fullname, 1, &uadestate))
		return;

	snprintf(name, maxname, "%s.wav", filename);
}


static int uadefs_readdir(const char *fpath, void *buf, fuse_fill_dir_t filler,
			  off_t offset, struct fuse_file_info *fi)
{
	DIR *dp;
	struct dirent *de;
	char *path = uadefs_get_path(NULL, fpath);
	char name[256];
	char fullname[PATH_MAX];

	(void) offset;
	(void) fi;

	dp = opendir(path);

	if (dp == NULL) {
		free(path);
		return -errno;
	}

	while ((de = readdir(dp)) != NULL) {
		struct stat st;
		memset(&st, 0, sizeof(st));
		st.st_ino = de->d_ino;
		st.st_mode = de->d_type << 12;
		if (st.st_mode == 0) {
			/* de->d_type not supported -> use stat() */
			struct stat oldst;
			snprintf(fullname, sizeof fullname, "%s/%s", path, de->d_name);
			if (!stat(fullname, &oldst))
				st.st_mode = oldst.st_mode & ~0777;
		}

		gen_uade_name(name, sizeof name, st.st_mode, path, de->d_name);

		if (filler(buf, name, &st, 0))
			break;
	}

	free(path);
	closedir(dp);
	return 0;
}

static int uadefs_mknod(const char *fpath, mode_t mode, dev_t rdev)
{
	(void) mode;
	(void) fpath;
	(void) rdev;
	return -EACCES;
}

static int uadefs_mkdir(const char *fpath, mode_t mode)
{
	int res;
	char *path = uadefs_get_path(NULL, fpath);

	res = mkdir(path, mode);

	free(path);

	if (res == -1)
		return -errno;

	return 0;
}

static int uadefs_unlink(const char *fpath)
{
	int res;
	char *path = uadefs_get_path(NULL, fpath);

	res = unlink(path);

	free(path);

	if (res == -1)
		return -errno;

	return 0;
}

static int uadefs_rmdir(const char *fpath)
{
	int res;
	char *path = uadefs_get_path(NULL, fpath);

	res = rmdir(path);

	free(path);

	if (res == -1)
		return -errno;

	return 0;
}

static int uadefs_symlink(const char *ffrom, const char *fto)
{
	int res;
	char *from = uadefs_get_path(NULL, ffrom);
	char *to = uadefs_get_path(NULL, fto);

	res = symlink(from, to);

	free(from);
	free(to);

	if (res == -1)
		return -errno;

	return 0;
}

static int uadefs_rename(const char *ffrom, const char *fto)
{
	int res;
	char *from = uadefs_get_path(NULL, ffrom);
	char *to = uadefs_get_path(NULL, fto);

	res = rename(from, to);

	free(from);
	free(to);

	if (res == -1)
		return -errno;

	return 0;
}

static int uadefs_link(const char *ffrom, const char *fto)
{
	int res;
	char *from = uadefs_get_path(NULL, ffrom);
	char *to = uadefs_get_path(NULL, fto);

	res = link(from, to);

	free(from);
	free(to);

	if (res == -1)
		return -errno;

	return 0;
}

static int uadefs_chmod(const char *fpath, mode_t mode)
{
	int res;
	char *path = uadefs_get_path(NULL, fpath);

	res = chmod(path, mode);

	free(path);

	if (res == -1)
		return -errno;

	return 0;
}

static int uadefs_chown(const char *fpath, uid_t uid, gid_t gid)
{
	int res;
	char *path = uadefs_get_path(NULL, fpath);

	res = lchown(path, uid, gid);

	free(path);

	if (res == -1)
		return -errno;

	return 0;
}

static int uadefs_truncate(const char *fpath, off_t size)
{
	(void) fpath;
	(void) size;

	return -EIO;
}

static int uadefs_utimens(const char *fpath, const struct timespec ts[2])
{
	int res;
	struct timeval tv[2];
	char *path = uadefs_get_path(NULL, fpath);

	tv[0].tv_sec = ts[0].tv_sec;
	tv[0].tv_usec = ts[0].tv_nsec / 1000;
	tv[1].tv_sec = ts[1].tv_sec;
	tv[1].tv_usec = ts[1].tv_nsec / 1000;

	res = utimes(path, tv);

	free(path);

	if (res == -1)
		return -errno;

	return 0;
}

static int uadefs_open(const char *fpath, struct fuse_file_info *fi)
{
	int ret;
	struct sndctx *ctx;
	int isuade;
	char *path = uadefs_get_path(&isuade, fpath);

	if (fi->flags & O_CREAT) {
		free(path);
		return -EPERM;
	}

	ctx = open_file(&ret, path, isuade);

	free(path);

	if (ctx == NULL)
		return ret;

	fi->direct_io = 1;
	fi->fh = (uint64_t) (uintptr_t) ctx;

	DEBUG("Opened %s as %s file\n", ctx->fname, ctx->normalfile ? "normal" : "UADE");
	return 0;
}

static int uadefs_read(const char *fpath, char *buf, size_t size, off_t off,
		       struct fuse_file_info *fi)
{
	int fd;
	size_t res;
	struct sndctx *ctx = get_uadefs_file(fi);
	ssize_t totalread = 0;
	size_t bsize;

	if (ctx->normalfile) {
		char *path = uadefs_get_path(NULL, fpath);

		fd = open(path, O_RDONLY);

		free(path);

		if (fd == -1)
			return -errno;

		totalread = pread(fd, buf, size, off);
		if (totalread == -1)
			totalread = -errno;

		close(fd);

		return totalread;
	}

	pthread_mutex_lock(&readmutex);

	while (size > 0) {
		bsize = MIN(CACHE_BLOCK_SIZE - (off & CACHE_LSB_MASK), size);
		res = cache_read(ctx, buf, off, bsize);
		if (res == 0)
			break;

		totalread += res;
		buf += res;
		off += res;
		size -= res;
	}

	DEBUG("read() returns %zd\n", totalread);
	pthread_mutex_unlock(&readmutex);

	return totalread;
}

static int uadefs_write(const char *fpath, const char *buf, size_t size,
			off_t offset, struct fuse_file_info *fi)
{
	(void) fpath;
	(void) buf;
	(void) size;
	(void) offset;
	(void) fi;

	return -EIO;
}

static int uadefs_statfs(const char *fpath, struct statvfs *stbuf)
{
	int res;
	char *path = uadefs_get_path(NULL, fpath);

	res = statvfs(path, stbuf);

	free(path);

	if (res == -1)
		return -errno;

	return 0;
}

static int uadefs_flush(const char *fpath, struct fuse_file_info *fi)
{
	(void) fi;
	(void) fpath;
	return 0;
}

static int uadefs_release(const char *fpath, struct fuse_file_info *fi)
{
	destroy_ctx(get_uadefs_file(fi));

	DEBUG("release %s\n", fpath);

	return 0;
}

static int uadefs_fsync(const char *fpath, int isdatasync,
		     struct fuse_file_info *fi)
{
	/* Just a stub.	 This method is optional and can safely be left
	   unimplemented */

	(void) fpath;
	(void) isdatasync;
	(void) fi;
	return 0;
}

#ifdef HAVE_SETXATTR
/* xattr operations are optional and can safely be left unimplemented */
static int uadefs_setxattr(const char *fpath, const char *name, const char *value,
			size_t size, int flags)
{
	char *path = uadefs_get_path(NULL, fpath);
	int res;

	res = lsetxattr(path, name, value, size, flags);

	free(path);

	if (res == -1)
		return -errno;
	return 0;
}

static int uadefs_getxattr(const char *fpath, const char *name, char *value,
			size_t size)
{
	char *path = uadefs_get_path(NULL, fpath);
	int res;

	res = lgetxattr(path, name, value, size);

	free(path);

	if (res == -1)
		return -errno;
	return res;
}

static int uadefs_listxattr(const char *fpath, char *list, size_t size)
{
	char *path = uadefs_get_path(NULL, fpath);
	int res;

	res = llistxattr(path, list, size);

	free(path);

	if (res == -1)
		return -errno;
	return res;
}

static int uadefs_removexattr(const char *fpath, const char *name)
{
	char *path = uadefs_get_path(NULL, fpath);
	int res;

	res = lremovexattr(path, name);

	free(path);

	if (res == -1)
		return -errno;
	return 0;
}
#endif /* HAVE_SETXATTR */

static struct fuse_operations uadefs_oper = {
	.getattr	= uadefs_getattr,
	.access		= uadefs_access,
	.readlink	= uadefs_readlink,
	.readdir	= uadefs_readdir,
	.mknod		= uadefs_mknod,
	.mkdir		= uadefs_mkdir,
	.symlink	= uadefs_symlink,
	.unlink		= uadefs_unlink,
	.rmdir		= uadefs_rmdir,
	.rename		= uadefs_rename,
	.link		= uadefs_link,
	.chmod		= uadefs_chmod,
	.chown		= uadefs_chown,
	.truncate	= uadefs_truncate,
	.utimens	= uadefs_utimens,
	.open		= uadefs_open,
	.read		= uadefs_read,
	.write		= uadefs_write,
	.statfs		= uadefs_statfs,
	.flush          = uadefs_flush,
	.release	= uadefs_release,
	.fsync		= uadefs_fsync,
#ifdef HAVE_SETXATTR
	.setxattr	= uadefs_setxattr,
	.getxattr	= uadefs_getxattr,
	.listxattr	= uadefs_listxattr,
	.removexattr	= uadefs_removexattr,
#endif
};

static void usage(const char *progname)
{
	fprintf(stderr,
"usage: %s musicdir mountpoint [options]\n"
"\n"
"general options:\n"
"    -o opt,[opt...]        mount options\n"
"    -h   --help            print help\n"
"    -V   --version         print version\n"
"\n", progname);
}

enum {
	KEY_HELP,
	KEY_VERSION,
	KEY_FOREGROUND,
};

static struct fuse_opt uadefs_opts[] = {
	FUSE_OPT_KEY("-V",             KEY_VERSION),
	FUSE_OPT_KEY("--version",      KEY_VERSION),
	FUSE_OPT_KEY("-h",             KEY_HELP),
	FUSE_OPT_KEY("--help",         KEY_HELP),
	FUSE_OPT_KEY("debug",          KEY_FOREGROUND),
	FUSE_OPT_KEY("-d",             KEY_FOREGROUND),
	FUSE_OPT_KEY("-f",             KEY_FOREGROUND),
	FUSE_OPT_END
};

static int uadefs_fuse_main(struct fuse_args *args)
{
#if FUSE_VERSION >= 26
	return fuse_main(args->argc, args->argv, &uadefs_oper, NULL);
#else
	return fuse_main(args->argc, args->argv, &uadefs_oper);
#endif
}

static int uadefs_opt_proc(void *data, const char *arg, int key,
			   struct fuse_args *outargs)
{
	(void) data;
	char dname[4096];

	switch (key) {
	case FUSE_OPT_KEY_OPT:
		return 1;

	case FUSE_OPT_KEY_NONOPT:
		if (!srcdir) {
			if (arg[0] == '/') {
				srcdir = strdup(arg);
				if (srcdir == NULL)
					DIE("No memory for srcdir\n");
			} else {
				if (getcwd(dname, sizeof dname) == NULL)
					DIE("getcwd() failed\n");

				if (asprintf(&srcdir, "%s/%s", dname, arg) == -1)
					DIE("asprintf() failed\n");
			}

			while (1) {
				size_t l = strlen(srcdir);

				if (l == 1 && srcdir[0] == '/')
					break;

				if (srcdir[l - 1] != '/')
					break;

				srcdir[l - 1] = 0;
			}

			return 0;
		}
		return 1;

	case KEY_HELP:
		usage(outargs->argv[0]);
		fuse_opt_add_arg(outargs, "-ho");
		uadefs_fuse_main(outargs);
		exit(1);

	case KEY_VERSION:
		fprintf(stderr, "uadefs version %s\n", UADE_VERSION);
#if FUSE_VERSION >= 25
		fuse_opt_add_arg(outargs, "--version");
		uadefs_fuse_main(outargs);
#endif
		exit(0);

	case KEY_FOREGROUND:
		debugmode = 1;
		return 1;

	default:
		fprintf(stderr, "internal error\n");
		abort();
	}

	return 0;
}

static void init_uade(void)
{
    char uadeconfname[4096];

    (void) uade_load_initial_config(uadeconfname, sizeof uadeconfname,
				    &uadestate.config, NULL);

    load_content_db();
}


int main(int argc, char *argv[])
{
	struct fuse_args args = FUSE_ARGS_INIT(argc, argv);

	if (fuse_opt_parse(&args, NULL, uadefs_opts, uadefs_opt_proc) == -1)
		exit(1);

	DEBUG("srcdir: %s\n", srcdir);

	if (getenv("HOME")) {
		int flags = O_WRONLY | O_TRUNC | O_APPEND | O_CREAT;
		int fmode = S_IRUSR | S_IWUSR;
		char logfname[4096];

		snprintf(logfname, sizeof logfname, "%s/.uade2/uadefs.log", getenv("HOME"));
		debugfd = open(logfname, flags, fmode);
	}

	init_uade();

	umask(0);
	return uadefs_fuse_main(&args);
}
