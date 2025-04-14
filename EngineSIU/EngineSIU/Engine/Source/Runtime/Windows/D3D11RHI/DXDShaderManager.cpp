#include "DXDShaderManager.h"


FDXDShaderManager::FDXDShaderManager(ID3D11Device* Device)
    : DXDDevice(Device)
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

size_t FDXDShaderManager::ComputeShaderHash(const std::wstring& FileName, const std::string& EntryPoint, const D3D_SHADER_MACRO* Defines)
{
    std::wstringstream ss;
    ss << FileName << L"_" << std::wstring(EntryPoint.begin(), EntryPoint.end());
   
    if (Defines == nullptr)
        return std::hash<std::wstring>()(ss.str());

    
    for (int i = 0; Defines[i].Name != nullptr; ++i)
    {
        if (Defines[i].Name == nullptr)
            break;
        ss << L"_" << std::wstring(Defines[i].Name, Defines[i].Name + strlen(Defines[i].Name));
        ss << L"_" << std::wstring(Defines[i].Definition, Defines[i].Definition + strlen(Defines[i].Definition));
    }

    std::wstring combinedStr = ss.str();
    return std::hash<std::wstring>()(combinedStr);
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

HRESULT FDXDShaderManager::AddVertexShader(const std::wstring& FileName, const std::string& EntryPoint, const D3D_SHADER_MACRO* Defines, size_t& OutShaderKey)
{
    size_t shaderKey = ComputeShaderHash(FileName, EntryPoint, Defines);
   
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

    return S_OK;
}


HRESULT FDXDShaderManager::AddPixelShader(const std::wstring& FileName, const std::string& EntryPoint, const D3D_SHADER_MACRO* Defines, size_t& OutShaderKey)
{
    // 고유 해시 키 계산
    size_t shaderKey = ComputeShaderHash(FileName, EntryPoint, Defines);
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
    hr = D3DCompileFromFile(FileName.c_str(), Defines, D3D_COMPILE_STANDARD_FILE_INCLUDE,
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

    PixelShaders[shaderKey] = NewPixelShader;

    return S_OK;
}

HRESULT FDXDShaderManager::AddInputLayout(const std::wstring& Key, const D3D11_INPUT_ELEMENT_DESC* Layout, uint32_t LayoutSize)
{
    return S_OK;
}

HRESULT FDXDShaderManager::AddVertexShaderAndInputLayout(const std::wstring& FileName, const std::string& EntryPoint, const D3D11_INPUT_ELEMENT_DESC* Layout, 
                                                         uint32_t LayoutSize, const D3D_SHADER_MACRO* Defines, size_t& OutShaderKey)
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

    size_t shaderKey = ComputeShaderHash(FileName, EntryPoint, Defines);

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
