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

size_t FDXDShaderManager::CalculateShaderHashKey(const std::wstring& FileName, const std::string& EntryPoint, const D3D_SHADER_MACRO* Defines)
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

HRESULT FDXDShaderManager::AddVertexShader(const std::wstring& Key, const std::wstring& FileName)
{
    return E_NOTIMPL;
}

HRESULT FDXDShaderManager::AddVertexShader(const std::wstring& FileName, const std::string& EntryPoint, const D3D_SHADER_MACRO* Defines, size_t& OutShaderKey)
{
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
FILETIME GetLastWriteTime(const std::wstring& filename)
{
    FILETIME ftWrite = { 0 };
    WIN32_FILE_ATTRIBUTE_DATA attrData;
    if (GetFileAttributesEx(filename.c_str(), GetFileExInfoStandard, &attrData))
    {
        ftWrite = attrData.ftLastWriteTime;
    }
    return ftWrite;
}
void FDXDShaderManager::CheckAndReloadShaders()
{
    for (auto& pair : ShaderReloadMap)
    {
        size_t shaderKey = pair.Key;
        FShaderReloadInfo& info = pair.Value;

        FILETIME currentWriteTime = GetLastWriteTime(info.FileName);
        // 파일이 수정되었는지 비교
        if (CompareFileTime(&currentWriteTime, &info.LastModified) != 0)
        {
            // 변경 감지됨
            UINT shaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#ifdef _DEBUG
            shaderFlags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

            HRESULT hr = S_OK;
            if (info.ShaderType == EShaderType::Vertex)
            {
                // 기존 버텍스 셰이더 해제
                if (VertexShaders.Contains(shaderKey))
                {
                    VertexShaders[shaderKey]->Release();
                    VertexShaders.Remove(shaderKey);
                }

                ID3DBlob* VsBlob = nullptr;
                hr = D3DCompileFromFile(info.FileName.c_str(), info.Defines, D3D_COMPILE_STANDARD_FILE_INCLUDE,
                    info.EntryPoint.c_str(), "vs_5_0", shaderFlags, 0, &VsBlob, nullptr);
                if (SUCCEEDED(hr))
                {
                    ID3D11VertexShader* NewVertexShader = nullptr;
                    hr = DXDDevice->CreateVertexShader(VsBlob->GetBufferPointer(), VsBlob->GetBufferSize(), nullptr, &NewVertexShader);
                    if (SUCCEEDED(hr))
                    {
                        VertexShaders[shaderKey] = NewVertexShader;
                        // 최신 수정 시간 반영
                        info.LastModified = currentWriteTime;
                    }
                    VsBlob->Release();
                }
                else
                {
                    // 디버깅용 출력: 컴파일 실패 시 에러 정보 출력 가능
                    OutputDebugStringA("Vertex Shader 재컴파일 실패\n");
                }
            }
            else if (info.ShaderType ==  EShaderType::Pixel)
            {
                // 기존 픽셀 셰이더 해제
                if (PixelShaders.Contains(shaderKey))
                {
                    PixelShaders[shaderKey]->Release();
                    PixelShaders.Remove(shaderKey);
                }

                ID3DBlob* PsBlob = nullptr;
                hr = D3DCompileFromFile(info.FileName.c_str(), info.Defines, D3D_COMPILE_STANDARD_FILE_INCLUDE,
                    info.EntryPoint.c_str(), "ps_5_0", shaderFlags, 0, &PsBlob, nullptr);
                if (SUCCEEDED(hr))
                {
                    ID3D11PixelShader* NewPixelShader = nullptr;
                    hr = DXDDevice->CreatePixelShader(PsBlob->GetBufferPointer(), PsBlob->GetBufferSize(), nullptr, &NewPixelShader);
                    if (SUCCEEDED(hr))
                    {
                        PixelShaders[shaderKey] = NewPixelShader;
                        info.LastModified = currentWriteTime;
                    }
                    PsBlob->Release();
                }
                else
                {
                    OutputDebugStringA("Pixel Shader 재컴파일 실패\n");
                }
            }
        }
    }
}

DWORD WINAPI DirectoryChangeWatcher(LPVOID lpParam)
{
    FWatchParams* pParams = reinterpret_cast<FWatchParams*>(lpParam);
    FDXDShaderManager* pShaderManager = pParams->pShaderManager;
    std::wstring directory = pParams->Directory;
    delete pParams;  // 할당받은 파라미터 메모리 해제

    // 디렉터리 감시를 위해 핸들 생성. FILE_FLAG_BACKUP_SEMANTICS는 디렉터리 접근에 필요.
    HANDLE hDir = CreateFileW(
        directory.c_str(),
        FILE_LIST_DIRECTORY,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        NULL,
        OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED,
        NULL);

    if (hDir == INVALID_HANDLE_VALUE)
    {
        OutputDebugStringA("디렉토리 감시 핸들 생성 실패\n");
        return 1;
    }

    // 내부 버퍼 (충분한 크기로 설정)
    const DWORD bufferSize = 4096;
    BYTE buffer[bufferSize];
    DWORD bytesReturned = 0;

    OVERLAPPED overlapped = {};
    // overlapped 이벤트 객체 생성 (비동기 처리를 위한)
    overlapped.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

    while (true)
    {
        // 디렉터리 내의 수정 이벤트 감시 (여기서는 FILE_NOTIFY_CHANGE_LAST_WRITE)
        BOOL success = ReadDirectoryChangesW(
            hDir,
            buffer,
            bufferSize,
            TRUE,  // 하위 디렉터리까지 감시할 경우 TRUE
            FILE_NOTIFY_CHANGE_LAST_WRITE,
            &bytesReturned,
            &overlapped,
            NULL);

        if (success)
        {
            DWORD waitStatus = WaitForSingleObject(overlapped.hEvent, 1000);
            if (waitStatus == WAIT_OBJECT_0)
            {
                // 이벤트 발생 시 셰이더 재컴파일 호출
                pShaderManager->CheckAndReloadShaders();

                // overlapped 객체 재설정
                ResetEvent(overlapped.hEvent);
            }
        }
        // 너무 빠른 루프를 막기 위해 짧은 Sleep 적용 (필요에 따라 조정)
        Sleep(50);
    }

    // 스레드 종료 시 자원 해제 (실제로 while문을 탈출하는 구조가 필요하면 처리)
    CloseHandle(overlapped.hEvent);
    CloseHandle(hDir);
    return 0;
}
