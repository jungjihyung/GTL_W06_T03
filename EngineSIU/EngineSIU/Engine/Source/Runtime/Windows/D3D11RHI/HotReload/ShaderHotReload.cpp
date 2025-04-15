#include "ShaderHotReload.h"
#include "ShaderHashUtils.h"
#include "D3D11RHI/DXDShaderManager.h"
#include "D3D11RHI/GraphicDevice.h"

FShaderHotReload::FShaderHotReload(FDXDShaderManager* InShaderManager) :ShaderManager(InShaderManager)
{

}

void FShaderHotReload::CreateShaderHotReloadThread()
{
    FWatchParams* pParams = new FWatchParams();
    pParams->pShaderHotReload = this;
    pParams->Directory = L"Shaders";

    HANDLE hThread = CreateThread(nullptr, 0, DirectoryChangeWatcher, pParams, 0, nullptr);
}

void FShaderHotReload::CheckAndReloadShaders()
{
    for (auto& pair : ShaderManager->ShaderReloadMap)
    {
        FShaderReloadInfo& info = pair.Value;

        // 현재 파일 해시값 계산
        std::size_t currentHash = ShaderHashUtils::ComputeFileHash(info.FileName);
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
                const D3D_SHADER_MACRO* Defines = ShaderManager->GetShaderMacro(info.ViewMode);
                hr = CompileShaderWithTempFile(info.FileName, Defines, info.EntryPoint, "vs_5_0", shaderFlags, &VsBlob);
                if (SUCCEEDED(hr))
                {
                    ID3D11VertexShader* NewVertexShader = nullptr;
                    hr = ShaderManager->DXDDevice->CreateVertexShader(VsBlob->GetBufferPointer(), VsBlob->GetBufferSize(), nullptr, &NewVertexShader);
                    if (SUCCEEDED(hr) && NewVertexShader)
                    {
                        // 기존 버텍스 셰이더 해제
                        if (ShaderManager->VertexShaders.Contains(info.ShaderKey))
                        {
                            ShaderManager->VertexShaders[info.ShaderKey]->Release();
                            ShaderManager->VertexShaders.Remove(info.ShaderKey);
                        }
                        ShaderManager->VertexShaders[info.ShaderKey] = NewVertexShader;
                        info.FileHash = currentHash;
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
                const D3D_SHADER_MACRO* Defines = ShaderManager->GetShaderMacro(info.ViewMode);
                hr = CompileShaderWithTempFile(info.FileName, Defines, info.EntryPoint, "ps_5_0", shaderFlags, &PsBlob);

                if (SUCCEEDED(hr))
                {
                    ID3D11PixelShader* NewPixelShader = nullptr;
                    hr = ShaderManager->DXDDevice->CreatePixelShader(PsBlob->GetBufferPointer(), PsBlob->GetBufferSize(), nullptr, &NewPixelShader);
                    if (SUCCEEDED(hr) && NewPixelShader)
                    {

                        // 기존 픽셀 셰이더 해제
                        if (ShaderManager->PixelShaders.Contains(info.ShaderKey))
                        {
                            ShaderManager->PixelShaders[info.ShaderKey]->Release();
                            ShaderManager->PixelShaders.Remove(info.ShaderKey);
                        }

                        ShaderManager->PixelShaders[info.ShaderKey] = NewPixelShader;
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
