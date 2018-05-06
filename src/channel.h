/*
 * channel.h
 *
 *  Created on: May 5, 2018
 *      Author: crh
 */

#ifndef SRC_CHANNEL_H_
#define SRC_CHANNEL_H_


#ifdef __cplusplus
extern "C" {
#endif

typedef void *channel_t;

int channel_create(channel_t *chn);
int channel_destroy(channel_t chn);

int channel_connect(channel_t chn, const char *service);
int channel_serve(channel_t chn, const char *service);

int msg_loop(channel_t chn, void (*on_req)(channel_t chn, const void *req, unsigned reqlen));
int send_rsp(const void *req, const void *rsp, unsigned rsplen);

int sync_call(channel_t chn, const void *req, unsigned reqlen, void **rsp, unsigned *rsplen, unsigned timeout_ms);
void free_rsp(void *rsp);







#ifdef __cplusplus
}
#endif
#endif /* SRC_CHANNEL_H_ */
