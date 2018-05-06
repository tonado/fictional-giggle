/*
 * channel.c
 *
 *  Created on: May 5, 2018
 *      Author: crh
 */

#include <channel.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/time.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>

#define LOGE(fmt, ...) printf("errno[%d], " fmt "\n", errno, ##__VA_ARGS__)

#define CHANNEL_MAGIC (0xeaeaeaea)
#define SUN_PATH_LEN (108)

enum channel_status
{
	status_idle,
	status_connected,
	status_listened,
};

typedef struct channel
{
	unsigned magic;
	int fd;
	enum channel_status st;
	char sun_path[SUN_PATH_LEN];
}channel;

typedef struct rpc_msg_hdr
{
	unsigned long transid;
	unsigned msglen;
	int msg_type; //0-req, 1-rsp
}rpc_msg_hdr;

static int check_channel(channel_t chn)
{
	channel *c = (channel*)chn;
	if (c && c->magic == CHANNEL_MAGIC)
	{
		return 1;
	}
	return 0;
}

int channel_create(channel_t *chn)
{
	static unsigned i = 0;
	struct sockaddr_un un;
	int fd = socket(AF_UNIX, SOCK_STREAM, 0);
	if (fd < 0)
	{
		LOGE("socket fail.");
		return -1;
	}

	un.sun_family = AF_UNIX;
	sprintf(un.sun_path, "/var/tmp/%d-%d", getpid(), __sync_fetch_and_add(&i, 1));

	unlink(un.sun_path);
	if (bind(fd, (struct sockaddr *)&un, sizeof(un)) < 0)
	{
		LOGE("bind fail.");
		close(fd);
		return -1;
	}

	channel *c = (channel*)malloc(sizeof(channel));
	if (c == 0)
	{
		LOGE("malloc fail.");
		close(fd);
		return -1;
	}

	c->magic = CHANNEL_MAGIC;
	c->fd = fd;
	c->st = status_idle;
	memcpy(c->sun_path, un.sun_path, sizeof(c->sun_path));
	*chn = c;
	return 0;
}

int channel_destroy(channel_t chn)
{
	if (!check_channel(chn))
	{
		LOGE("invalid channel %p", chn);
		return -1;
	}

	channel *c = (channel*)chn;
	close(c->fd);
	free(c);
	return 0;
}

static int get_service_addr(const char *service, char *addr, unsigned size)
{
	snprintf(addr, size, "/var/tmp/%s", service);
	return 0;
}

int channel_connect(channel_t chn, const char *service)
{
	if (!check_channel(chn))
	{
		LOGE("invalid channel %p", chn);
		return -1;
	}

	if (service == 0)
	{
		LOGE("invalid param");
		return -1;
	}

	channel *c = (channel *)chn;
	if (c->st != status_idle)
	{
		LOGE("invalid channel state %d", c->st);
		return -1;
	}
	struct sockaddr_un un;
	un.sun_family = AF_UNIX;
	get_service_addr(service, un.sun_path, sizeof(un.sun_path));
	if (connect(c->fd, (struct sockaddr *)&un, sizeof(un)) < 0)
	{
		LOGE("connect to service %s fail", service);
		return -1;
	}

	c->st = status_connected;
	return 0;
}

int channel_serve(channel_t chn, const char *service)
{
	if (!check_channel(chn))
	{
		LOGE("invalid channel %p", chn);
		return -1;
	}

	if (service == 0)
	{
		LOGE("invalid param");
		return -1;
	}

	channel *c = (channel *)chn;
	if (c->st != status_idle)
	{
		LOGE("invalid channel state %d", c->st);
		return -1;
	}

	char path[SUN_PATH_LEN] = {0};
	get_service_addr(service, path, sizeof(path));
	unlink(path);

	if (symlink(c->sun_path, path) < 0)
	{
		LOGE("symlink fail");
		return -1;
	}

	if (listen(c->fd, 5) < 0)
	{
		LOGE("listen fail");
		return -1;
	}

	c->st = status_listened;
	return 0;
}

int msg_loop(channel_t chn, void (*on_req)(channel_t chn, const void *req, unsigned reqlen))
{
	if (!check_channel(chn))
	{
		LOGE("invalid channel %p", chn);
		return -1;
	}

	channel *c = (channel *)chn;
	if (c->st == status_idle)
	{
		LOGE("invalid channel state %d", c->st);
		return -1;
	}


}

int send_rsp(const void *req, const void *rsp, unsigned rsplen)
{

}

static unsigned long generate_transid()
{
	static unsigned long transid = 0;
}

static int wait_rsp(unsigned long transid, unsigned timeout_ms)
{

}

int sync_call(channel_t chn, const void *req, unsigned reqlen, void **rsp, unsigned *rsplen, unsigned timeout_ms)
{
	if (!check_channel(chn))
	{
		LOGE("invalid channel %p", chn);
		return -1;
	}

	channel *c = (channel *)chn;
	if (c->st == status_idle)
	{
		LOGE("invalid channel state %d", c->st);
		return -1;
	}

	rpc_msg_hdr hdr = {generate_transid(), sizeof(hdr)+reqlen, 0};
	struct iovec msg_iov[] = {{&hdr,sizeof(hdr)},{req,reqlen}};
	struct msghdr msg = {0,0,msg_iov,sizeof(msg_iov)/sizeof(msg_iov[0]),0,0,0};

	if (sendmsg(c->fd, &msg, 0) < 0)
	{
		LOGE("sendmsg fail");
		return -1;
	}

	wait_rsp(hdr.transid, timeout_ms);
}
void free_rsp(void *rsp)
{

}

