/*
 * channel_test.c
 *
 *  Created on: May 5, 2018
 *      Author: crh
 */

#include <channel.h>
#include <string.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#include "demo_app.h"

#define HANDLER(msg) handler_##msg
#define HANDLER_IMPL(msg) void HANDLER(msg)(channel_t chn, const demo_msg *req, unsigned reqlen)
#define MSG_HANDLER(msg) {msg, HANDLER(msg)}

HANDLER_IMPL(CMD_1)
{
	send_rsp(req, "", 0);
}

HANDLER_IMPL(CMD_2)
{
	send_rsp(req, "", 0);
}

HANDLER_IMPL(CMD_3)
{
	send_rsp(req, "", 0);
}

static struct
{
	int cmd;
	void (*msg_handler)(channel_t chn, const demo_msg *req, unsigned reqlen);
}gs_msg_handlers[] =
		{
				MSG_HANDLER(CMD_1),
				MSG_HANDLER(CMD_2),
				MSG_HANDLER(CMD_3)
		};

static void on_req(channel_t chn, const void *req, unsigned reqlen)
{
	const demo_msg *msg = (demo_msg*)req;
	if (reqlen < sizeof(demo_msg))
	{
		printf("invalid req size %u\n", reqlen);
		return;
	}

	if (msg->cmd >= CMD_BUTT)
	{
		printf("invalid req cmd %d\n", msg->cmd);
		return;
	}

	gs_msg_handlers[msg->cmd].msg_handler(chn, msg, reqlen);
}

static void *msgloop(void *param)
{
	channel_t chn = (channel_t)param;

	msg_loop(chn, on_req);
	return 0;
}

static int client()
{
	pthread_t thr;
	channel_t chn;
	int ret;

	ret = channel_create(&chn);
	if (ret != 0)
	{
		printf("create channel fail %d\n", ret);
		return -1;
	}

	ret = channel_connect(chn, "test");
	if (ret != 0)
	{
		printf("channel connect `test' fail %d\n", ret);
		return -1;
	}

	pthread_create(&thr, 0, msgloop, chn);

	unsigned msgsize = sizeof(demo_msg) + 100, rsplen;
	void *rsp;
	demo_msg *req = (demo_msg*)malloc(msgsize);
	if (req)
	{
		req->cmd = CMD_1;
		strcpy((char*)(req+1), "hello");

		ret = sync_call(chn, req, msgsize, &rsp, &rsplen, 1000);
		if (ret == 0)
		{
			free_rsp(rsp);
		}
	}

	pthread_join(thr, 0);

	return 0;
}

static int server()
{
	pthread_t thr;
	channel_t chn;
	int ret;

	ret = channel_create(&chn);
	if (ret != 0)
	{
		printf("create channel fail %d\n", ret);
		return -1;
	}

	ret = channel_serve(chn, "test");
	if (ret != 0)
	{
		printf("channel serve `test' fail %d\n", ret);
		return -1;
	}

	pthread_create(&thr, 0, msgloop, chn);
	pthread_join(thr, 0);

	return 0;
}

int main0(int argc, const char *argv[])
{
	if (argc > 1 && 0==strcmp(argv[1], "-s"))
	{
		return server();
	}
	return client();
}
