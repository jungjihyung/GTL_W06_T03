#pragma once
#define _TCHAR_DEFINED
#include <d3d11.h>
#include <d3dcompiler.h>
#include <sstream>
#include "Container/Map.h"
#include "Container/Array.h"

enum class EShaderType
{
    Vertex,
    Pixel
};
struct FVertexShaderData
{
    ID3DBlob* VertexShaderCSO;
    ID3D11VertexShader* VertexShader;
};

struct FShaderReloadInfo
{
    std::wstring FileName;
    std::string EntryPoint;
    const D3D_SHADER_MACRO* Defines; // Defines에 대한 메모리 관리에 주의
    FILETIME LastModified;
    EShaderType ShaderType;
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

    size_t CalculateShaderHashKey(const std::wstring& FileName, const std::string& EntryPoint, const D3D_SHADER_MACRO* Defines);

    ID3D11InputLayout* GetInputLayoutByKey(size_t Key) const;
    ID3D11VertexShader* GetVertexShaderByKey(size_t Key) const;
    ID3D11PixelShader* GetPixelShaderByKey(size_t key) const;

    void CheckAndReloadShaders();
private:
    TMap<size_t, ID3D11InputLayout*> InputLayouts;
    TMap<size_t, ID3D11VertexShader*> VertexShaders;
    TMap<size_t, ID3D11PixelShader*> PixelShaders;
    TMap<size_t, FShaderReloadInfo> ShaderReloadMap;
};

struct FWatchParams
{
    FDXDShaderManager* pShaderManager;
    std::wstring Directory;
};
