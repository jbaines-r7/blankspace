#include "encrypted_object_manipulation.hpp"

#include <algorithm>
#include <iterator>
#include <iostream>

template <typename T> void AppendValue(std::vector<char>& buf, const T value)
{
    const char* base = reinterpret_cast<const char*>(&value);
    buf.insert(buf.end(), base, base + sizeof(T));
}

void AppendWideString(std::vector<char>& buf, const std::wstring& str)
{
    for (std::wstring::const_iterator it = str.begin(); it != str.end(); ++it)
    {
        AppendValue(buf, *it);
    }
}

void AppendBuffer(std::vector<char>& buf, const std::vector<char>& from)
{
    std::copy(from.begin(), from.end(), std::back_inserter<std::vector<char> >(buf));
}

void AppendBuffer(std::vector<char>& buf, const std::string& from)
{
    std::copy(from.begin(), from.end(), std::back_inserter<std::vector<char> >(buf));
}

template <typename T> T ReadValue(const std::vector<char>& from, size_t& ofs)
{
    size_t remaining = from.size() - ofs;
    if (remaining < sizeof(T))
    {
        std::cerr << "[-] Invalid read value size. Aborting" << std::endl;
        exit(0);
    }
    T ret = *reinterpret_cast<const T*>(from.data() + ofs);
    ofs += sizeof(T);
    return ret;
}

std::vector<char> ReadBuffer(const std::vector<char>& from, size_t& ofs, size_t size)
{
    size_t remaining = from.size() - ofs;
    if (remaining < size)
    {
        std::cerr << "[-] Invalid read value size. Aborting" << std::endl;
        exit(0);
    }
    auto ret = std::vector<char>(from.begin() + ofs, from.begin() + ofs + size);
    ofs += size;
    return ret;
}

void CopyChunk(std::vector<char>& buf, const std::vector<char>& from, size_t& ofs)
{
    int length = ReadValue<int>(from, ofs);
    AppendValue(buf, length);
    AppendBuffer(buf, ReadBuffer(from, ofs, length - 4));
}

void WriteStreamHeader(std::vector<char>& buf, const std::wstring& name)
{
    int name_length = name.size() * sizeof(wchar_t);
    int length = 4 + 8 + 4 + 8 + 4 + name_length;
    AppendValue(buf, length);
    AppendWideString(buf, L"NTFS");
    AppendValue(buf, 1);
    AppendValue<unsigned long long>(buf, 0);
    AppendValue(buf, name_length);
    AppendWideString(buf, name);
}

void WriteGUREHeader(std::vector<char>& buf, const std::string& contents)
{
    int length = 4 + 8 + 4 + contents.size();
    AppendValue(buf, length);
    AppendWideString(buf, L"GURE");
    AppendValue(buf, 0);
    AppendBuffer(buf, contents);
}