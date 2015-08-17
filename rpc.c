#include <string.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <endian.h>

#include "rpc.h"
#include "pbrpc.pb-c.h"
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

#define NUM_METHODS 5
/* Method name to enum mapping*/
char *method_by_name[NUM_METHODS] =
{ [CALCULATE] = "calculate"
};

/* Rpcproto methods table */
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
rpc_write_request (Pbcodec__PbRpcRequest *reqhdr, char **buf)
{
        uint64_t be_len = 0;
        if (!buf)
                return -1;

        size_t reqlen = pbcodec__pb_rpc_request__get_packed_size (reqhdr);
        *buf = calloc (1, reqlen+8);
        if (!*buf)
                return -1;

        pbcodec__pb_rpc_request__pack(reqhdr, *buf+8);
        be_len = htobe64 (reqlen);
        memcpy(*buf, &be_len, sizeof(uint64_t));
        return reqlen + sizeof(uint64_t);
}

/* returns the no. of bytes written to @buf
 * @buf is allocated by this function.
 * */
int
rpc_write_reply (Pbcodec__PbRpcResponse *rsphdr, char **buf)
{
        uint64_t be_len = 0;
        if (!buf)
                return -1;

        size_t rsplen = pbcodec__pb_rpc_response__get_packed_size (rsphdr);
        *buf = calloc (1, rsplen+sizeof(uint64_t));
        if (!*buf)
                return -1;

        be_len = htobe64 (rsplen);
        pbcodec__pb_rpc_response__pack (rsphdr, *buf + sizeof(uint64_t));
        memcpy (*buf, &be_len, sizeof(be_len));
        return rsplen+sizeof(uint64_t);
}

int
rpc_invoke_call (Pbcodec__PbRpcRequest *reqhdr, Pbcodec__PbRpcResponse *rsphdr)
{
        int ret;

        if (!reqhdr->has_params){
                fprintf(stderr, "no params passed\n");
                return -1;
        }
        rsphdr->id = reqhdr->id;

        int idx = 0, method = -1;
        for (idx = 0; idx < NUM_METHODS; idx++) {
                if (!method_by_name[idx])
                        continue;
                if (strcmp (method_by_name[idx], reqhdr->method) == 0) {
                        method = idx;
                        break;
                }
        }
        if (method == -1)
                return -1;

        ret = vtable[method] (&reqhdr->params, &rsphdr->result);

        return ret;
}

Pbcodec__PbRpcResponse *
rpc_read_rsp (const char *msg, size_t msg_len)
{
        char *hdr;
        uint64_t proto_len = 0;

        memcpy (&proto_len, msg, sizeof(uint64_t));
        proto_len = be64toh (proto_len);

        return pbcodec__pb_rpc_response__unpack(NULL, proto_len, msg+sizeof(proto_len));
}

Pbcodec__PbRpcRequest *
rpc_read_req (const char* msg, size_t msg_len)
{
        char *hdr;
        uint64_t proto_len = 0;

        memcpy (&proto_len, msg, sizeof(uint64_t));
        proto_len = be64toh (proto_len);
        return pbcodec__pb_rpc_request__unpack (NULL, proto_len, msg+sizeof(proto_len));
}
