#pragma once

#include <rpc.h>
#include <string>
#include <vector>

RPC_BINDING_HANDLE create_binding_handle(const std::string& p_rhost);
bool create_remote_file(RPC_BINDING_HANDLE p_bind_handle, const std::string& p_rfile);
bool create_remote_directory(RPC_BINDING_HANDLE p_bind_handle, const std::string& p_rfile);
bool encrypt_remote_file(RPC_BINDING_HANDLE p_bind_handle, const std::string& p_rfile);
bool decrypt_remote_file(RPC_BINDING_HANDLE p_bind_handle, const std::string& p_rfile);
bool decrypt_remote_file(RPC_BINDING_HANDLE p_bind_handle, const std::string& p_rfile, RPC_STATUS& p_status);
std::vector<char> read_remote_file(RPC_BINDING_HANDLE p_bind_handle, const std::string& p_rfile);
bool write_plaintext_to_encrypted_efs(RPC_BINDING_HANDLE p_bind_handle, const std::vector<char>& p_remote_data, const std::string& p_rfile, const std::string&  p_attacker_data);