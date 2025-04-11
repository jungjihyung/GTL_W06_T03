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
    HRESULT AddVertexShader(const std::wstring& Key, const std::wstring& FileName, const std::string& EntryPoint, TArray<D3D_SHADER_MACRO> Defines = TArray<D3D_SHADER_MACRO>());
    HRESULT AddInputLayout(const std::wstring& Key, const D3D11_INPUT_ELEMENT_DESC* Layout, uint32_t LayoutSize);
    HRESULT AddPixelShader(const std::wstring& FileName, const std::string& EntryPoint, const D3D_SHADER_MACRO* Defines, size_t& OutShaderKey);

    HRESULT AddVertexShaderAndInputLayout(const std::wstring& Key, const std::wstring& FileName, const std::string& EntryPoint, const D3D11_INPUT_ELEMENT_DESC* Layout, uint32_t LayoutSize, TArray<D3D_SHADER_MACRO> Defines = TArray<D3D_SHADER_MACRO>());

    size_t ComputeShaderHash(const std::wstring& FileName, const std::string& EntryPoint, const D3D_SHADER_MACRO* Defines);

    ID3D11InputLayout* GetInputLayoutByKey(const std::wstring& Key) const;
    ID3D11VertexShader* GetVertexShaderByKey(const std::wstring& Key) const;
    ID3D11PixelShader* GetPixelShaderByKey(size_t key) const;

private:
    TMap<std::wstring, ID3D11InputLayout*> InputLayouts;
    TMap<std::wstring, ID3D11VertexShader*> VertexShaders;
    TMap<size_t, ID3D11PixelShader*> PixelShaders;

};

