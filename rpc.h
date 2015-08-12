#ifndef _RPC_H
#define _RPC_H

#include "rpc.pb-c.h"

typedef int (*rpc_handler_func) (ProtobufCBinaryData *req,
                                 ProtobufCBinaryData *reply);

int
rpc_build_response (Rpcproto__RspHeader *rsphdr, char **buf);

int
rpc_invoke_call (Rpcproto__ReqHeader *reqhdr, Rpcproto__RspHeader *rsphdr);

Rpcproto__ReqHeader *
rpc_parse_header (const char* msg, size_t msg_len);

#endif
