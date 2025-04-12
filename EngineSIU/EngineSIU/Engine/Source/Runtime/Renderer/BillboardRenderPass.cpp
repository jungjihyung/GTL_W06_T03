#include "BillboardRenderPass.h"

#include "D3D11RHI/DXDBufferManager.h"
#include "D3D11RHI/GraphicDevice.h"
#include "D3D11RHI/DXDShaderManager.h"

#include "RendererHelpers.h"

#include "UObject/UObjectIterator.h"
#include "UObject/Casts.h"

#include "UnrealEd/EditorViewportClient.h"
#include "PropertyEditor/ShowFlags.h"

#include "Components/BillboardComponent.h"
#include "Components/ParticleSubUVComponent.h"
#include "Components/TextComponent.h"
#include "Engine/EditorEngine.h"

#include "EngineLoop.h"

#include "World/World.h"

FBillboardRenderPass::FBillboardRenderPass()
    : BufferManager(nullptr)
    , Graphics(nullptr)
    , ShaderManager(nullptr)
    , VertexShader(nullptr)
    , PixelShader(nullptr)
    , InputLayout(nullptr)
    , Stride(0)
{
}

FBillboardRenderPass::~FBillboardRenderPass()
{
    ReleaseShader();
}

void FBillboardRenderPass::Initialize(FDXDBufferManager* InBufferManager, FGraphicsDevice* InGraphics, FDXDShaderManager* InShaderManager)
{
    BufferManager = InBufferManager;
    Graphics = InGraphics;
    ShaderManager = InShaderManager;
    CreateShader();
}

void FBillboardRenderPass::PrepareRender()
{
    BillboardObjs.Empty();
    for (const auto iter : TObjectRange<UBillboardComponent>())
    {
        if (iter->GetWorld() == GEngine->ActiveWorld)
        {
            BillboardObjs.Add(iter);
        }
    }
}

void FBillboardRenderPass::PrepareTextureShader() const
{
    Graphics->DeviceContext->VSSetShader(VertexShader, nullptr, 0);
    Graphics->DeviceContext->PSSetShader(PixelShader, nullptr, 0);
    Graphics->DeviceContext->IASetInputLayout(InputLayout);

    BufferManager->BindConstantBuffer(TEXT("FPerObjectConstantBuffer"), 0, EShaderStage::Vertex);
}

void FBillboardRenderPass::PrepareSubUVConstant() const
{
    BufferManager->BindConstantBuffer(TEXT("FSubUVConstant"), 1, EShaderStage::Vertex);
    BufferManager->BindConstantBuffer(TEXT("FSubUVConstant"), 1, EShaderStage::Pixel);
}

void FBillboardRenderPass::UpdateSubUVConstant(FVector2D uvOffset, FVector2D uvScale) const
{
    FSubUVConstant data;
    data.uvOffset = uvOffset;
    data.uvScale = uvScale;

    BufferManager->UpdateConstantBuffer(TEXT("FSubUVConstant"), data);
}

void FBillboardRenderPass::UpdatePerObjectConstant(const FMatrix& Model, const FMatrix& View, const FMatrix& Projection, const FVector4& UUIDColor, bool Selected) const
{
    FMatrix MVP = RendererHelpers::CalculateMVP(Model, View, Projection);
    FMatrix NormalMatrix = RendererHelpers::CalculateNormalMatrix(Model);
    FPerObjectConstantBuffer data(MVP, NormalMatrix, UUIDColor, Selected);

    BufferManager->UpdateConstantBuffer(TEXT("FPerObjectConstantBuffer"), data);
}

void FBillboardRenderPass::RenderTexturePrimitive(ID3D11Buffer* pVertexBuffer, UINT numVertices, ID3D11Buffer* pIndexBuffer, UINT numIndices, ID3D11ShaderResourceView* TextureSRV, ID3D11SamplerState* SamplerState) const
{
    SetupVertexBuffer(pVertexBuffer, numVertices);

    Graphics->DeviceContext->IASetIndexBuffer(pIndexBuffer, DXGI_FORMAT_R16_UINT, 0);
    Graphics->DeviceContext->PSSetShaderResources(0, 1, &TextureSRV);
    Graphics->DeviceContext->PSSetSamplers(0, 1, &SamplerState);
    Graphics->DeviceContext->DrawIndexed(numIndices, 0, 0);
}

void FBillboardRenderPass::RenderTextPrimitive(ID3D11Buffer* pVertexBuffer, UINT numVertices, ID3D11ShaderResourceView* TextureSRV, ID3D11SamplerState* SamplerState) const
{
    SetupVertexBuffer(pVertexBuffer, numVertices);

    Graphics->DeviceContext->PSSetShaderResources(0, 1, &TextureSRV);
    Graphics->DeviceContext->PSSetSamplers(0, 1, &SamplerState);
    Graphics->DeviceContext->Draw(numVertices, 0);
}

void FBillboardRenderPass::CreateShader()
{
    // Billboard 셰이더 생성
    D3D11_INPUT_ELEMENT_DESC TextureLayoutDesc[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0}
    };

    Stride = sizeof(FVertexTexture);
    size_t TextureShaderKey;
    HRESULT hr = ShaderManager->AddVertexShaderAndInputLayout(L"Shaders/BillboardShader.hlsl", "MainVS",
        TextureLayoutDesc, ARRAYSIZE(TextureLayoutDesc), nullptr, TextureShaderKey);

    hr = ShaderManager->AddPixelShader(L"Shaders/BillboardShader.hlsl", "MainPS", nullptr, PixelShaderKey);

    VertexShader = ShaderManager->GetVertexShaderByKey(TextureShaderKey);
    PixelShader = ShaderManager->GetPixelShaderByKey(PixelShaderKey);
    InputLayout = ShaderManager->GetInputLayoutByKey(TextureShaderKey);
}

void FBillboardRenderPass::ReleaseShader()
{
    FDXDBufferManager::SafeRelease(InputLayout);
    FDXDBufferManager::SafeRelease(PixelShader);
    FDXDBufferManager::SafeRelease(VertexShader);
}

void FBillboardRenderPass::Render(const std::shared_ptr<FEditorViewportClient>& Viewport)
{
    if (!(Viewport->GetShowFlag() & static_cast<uint64>(EEngineShowFlags::SF_BillboardText))) return;

    PrepareTextureShader();

    PrepareSubUVConstant();

    FVertexInfo VertexInfo;
    FIndexInfo IndexInfo;

    BufferManager->GetQuadBuffer(VertexInfo, IndexInfo);

    // 각 Billboard에 대해 렌더링 처리
    for (auto BillboardComp : BillboardObjs)
    {
        FMatrix Model = BillboardComp->CreateBillboardMatrix();
        FVector4 UUIDColor = BillboardComp->EncodeUUID() / 255.0f;

        bool Selected = (BillboardComp == Viewport->GetPickedGizmoComponent());

        UpdatePerObjectConstant(Model, Viewport->GetViewMatrix(), Viewport->GetProjectionMatrix(), UUIDColor, Selected);

        if (UParticleSubUVComponent* SubUVParticle = Cast<UParticleSubUVComponent>(BillboardComp))
        {
            UpdateSubUVConstant(SubUVParticle->GetUVOffset(), SubUVParticle->GetUVScale());

            RenderTexturePrimitive(VertexInfo.VertexBuffer, VertexInfo.NumVertices, IndexInfo.IndexBuffer,
                IndexInfo.NumIndices, SubUVParticle->Texture->TextureSRV, SubUVParticle->Texture->SamplerState);
        }
        else if (UTextComponent* TextComp = Cast<UTextComponent>(BillboardComp))
        {
            FBufferInfo Buffers;
            float Height = TextComp->Texture->Height;
            float Width = TextComp->Texture->Width;
            BufferManager->CreateUnicodeTextBuffer(TextComp->GetText(), Buffers, Width, Height, TextComp->GetColumnCount(), TextComp->GetRowCount());

            UpdateSubUVConstant(FVector2D(), FVector2D(1, 1));

            RenderTextPrimitive(Buffers.VertexInfo.VertexBuffer, Buffers.VertexInfo.NumVertices, TextComp->Texture->TextureSRV, TextComp->Texture->SamplerState);

        }
        else
        {
            UpdateSubUVConstant(FVector2D(BillboardComp->finalIndexU, BillboardComp->finalIndexV), FVector2D(1, 1));

            RenderTexturePrimitive(VertexInfo.VertexBuffer, VertexInfo.NumVertices,
                IndexInfo.IndexBuffer, IndexInfo.NumIndices, BillboardComp->Texture->TextureSRV,
                BillboardComp->Texture->SamplerState);
        }
    }
}
void FBillboardRenderPass::SetupVertexBuffer(ID3D11Buffer* pVertexBuffer, UINT numVertices) const
{
    UINT offset = 0;
    Graphics->DeviceContext->IASetVertexBuffers(0, 1, &pVertexBuffer, &Stride, &offset);
    Graphics->DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}

void FBillboardRenderPass::ClearRenderArr()
{
    BillboardObjs.Empty();
}
