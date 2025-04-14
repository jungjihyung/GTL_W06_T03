#pragma once
#define _TCHAR_DEFINED
#include <d3d11.h>
#include <d3dcompiler.h>
#include <sstream>
#include "Container/Map.h"
#include "Container/Array.h"
#include "Launch/EngineBaseTypes.h"
class FGraphicsDevice;

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
    EViewModeIndex ViewMode; // Defines에 대한 메모리 관리에 주의
    std::size_t FileHash;
    EShaderType ShaderType;
};


namespace
{
    // Gouraud 조명 모델 (Lit 모드)
    D3D_SHADER_MACRO DefineLit_Gouraud[] =
    {
        { "LIT_MODE", "1" },
        { "LIGHTING_MODEL_GOURAUD", "1" },
        { "LIGHTING_MODEL_LAMBERT", "0" },
        { "LIGHTING_MODEL_PHONG", "0" },
         {"WORLD_NORMAL_MODE", "0" },
        { nullptr, nullptr }
    };

    // Lambert 조명 모델 (Lit 모드)
    D3D_SHADER_MACRO DefineLit_Lambert[] =
    {
        { "LIT_MODE", "1" },
        { "LIGHTING_MODEL_GOURAUD", "0" },
        { "LIGHTING_MODEL_LAMBERT", "1" },
        { "LIGHTING_MODEL_PHONG", "0" },
        {   "WORLD_NORMAL_MODE", "0" },

        { nullptr, nullptr }
    };

    // Phong 조명 모델 (Lit 모드)
    D3D_SHADER_MACRO DefineLit_Phong[] =
    {
        { "LIT_MODE", "1" },
        { "LIGHTING_MODEL_GOURAUD", "0" },
        { "LIGHTING_MODEL_LAMBERT", "0" },
        { "LIGHTING_MODEL_PHONG", "1" },
        {"WORLD_NORMAL_MODE", "0" },
        { nullptr, nullptr }
    };

    D3D_SHADER_MACRO DefineUnLit[] =
    {
        { "LIT_MODE", "0" },
        { "LIGHTING_MODEL_GOURAUD", "0" },
        { "LIGHTING_MODEL_LAMBERT", "0" },
        { "LIGHTING_MODEL_PHONG", "0" },
        {"WORLD_NORMAL_MODE", "0" },
        { nullptr, nullptr }
    };

    D3D_SHADER_MACRO DefineWorldNormal[] =
    {
        { "LIT_MODE", "0" },
        { "LIGHTING_MODEL_GOURAUD", "0" },
        { "LIGHTING_MODEL_LAMBERT", "0" },
        { "LIGHTING_MODEL_PHONG", "0" },
        {"WORLD_NORMAL_MODE", "1" },
        { nullptr, nullptr }
    };
}

class FDXDShaderManager
{
public:
    FDXDShaderManager() = default;
    FDXDShaderManager(ID3D11Device* Device, FGraphicsDevice* GraphicsDevice);

    void ReleaseAllShader();

private:
    ID3D11Device* DXDDevice;
    FGraphicsDevice* GraphicDevice;

public:
    HRESULT AddVertexShader(const std::wstring& Key, const std::wstring& FileName);
    HRESULT AddVertexShader(const std::wstring& FileName, const std::string& EntryPoint, EViewModeIndex ViewMode, size_t& OutShaderKey);
    HRESULT AddInputLayout(const std::wstring& Key, const D3D11_INPUT_ELEMENT_DESC* Layout, uint32_t LayoutSize);
    HRESULT AddPixelShader(const std::wstring& FileName, const std::string& EntryPoint, EViewModeIndex ViewMode, size_t& OutShaderKey);

    HRESULT AddVertexShaderAndInputLayout(const std::wstring& FileName, const std::string& EntryPoint, const D3D11_INPUT_ELEMENT_DESC* Layout, uint32_t LayoutSize, EViewModeIndex ViewMode, size_t& OutShaderKey);

    HRESULT AddComputeShader(const std::wstring& Key, const std::wstring& FileName, const std::string& EntryPoint);
    size_t CalculateShaderHashKey(const std::wstring& FileName, const std::string& EntryPoint, const D3D_SHADER_MACRO* Define);

    D3D_SHADER_MACRO* GetShaderMacro(EViewModeIndex ViewMode);

    ID3D11InputLayout* GetInputLayoutByKey(size_t Key) const;
    ID3D11VertexShader* GetVertexShaderByKey(size_t Key) const;
    ID3D11PixelShader* GetPixelShaderByKey(size_t key) const;
    ID3D11ComputeShader* GetComputeShaderByKey(const std::wstring& Key) const;
    FILETIME GetLastWriteTime(const std::wstring& filename);
    void CheckAndReloadShaders();
private:
    TMap<size_t, ID3D11InputLayout*> InputLayouts;
    TMap<size_t, ID3D11VertexShader*> VertexShaders;
    TMap<size_t, ID3D11PixelShader*> PixelShaders;
    TMap<std::wstring, ID3D11ComputeShader*> ComputeShaders;
    TMap<size_t, FShaderReloadInfo> ShaderReloadMap;
};

struct FWatchParams
{
    FDXDShaderManager* pShaderManager;
    std::wstring Directory;
};
