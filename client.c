#include "calc.pb-c.h"
#include "rpc.pb-c.h"
#include "rpc.h"
#include <inttypes.h>
#include <event2/event.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>


#define DBUG(x) fprintf (stdout, "%s was called\n", # x)
static void
event_cb (struct bufferevent *bev, short events, void *ctx)
{
        DBUG(event_cb);
        if (events & BEV_EVENT_CONNECTED) {
                Calc__CalcReq calc = CALC__CALC_REQ__INIT;
                calc.op = 1; calc.a = 2; calc.b = 3;
                size_t clen = calc__calc_req__get_packed_size(&calc);
                char *cbuf = calloc (1, clen);
                calc__calc_req__pack(&calc, cbuf);

                Rpcproto__ReqHeader reqhdr = RPCPROTO__REQ_HEADER__INIT;
                reqhdr.id = 1;
                reqhdr.method = CALCULATE;
                reqhdr.has_params = 1;
                reqhdr.params.data = cbuf;
                reqhdr.params.len = clen;

                char *rbuf = NULL;
                size_t rlen = rpc_write_request (&reqhdr, &rbuf);
                bufferevent_write(bev, rbuf, rlen);
        }
}

static void
write_cb (struct bufferevent *bev, void *ctx)
{
        DBUG(write_cb);
        fprintf (stdout, "remaining bytes in output buffer %d\n",
                 evbuffer_get_length(bufferevent_get_output(bev)));
}

static void
read_cb (struct bufferevent *bev, void *ctx)
{
        DBUG(read_cb);
        char buf[128] = {0};
        size_t read = bufferevent_read(bev, buf, sizeof(buf));
        Rpcproto__RspHeader *rsp = rpc_read_rsp (buf, read);
        Calc__CalcRsp *crsp = calc__calc_rsp__unpack (NULL, rsp->result.len, rsp->result.data);

        printf("rsp->id = %d, rsp->sum = %d\n", rsp->id, crsp->ret);
        calc__calc_rsp__free_unpacked (crsp, NULL);
}

int main(int argc, char **argv)
{
        struct event_base *base;
        int ret;
        base = event_base_new ();
        if (!base) {
                fprintf(stderr, "failed to create event base\n");
                return 1;
        }
        struct bufferevent *bev = bufferevent_socket_new (base, -1,
                                                          BEV_OPT_CLOSE_ON_FREE);
        if (!bev) {
                fprintf(stderr, "failed to create bufferevent\n");
                return 1;
        }
        ret = bufferevent_socket_connect_hostname (bev, NULL, AF_INET,
                                                   "localhost", 9876);
        if (ret) {
                perror("connect failed");
                return 1;
        }
        bufferevent_setcb (bev, read_cb, write_cb, event_cb, NULL);
        bufferevent_enable (bev, EV_READ|EV_WRITE);
        event_base_dispatch(base);
}



