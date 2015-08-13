#ifndef _RPC_H
#define _RPC_H

#include "rpc.pb-c.h"

/* Rpcproto Method constants */
enum method_type {
        CALCULATE = 1
};

typedef int (*rpc_handler_func) (ProtobufCBinaryData *req,
                                 ProtobufCBinaryData *reply);

Rpcproto__RspHeader *
rpc_read_rsp (const char* msg, size_t msg_len);

Rpcproto__ReqHeader *
rpc_read_req (const char* msg, size_t msg_len);

int
rpc_write_request (Rpcproto__ReqHeader *reqhdr, char **buf);

int
rpc_write_reply (Rpcproto__RspHeader *rsphdr, char **buf);

int
rpc_invoke_call (Rpcproto__ReqHeader *reqhdr, Rpcproto__RspHeader *rsphdr);

#endif
