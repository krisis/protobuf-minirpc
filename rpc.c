#include <string.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>

#include "rpc.h"
#include "rpc.pb-c.h"
#include "calc.pb-c.h"

/* Rpcproto Method constants */
enum method_type {
        CALCULATE = 1
};
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
 * reqhdr = rpc_parse_header (msg, len);
 * rsphdr = RSP_HEADER__INIT;
 * ret = rpc_invoke_call (reqhdr, &rsphdr)
 * if (ret)
 *   goto out;
 * char *buf = NULL;
 * ret = rpc_build_response (&rsphdr, &buf);
 * if (ret)
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
rpc_build_response (Rpcproto__RspHeader *rsphdr, char **buf)
{
        if (!buf)
                return -1;

        size_t rsplen = rpcproto__rsp_header__get_packed_size (rsphdr);
        *buf = calloc (1, rsplen);
        if (*buf)
                return -1;

        return rpcproto__rsp_header__pack (rsphdr, *buf);
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

Rpcproto__ReqHeader *
rpc_parse_header (const char* msg, size_t msg_len)
{
        char *hdr;
        uint64_t proto_len;

        proto_len = strtoull (msg, &hdr, 10);
        if ((hdr == msg)|| (proto_len == ULLONG_MAX)) {
                fprintf(stderr, "invalid message length\n");
                return NULL;
        }

        return rpcproto__req_header__unpack (NULL, proto_len, hdr);
}
