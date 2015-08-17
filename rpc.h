#ifndef _RPC_H
#define _RPC_H

#include "pbrpc.pb-c.h"

/* Rpcproto Method constants */
enum method_type {
        CALCULATE = 1
};

typedef int (*rpc_handler_func) (ProtobufCBinaryData *req,
                                 ProtobufCBinaryData *reply);

Pbcodec__PbRpcResponse *
rpc_read_rsp (const char* msg, size_t msg_len);

Pbcodec__PbRpcRequest *
rpc_read_req (const char* msg, size_t msg_len);

int
rpc_write_request (Pbcodec__PbRpcRequest *reqhdr, char **buf);

int
rpc_write_reply (Pbcodec__PbRpcResponse *rsphdr, char **buf);

int
rpc_invoke_call (Pbcodec__PbRpcRequest *reqhdr, Pbcodec__PbRpcResponse *rsphdr);

#endif
