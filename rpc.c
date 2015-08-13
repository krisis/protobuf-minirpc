#include <string.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>

#include "list.h"
#include "rpc.h"
#include "rpc.pb-c.h"
#include "calc.pb-c.h"

/* Rpcproto method declarations */

static int
calculate (ProtobufCBinaryData *req, ProtobufCBinaryData *reply);

/* End of method declarations */

/*
 * FIXME: Need to come up with rpc registration API
 * Should provide vtable during initialisation.
 *
 * */

/* Rpcproto methods table */
#define NUM_METHODS 5
static rpc_handler_func vtable[NUM_METHODS] =
{
        [CALCULATE] = calculate,
};

/*
 * RPC Format
 * -----------------------------------------------------
 * | Length: uint64                                    |
 * -----------------------------------------------------
 * |                                                   |
 * | Header:  Id uint64, Method int32, Params []bytes |
 * |                                                   |
 * -----------------------------------------------------
 * |                                                   |
 * | Body: Method dependent representation             |
 * |                                                   |
 * -----------------------------------------------------
 *
 **/

static int
calculate (ProtobufCBinaryData *req, ProtobufCBinaryData *reply)
{
        Calc__CalcReq *creq;
        Calc__CalcRsp crsp = CALC__CALC_RSP__INIT;
        size_t rsplen;
        int ret = 0;

        creq = calc__calc_req__unpack (NULL, req->len, req->data);

        /* method-specific logic */
        switch(creq->op) {
        default:
                crsp.ret = creq->a + creq->b;
        }

        rsplen = calc__calc_rsp__get_packed_size(&crsp);
        reply->data = calloc (1, rsplen);
        if (!reply->data) {
                ret = -1;
                goto out;
        }

        reply->len = rsplen;
        calc__calc_rsp__pack(&crsp, reply->data);
out:
        calc__calc_req__free_unpacked (creq, NULL);

        return 0;
}

/*
 * pseudo code:
 *
 * read buffer from network
 * reqhdr = rpc_read_req (msg, len);
 * rsphdr = RSP_HEADER__INIT;
 * ret = rpc_invoke_call (reqhdr, &rsphdr)
 * if (ret)
 *   goto out;
 * char *buf = NULL;
 * ret = rpc_write_reply (&rsphdr, &buf);
 * if (ret != -1)
 *   goto out;
 *
 * send buf over n/w
 *
 * free(buf);
 * free(rsp->result->data);
 *
 *
 * ...
 * clean up protobuf buffers on finish
 *
 * */

/* returns the no. of bytes written to @buf
 * @buf is allocated by this function.
 * */
int
rpc_write_request (Rpcproto__ReqHeader *reqhdr, char **buf)
{
        if (!buf)
                return -1;

        size_t reqlen = rpcproto__req_header__get_packed_size (reqhdr);
        *buf = calloc (1, reqlen+8);
        if (!*buf)
                return -1;

        rpcproto__req_header__pack(reqhdr, *buf+8);
        memcpy(*buf, &reqlen, sizeof(uint64_t));
        return reqlen + sizeof(uint64_t);
}

/* returns the no. of bytes written to @buf
 * @buf is allocated by this function.
 * */
int
rpc_write_reply (Rpcproto__RspHeader *rsphdr, char **buf)
{
        if (!buf)
                return -1;

        size_t rsplen = rpcproto__rsp_header__get_packed_size (rsphdr);
        *buf = calloc (1, rsplen+sizeof(uint64_t));
        if (!*buf)
                return -1;

        rpcproto__rsp_header__pack (rsphdr, *buf + sizeof(uint64_t));
        memcpy (*buf, &rsplen, sizeof(rsplen));
        return rsplen+sizeof(uint64_t);
}

int
rpc_invoke_call (Rpcproto__ReqHeader *reqhdr, Rpcproto__RspHeader *rsphdr)
{
        int ret;

        if (!reqhdr->has_params){
                fprintf(stderr, "no params passed\n");
                return -1;
        }
        rsphdr->id = reqhdr->id;
        ret = vtable[reqhdr->method] (&reqhdr->params, &rsphdr->result);

        return ret;
}

Rpcproto__RspHeader *
rpc_read_rsp (const char *msg, size_t msg_len)
{
        char *hdr;
        uint64_t proto_len = 0;

        memcpy (&proto_len, msg, sizeof(uint64_t));
        printf ("proto_len = %"PRIu64"\n", proto_len);

        return rpcproto__rsp_header__unpack(NULL, proto_len, msg+sizeof(proto_len));
}

Rpcproto__ReqHeader *
rpc_read_req (const char* msg, size_t msg_len)
{
        char *hdr;
        uint64_t proto_len = 0;

        memcpy (&proto_len, msg, sizeof(uint64_t));
        printf ("proto_len = %"PRIu64"\n", proto_len);
        return rpcproto__req_header__unpack (NULL, proto_len, msg+sizeof(proto_len));
}
