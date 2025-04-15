#pragma once
#pragma comment(lib, "Shlwapi.lib")
#include "Define.h"
#include <d3d11.h>
#include <d3dcompiler.h>
#include <Shlwapi.h>

class FDXDShaderManager;

class FShaderHotReload
{
public:
    FShaderHotReload() = default;
    FShaderHotReload(FDXDShaderManager* InShaderManager);
    void CreateShaderHotReloadThread();
    void CheckAndReloadShaders();
private:
    FDXDShaderManager* ShaderManager;
};

namespace {

    struct FWatchParams
    {
        FShaderHotReload* pShaderHotReload;
        std::wstring Directory;
    };
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

        if (!PathFileExistsW(tempFile.c_str()))
        {
            const int maxRetry = 5;
            int retry = 0;
            BOOL copySuccess = FALSE;
            const int maxRetries = 5;
            const int delayMillis = 1000; // 1초 간격
            for (int i = 0; i < maxRetries; ++i)
            {
                copySuccess = CopyFileW(originalFile.c_str(), tempFile.c_str(), FALSE);
                if (copySuccess)
                    break;
                // 파일 사용중일 가능성이 있으므로 지연 후 재시도
                Sleep(delayMillis);
            }
            if (!copySuccess)
            {
                // 실패 시 오류 코드 출력
                DWORD error = GetLastError();
                // error 코드에 대한 처리 로직 추가
            }
        }
        else
        {
            OutputDebugStringA("임시 파일이 이미 존재함. 기존 파일로 컴파일 진행\n");
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

    DWORD WINAPI DirectoryChangeWatcher(LPVOID lpParam)
    {
        FWatchParams* pParams = reinterpret_cast<FWatchParams*>(lpParam);

        FShaderHotReload* pShaderHotReload = pParams->pShaderHotReload;
        std::wstring directory = pParams->Directory;
        delete pParams;

        HANDLE hDir = CreateFileW(
            directory.c_str(),
            FILE_LIST_DIRECTORY,
            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
            NULL,
            OPEN_EXISTING,
            FILE_FLAG_BACKUP_SEMANTICS,
            NULL);

        if (hDir == INVALID_HANDLE_VALUE)
        {
            OutputDebugStringA("디렉토리 감시 핸들 생성 실패\n");
            return 1;
        }

        const DWORD bufferSize = 4096;
        BYTE buffer[bufferSize];
        DWORD bytesReturned = 0;

        while (true)
        {
            BOOL success = ReadDirectoryChangesW(
                hDir,
                buffer,
                bufferSize,
                TRUE,
                FILE_NOTIFY_CHANGE_LAST_WRITE,
                &bytesReturned,
                NULL,
                NULL);

            if (success)
            {
                pShaderHotReload->CheckAndReloadShaders();
            }
            else
            {
                OutputDebugStringA("ReadDirectoryChangesW 실패\n");
            }
            Sleep(50);
        }

        CloseHandle(hDir);
        return 0;
    }
}
