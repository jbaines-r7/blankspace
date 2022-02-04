#pragma once

#include <vector>
#include <string>

// I hate this. But I didn't want to junk up efsrpc_wrapper with this buffer manipulation
// code, especially since I don't think it's ideal. Seperating this stuff into a new compilation
// unit should hopefully make it easier for others (lol) to rip it out of efsrpc_wrapper.

template <typename T> void AppendValue(std::vector<char>& buf, const T value);
void AppendWideString(std::vector<char>& buf, const std::wstring& str);
void AppendBuffer(std::vector<char>& buf, const std::vector<char>& from);
void AppendBuffer(std::vector<char>& buf, const std::string& from);

template <typename T> T ReadValue(const std::vector<char>& from, size_t& ofs);
std::vector<char> ReadBuffer(const std::vector<char>& from, size_t& ofs, size_t size);

void CopyChunk(std::vector<char>& buf, const std::vector<char>& from, size_t& ofs);
void WriteStreamHeader(std::vector<char>& buf, const std::wstring& name);
void WriteGUREHeader(std::vector<char>& buf, const std::string& contents);