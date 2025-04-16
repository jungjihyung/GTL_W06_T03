// ShaderHashUtils.h
#pragma once
#include <string>
#include <sstream>
#include <functional>
#include <d3dcompiler.h> 

#include <Windows.h>
#include <fstream>

struct ShaderCompileInfo
{
    std::wstring FileName;
    std::string EntryPoint;
    const D3D_SHADER_MACRO* Defines;
};

namespace ShaderHashUtils
{
    inline size_t ComputeHashKey(const ShaderCompileInfo& info)
    {
        std::wstringstream ss;
        ss << info.FileName << L"_" << std::wstring(info.EntryPoint.begin(), info.EntryPoint.end());
        if (info.Defines)
        {
            for (int i = 0; info.Defines[i].Name != nullptr; ++i)
            {
                ss << L"_" << std::wstring(info.Defines[i].Name, info.Defines[i].Name + strlen(info.Defines[i].Name));
                ss << L"_" << std::wstring(info.Defines[i].Definition, info.Defines[i].Definition + strlen(info.Defines[i].Definition));
            }
        }
        return std::hash<std::wstring>()(ss.str());
    }

    inline std::size_t ComputeFileHash(const std::wstring& filename)
    {
        std::ifstream file(filename, std::ios::binary);
        if (!file)
            return 0;
        std::ostringstream contents;
        contents << file.rdbuf();
        std::string fileContent = contents.str();
        return std::hash<std::string>()(fileContent);
    }
}
