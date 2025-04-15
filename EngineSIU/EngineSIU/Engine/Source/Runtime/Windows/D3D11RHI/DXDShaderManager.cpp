#include "DXDShaderManager.h"
#include "D3D11RHI/GraphicDevice.h"
#include <sstream>
#include <functional>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <Windows.h>
#include <fstream>
#include <sstream>

// 익명 네임스페이스 내에 유틸리티 함수들을 정의
namespace {
    // 파일 내용을 읽어 std::hash<std::string>()를 사용하여 해시값 계산
    std::size_t ComputeFileHash(const std::wstring& filename)
    {
        std::ifstream file(filename, std::ios::binary);
        if (!file)
            return 0;
        std::ostringstream contents;
        contents << file.rdbuf();
        std::string fileContent = contents.str();
        return std::hash<std::string>()(fileContent);
    }

    // 셰이더 컴파일 공통 함수
    HRESULT CompileShader(const std::wstring& FileName,
        const D3D_SHADER_MACRO* Defines,
        const std::string& EntryPoint,
        const std::string& Target,
        ID3DBlob** OutBlob)
    {
        UINT shaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#ifdef _DEBUG
        shaderFlags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif
        return D3DCompileFromFile(FileName.c_str(), Defines, D3D_COMPILE_STANDARD_FILE_INCLUDE,
            EntryPoint.c_str(), Target.c_str(), shaderFlags, 0, OutBlob, nullptr);
    }

    HRESULT CompileShaderWithTempFile(const std::wstring& originalFile, const D3D_SHADER_MACRO* Defines, const std::string& EntryPoint,
        const std::string& ShaderModel, UINT shaderFlags, ID3DBlob** blobOut)
    {
        std::wstring tempFile;
        size_t pos = originalFile.find_last_of(L"\\/");
        if (pos != std::wstring::npos)
        {
            tempFile = originalFile.substr(0, pos + 1) + L"temp_" + originalFile.substr(pos + 1);
        }
        else
        {
            tempFile = L"temp_" + originalFile;
        }

        const int maxRetry = 30;
        int retry = 0;
        BOOL copySuccess = FALSE;
        while (retry < maxRetry)
        {
            copySuccess = CopyFileW(originalFile.c_str(), tempFile.c_str(), FALSE);

            if (copySuccess)
                break;

            retry++;

            Sleep(10); // 50ms 지연 후 재시도
        }

        if (!copySuccess)
        {
            OutputDebugStringA("임시 파일 복사 실패\n");
            return E_FAIL;
        }

        ID3DBlob* errorBlob = nullptr;
        HRESULT hr = D3DCompileFromFile(tempFile.c_str(), Defines, D3D_COMPILE_STANDARD_FILE_INCLUDE, EntryPoint.c_str(), ShaderModel.c_str(), shaderFlags, 0, blobOut, &errorBlob);
        DeleteFileW(tempFile.c_str());


        if (FAILED(hr))
        {
            if (errorBlob)
            {
                OutputDebugStringA((char*)errorBlob->GetBufferPointer());
                errorBlob->Release();
            }
            return hr;
        }

        return hr;
    }

    HRESULT ReloadShader(ID3D11Device* Device,
        const std::wstring& FileName,
        const D3D_SHADER_MACRO* Defines,
        const std::string& EntryPoint,
        const std::string& Target,
        void** NewShader)
    {
        if (Device == nullptr)
        {
            OutputDebugStringA("Device 포인터가 NULL입니다.\n");
            return E_INVALIDARG;
        }
        if (NewShader == nullptr)
        {
            OutputDebugStringA("NewShader 포인터가 NULL입니다.\n");
            return E_INVALIDARG;
        }

        ID3DBlob* shaderBlob = nullptr;
        HRESULT hr = CompileShader(FileName, Defines, EntryPoint, Target, &shaderBlob);
        if (FAILED(hr))
        {
            if (shaderBlob)
            {
                OutputDebugStringA("Shader 컴파일에 실패하였습니다.\n");
                shaderBlob->Release();
            }
            return hr;
        }

        if (Target == "vs_5_0")
        {
            hr = Device->CreateVertexShader(shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize(), nullptr,
                reinterpret_cast<ID3D11VertexShader**>(NewShader));
        }
        else if (Target == "ps_5_0")
        {
            hr = Device->CreatePixelShader(shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize(), nullptr,
                reinterpret_cast<ID3D11PixelShader**>(NewShader));
        }
        else
        {
            OutputDebugStringA("알 수 없는 타겟입니다: ");
            OutputDebugStringA(Target.c_str());
            OutputDebugStringA("\n");
            hr = E_INVALIDARG;
        }
        shaderBlob->Release();
        return hr;
    }
}

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

size_t FDXDShaderManager::CalculateShaderHashKey(const std::wstring& FileName, const std::string& EntryPoint, const D3D_SHADER_MACRO* Defines)
{
    std::wstringstream ss;
    ss << FileName << L"_" << std::wstring(EntryPoint.begin(), EntryPoint.end());

    if (Defines == nullptr)
        return std::hash<std::wstring>()(ss.str());

    for (int i = 0; Defines[i].Name != nullptr; ++i)
    {
        ss << L"_" << std::wstring(Defines[i].Name, Defines[i].Name + strlen(Defines[i].Name));
        ss << L"_" << std::wstring(Defines[i].Definition, Defines[i].Definition + strlen(Defines[i].Definition));
    }

    std::wstring combinedStr = ss.str();
    return std::hash<std::wstring>()(combinedStr);
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
        return DefineUnLit;
    case EViewModeIndex::VMI_WorldNormal:
        return DefineWorldNormal;
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
    size_t shaderKey = CalculateShaderHashKey(FileName, EntryPoint, Defines);

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
    std::size_t currentHash = ComputeFileHash(FileName);
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
    size_t shaderKey = CalculateShaderHashKey(FileName, EntryPoint, Defines);
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

    std::size_t currentHash = ComputeFileHash(FileName);
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
    size_t shaderKey = CalculateShaderHashKey(FileName, EntryPoint, Defines);

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

    std::size_t currentHash = ComputeFileHash(FileName);

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

// 파일 내용의 해시값을 사용해 변경 여부를 판단하는 핫리로드 함수
void FDXDShaderManager::CheckAndReloadShaders()
{
    for (auto& pair : ShaderReloadMap)
    {
        FShaderReloadInfo& info = pair.Value;

        // 현재 파일 해시값 계산
        std::size_t currentHash = ComputeFileHash(info.FileName);
        // 해시값이 변경되었으면 재컴파일 수행
        if (currentHash != info.FileHash)
        {
            UINT shaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#ifdef _DEBUG
            shaderFlags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

            HRESULT hr = S_OK;
            if (info.ShaderType == EShaderType::Vertex)
            {

                ID3DBlob* VsBlob = nullptr;
                const D3D_SHADER_MACRO* Defines = GetShaderMacro(info.ViewMode);
                hr = CompileShaderWithTempFile(info.FileName, Defines, info.EntryPoint, "vs_5_0", shaderFlags, &VsBlob);
                if (SUCCEEDED(hr))
                {
                    ID3D11VertexShader* NewVertexShader = nullptr;
                    hr = DXDDevice->CreateVertexShader(VsBlob->GetBufferPointer(), VsBlob->GetBufferSize(), nullptr, &NewVertexShader);
                    if (SUCCEEDED(hr))
                    {
                        // 기존 버텍스 셰이더 해제
                        if (VertexShaders.Contains(info.ShaderKey))
                        {
                            VertexShaders[info.ShaderKey]->Release();
                            VertexShaders.Remove(info.ShaderKey);
                        }
                        VertexShaders[info.ShaderKey] = NewVertexShader;
                        info.FileHash = currentHash;
                        GraphicDevice->DeviceContext->VSSetShader(NewVertexShader, nullptr, 0);
                    }
                    else
                    {
                        MessageBox(nullptr, L"Failed to Create Vertex Shader!", L"Error", MB_ICONERROR | MB_OK);
                    }
                    VsBlob->Release();
                }
                else
                {
                    MessageBox(nullptr, L"Failed to Compile Vertex Shader!", L"Error", MB_ICONERROR | MB_OK);
                    OutputDebugStringA("Vertex Shader 재컴파일 실패\n");
                }
            }
            else if (info.ShaderType == EShaderType::Pixel)
            {

                ID3DBlob* PsBlob = nullptr;
                const D3D_SHADER_MACRO* Defines = GetShaderMacro(info.ViewMode);
                hr = CompileShaderWithTempFile(info.FileName, Defines, info.EntryPoint, "ps_5_0", shaderFlags, &PsBlob);

                if (SUCCEEDED(hr))
                {
                    ID3D11PixelShader* NewPixelShader = nullptr;
                    hr = DXDDevice->CreatePixelShader(PsBlob->GetBufferPointer(), PsBlob->GetBufferSize(), nullptr, &NewPixelShader);
                    if (SUCCEEDED(hr) && NewPixelShader)
                    {

                        // 기존 픽셀 셰이더 해제
                        if (PixelShaders.Contains(info.ShaderKey))
                        {
                            PixelShaders[info.ShaderKey]->Release();
                            PixelShaders.Remove(info.ShaderKey);
                        }

                        PixelShaders[info.ShaderKey] = NewPixelShader;
                        info.FileHash = currentHash;
                    }
                    else
                    {
                        MessageBox(nullptr, L"Failed to Create Pixel Shader!", L"Error", MB_ICONERROR | MB_OK);
                    }
                    PsBlob->Release();
                }
                else
                {
                    MessageBox(nullptr, L"Failed to Compile Pixel Shader!", L"Error", MB_ICONERROR | MB_OK);
                    OutputDebugStringA("Pixel Shader 재컴파일 실패\n");
                }
            }
        }
    }
}
