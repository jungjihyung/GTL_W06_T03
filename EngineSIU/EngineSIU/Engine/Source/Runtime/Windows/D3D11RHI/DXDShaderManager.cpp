#include "DXDShaderManager.h"
#include "D3D11RHI/GraphicDevice.h"
#include "HotReload/ShaderHotReload.h"
#include "HotReload/ShaderHashUtils.h"
#include <d3d11.h>
#include <d3dcompiler.h>

// --------------------------------------------------------------------------
// FDXDShaderManager 클래스 구현
// --------------------------------------------------------------------------

FDXDShaderManager::FDXDShaderManager(ID3D11Device* Device, FGraphicsDevice* InGraphicsDevice)
    : DXDDevice(Device), GraphicDevice(InGraphicsDevice)
{
    VertexShaders.Empty();
    PixelShaders.Empty();
}

void FDXDShaderManager::ReleaseAllShader()
{
    for (auto& [Key, Shader] : VertexShaders)
    {
        if (Shader)
        {
            Shader->Release();
            Shader = nullptr;
        }
    }
    VertexShaders.Empty();

    for (auto& [Key, Shader] : PixelShaders)
    {
        if (Shader)
        {
            Shader->Release();
            Shader = nullptr;
        }
    }
    PixelShaders.Empty();
}

D3D_SHADER_MACRO* FDXDShaderManager::GetShaderMacro(EViewModeIndex ViewMode)
{
    switch (ViewMode)
    {
    case EViewModeIndex::VMI_Lit_Gouraud:
        return DefineLit_Gouraud;
    case EViewModeIndex::VMI_Lit_Lambert:
        return DefineLit_Lambert;
    case EViewModeIndex::VMI_Lit_Phong:
        return DefineLit_Phong;
    case EViewModeIndex::VMI_Unlit:
    case EViewModeIndex::VMI_SceneDepth:
        return DefineUnLit;
    case EViewModeIndex::VMI_WorldNormal:
        return DefineWorldNormal;
    case EViewModeIndex::VMI_ICON:
        return DefineDiscardAlpha;
    case EViewModeIndex::VMI_Billboard:
        return DefineDiscardBlack;
    default:
        return nullptr;
    }
}

HRESULT FDXDShaderManager::AddComputeShader(const std::wstring& Key, const std::wstring& FileName, const std::string& EntryPoint)
{
    UINT shaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#ifdef _DEBUG
    shaderFlags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif
    HRESULT hr = S_OK;

    if (DXDDevice == nullptr)
        return S_FALSE;

    ID3DBlob* CsBlob = nullptr;
    ID3DBlob* errorBlob = nullptr;
    hr = D3DCompileFromFile(FileName.c_str(), nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, EntryPoint.c_str(), "cs_5_0", shaderFlags, 0, &CsBlob, &errorBlob);
    if (FAILED(hr))
    {
        if (errorBlob)
        {
            OutputDebugStringA((char*)errorBlob->GetBufferPointer());
            errorBlob->Release();
        }
        return hr;
    }
    ID3D11ComputeShader* NewComputeShader;
    hr = DXDDevice->CreateComputeShader(CsBlob->GetBufferPointer(), CsBlob->GetBufferSize(), nullptr, &NewComputeShader);
    if (CsBlob)
    {
        CsBlob->Release();
    }
    if (FAILED(hr))
        return hr;
    ComputeShaders[Key] = NewComputeShader;

    return S_OK;
}

HRESULT FDXDShaderManager::AddVertexShader(const std::wstring& Key, const std::wstring& FileName)
{
    return E_NOTIMPL;
}

HRESULT FDXDShaderManager::AddVertexShader(const std::wstring& FileName, const std::string& EntryPoint, EViewModeIndex ViewMode, size_t& OutShaderKey)
{
    const D3D_SHADER_MACRO* Defines = GetShaderMacro(ViewMode);
    size_t shaderKey = ShaderHashUtils::ComputeHashKey(ShaderCompileInfo(FileName, EntryPoint, Defines));

    OutShaderKey = shaderKey;

    if (VertexShaders.Contains(shaderKey))
    {
        return S_OK;
    }

    UINT shaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#ifdef _DEBUG
    shaderFlags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

    if (DXDDevice == nullptr)
        return S_FALSE;

    HRESULT hr = S_OK;
    ID3DBlob* VsBlob = nullptr;
    ID3DBlob* errorBlob = nullptr;

    hr = D3DCompileFromFile(FileName.c_str(), Defines, D3D_COMPILE_STANDARD_FILE_INCLUDE,
        EntryPoint.c_str(), "vs_5_0", shaderFlags, 0, &VsBlob, &errorBlob);
    if (FAILED(hr)) {
        if (errorBlob) {
            OutputDebugStringA((char*)errorBlob->GetBufferPointer());
            errorBlob->Release();
        }
        return hr;
    }

    ID3D11VertexShader* NewVertexShader = nullptr;
    hr = DXDDevice->CreateVertexShader(VsBlob->GetBufferPointer(), VsBlob->GetBufferSize(), nullptr, &NewVertexShader);
    if (VsBlob)
    {
        VsBlob->Release();
    }

    if (FAILED(hr))
        return hr;

    VertexShaders[shaderKey] = NewVertexShader;

    // 해시값 계산하여 저장
    std::size_t currentHash = ShaderHashUtils::ComputeFileHash(FileName);
    FShaderReloadInfo reloadInfo;
    reloadInfo.FileName = FileName;
    reloadInfo.EntryPoint = EntryPoint;
    reloadInfo.ViewMode = ViewMode;
    reloadInfo.FileHash = currentHash;
    reloadInfo.ShaderKey = shaderKey;
    reloadInfo.ShaderType = EShaderType::Vertex;

    ShaderReloadMap[shaderKey] = reloadInfo;

    return S_OK;
}

HRESULT FDXDShaderManager::AddPixelShader(const std::wstring& FileName, const std::string& EntryPoint, EViewModeIndex ViewMode, size_t& OutShaderKey)
{
    const D3D_SHADER_MACRO* Defines = GetShaderMacro(ViewMode);
    return CompilePixelShader(FileName, EntryPoint, ViewMode, OutShaderKey, Defines);
}

HRESULT FDXDShaderManager::AddPixelShader(const std::wstring& FileName, const std::string& EntryPoint, EViewModeIndex ViewMode, size_t& OutShaderKey, const TArray<D3D_SHADER_MACRO>& DynamicMacros)
{
    TArray<D3D_SHADER_MACRO> FinalDefines;

    const D3D_SHADER_MACRO* BaseDefines = GetShaderMacro(ViewMode);
    for (; BaseDefines->Name != nullptr; ++BaseDefines) {
        FinalDefines.Add(*BaseDefines);
    }

    for (const auto& macro : DynamicMacros) {
        FinalDefines.Add(macro);
    }
    FinalDefines.Add({ nullptr, nullptr });

    return CompilePixelShader(FileName, EntryPoint, ViewMode, OutShaderKey, FinalDefines.GetData());
}

HRESULT FDXDShaderManager::CompilePixelShader(const std::wstring& FileName, const std::string& EntryPoint, EViewModeIndex ViewMode, size_t& OutShaderKey, const D3D_SHADER_MACRO* Macros)
{
    size_t shaderKey = ShaderHashUtils::ComputeHashKey(ShaderCompileInfo(FileName, EntryPoint, Macros));
    OutShaderKey = shaderKey;

    if (PixelShaders.Contains(shaderKey))
    {
        return S_OK;
    }

    UINT shaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#ifdef _DEBUG
    shaderFlags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

    if (DXDDevice == nullptr)
        return S_FALSE;

    HRESULT hr = S_OK;
    ID3DBlob* PsBlob = nullptr;
    hr = D3DCompileFromFile(FileName.c_str(), Macros, D3D_COMPILE_STANDARD_FILE_INCLUDE,
        EntryPoint.c_str(), "ps_5_0", shaderFlags, 0, &PsBlob, nullptr);
    if (FAILED(hr))
        return hr;

    ID3D11PixelShader* NewPixelShader = nullptr;
    hr = DXDDevice->CreatePixelShader(PsBlob->GetBufferPointer(), PsBlob->GetBufferSize(), nullptr, &NewPixelShader);
    if (PsBlob)
    {
        PsBlob->Release();
    }

    if (FAILED(hr))
        return hr;

    std::size_t currentHash = ShaderHashUtils::ComputeFileHash(FileName);
    FShaderReloadInfo reloadInfo;
    reloadInfo.FileName = FileName;
    reloadInfo.EntryPoint = EntryPoint;
    reloadInfo.ViewMode = ViewMode;
    reloadInfo.FileHash = currentHash;
    reloadInfo.ShaderKey = shaderKey;
    reloadInfo.ShaderType = EShaderType::Pixel;

    ShaderReloadMap[shaderKey] = reloadInfo;

    PixelShaders[shaderKey] = NewPixelShader;

    return S_OK;
}

FILETIME FDXDShaderManager::GetLastWriteTime(const std::wstring& filename)
{
    FILETIME ftWrite = { 0 };
    WIN32_FILE_ATTRIBUTE_DATA attrData;
    if (GetFileAttributesEx(filename.c_str(), GetFileExInfoStandard, &attrData))
    {
        ftWrite = attrData.ftLastWriteTime;
    }
    return ftWrite;
}

HRESULT FDXDShaderManager::AddInputLayout(const std::wstring& Key, const D3D11_INPUT_ELEMENT_DESC* Layout, uint32_t LayoutSize)
{
    return S_OK;
}

HRESULT FDXDShaderManager::AddVertexShaderAndInputLayout(const std::wstring& FileName, const std::string& EntryPoint, const D3D11_INPUT_ELEMENT_DESC* Layout,
    uint32_t LayoutSize, EViewModeIndex ViewMode, size_t& OutShaderKey)
{
    UINT shaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#ifdef _DEBUG
    shaderFlags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif
    if (DXDDevice == nullptr)
        return S_FALSE;

    HRESULT hr = S_OK;
    ID3DBlob* VertexShaderCSO = nullptr;
    ID3DBlob* ErrorBlob = nullptr;
    const D3D_SHADER_MACRO* Defines = GetShaderMacro(ViewMode);
    size_t shaderKey = ShaderHashUtils::ComputeHashKey(ShaderCompileInfo(FileName, EntryPoint, Defines));

    OutShaderKey = shaderKey;

    if (VertexShaders.Contains(OutShaderKey))
    {
        return S_OK;
    }

    hr = D3DCompileFromFile(FileName.c_str(), Defines, D3D_COMPILE_STANDARD_FILE_INCLUDE, EntryPoint.c_str(), "vs_5_0", shaderFlags, 0, &VertexShaderCSO, &ErrorBlob);
    if (FAILED(hr))
    {
        if (ErrorBlob) {
            OutputDebugStringA((char*)ErrorBlob->GetBufferPointer());
            ErrorBlob->Release();
        }
        return hr;
    }

    ID3D11VertexShader* NewVertexShader;
    hr = DXDDevice->CreateVertexShader(VertexShaderCSO->GetBufferPointer(), VertexShaderCSO->GetBufferSize(), nullptr, &NewVertexShader);
    if (FAILED(hr))
    {
        return hr;
    }

    ID3D11InputLayout* NewInputLayout;
    hr = DXDDevice->CreateInputLayout(Layout, LayoutSize, VertexShaderCSO->GetBufferPointer(), VertexShaderCSO->GetBufferSize(), &NewInputLayout);
    if (FAILED(hr))
    {
        VertexShaderCSO->Release();
        return hr;
    }

    VertexShaders[shaderKey] = NewVertexShader;
    InputLayouts[shaderKey] = NewInputLayout;

    VertexShaderCSO->Release();

    std::size_t currentHash = ShaderHashUtils::ComputeFileHash(FileName);

    FShaderReloadInfo reloadInfo;
    reloadInfo.FileName = FileName;
    reloadInfo.EntryPoint = EntryPoint;
    reloadInfo.ViewMode = ViewMode;
    reloadInfo.FileHash = currentHash;
    reloadInfo.ShaderKey = shaderKey;
    reloadInfo.ShaderType = EShaderType::Vertex;

    ShaderReloadMap[shaderKey] = reloadInfo;

    return S_OK;
}

ID3D11InputLayout* FDXDShaderManager::GetInputLayoutByKey(size_t Key) const
{
    if (InputLayouts.Contains(Key))
    {
        return *InputLayouts.Find(Key);
    }
    return nullptr;
}

ID3D11VertexShader* FDXDShaderManager::GetVertexShaderByKey(size_t Key) const
{
    if (VertexShaders.Contains(Key))
    {
        return *VertexShaders.Find(Key);
    }
    return nullptr;
}

ID3D11PixelShader* FDXDShaderManager::GetPixelShaderByKey(size_t Key) const
{
    if (PixelShaders.Contains(Key))
    {
        ID3D11PixelShader* PixelShader = *PixelShaders.Find(Key);
        PixelShader->AddRef();
        return PixelShader;
    }
    return nullptr;
}

ID3D11ComputeShader* FDXDShaderManager::GetComputeShaderByKey(const std::wstring& Key) const
{
    if (ComputeShaders.Contains(Key))
    {
        return *ComputeShaders.Find(Key);
    }
    return nullptr;
}
