#pragma once
#define _TCHAR_DEFINED
#include <d3d11.h>
#include <d3dcompiler.h>
#include <sstream>
#include "Container/Map.h"
#include "Container/Array.h"

struct FVertexShaderData
{
    ID3DBlob* VertexShaderCSO;
    ID3D11VertexShader* VertexShader;
};

class FDXDShaderManager
{
public:
    FDXDShaderManager() = default;
    FDXDShaderManager(ID3D11Device* Device);

    void ReleaseAllShader();

private:
    ID3D11Device* DXDDevice;

public:
    HRESULT AddVertexShader(const std::wstring& Key, const std::wstring& FileName);
    HRESULT AddVertexShader(const std::wstring& FileName, const std::string& EntryPoint, const D3D_SHADER_MACRO* Defines, size_t& OutShaderKey);
    HRESULT AddInputLayout(const std::wstring& Key, const D3D11_INPUT_ELEMENT_DESC* Layout, uint32_t LayoutSize);
    HRESULT AddPixelShader(const std::wstring& FileName, const std::string& EntryPoint, const D3D_SHADER_MACRO* Defines, size_t& OutShaderKey);

    HRESULT AddVertexShaderAndInputLayout(const std::wstring& FileName, const std::string& EntryPoint, const D3D11_INPUT_ELEMENT_DESC* Layout, uint32_t LayoutSize, const D3D_SHADER_MACRO* Defines, size_t& OutShaderKey);

    size_t ComputeShaderHash(const std::wstring& FileName, const std::string& EntryPoint, const D3D_SHADER_MACRO* Defines);

    ID3D11InputLayout* GetInputLayoutByKey(size_t Key) const;
    ID3D11VertexShader* GetVertexShaderByKey(size_t Key) const;
    ID3D11PixelShader* GetPixelShaderByKey(size_t key) const;
    ID3D11ComputeShader* GetComputeShaderByKey(const std::wstring& Key) const;

private:
    TMap<size_t, ID3D11InputLayout*> InputLayouts;
    TMap<size_t, ID3D11VertexShader*> VertexShaders;
    TMap<size_t, ID3D11PixelShader*> PixelShaders;
    TMap<std::wstring, ID3D11ComputeShader*> ComputeShaders;

};

