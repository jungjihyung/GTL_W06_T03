// ============================================================================
// UPrimitiveDrawBatch.cpp
// ============================================================================

#include "PrimitiveDrawBatch.h"
#include "EngineLoop.h"
#include "D3D11RHI/GraphicDevice.h"
#include "D3D11RHI/DXDBufferManager.h"
#include "UnrealEd/EditorViewportClient.h"


UPrimitiveDrawBatch::~UPrimitiveDrawBatch()
{
    if (VertexBuffer)
    {
        VertexBuffer->Release();
        VertexBuffer = nullptr;
    }
    ReleaseOBBBuffers();
    ReleaseBoundingBoxBuffers();
    ReleaseConeBuffers();
}

// 2. 초기화 및 릴리즈 함수
void UPrimitiveDrawBatch::Initialize(FGraphicsDevice* graphics, FDXDBufferManager* InBufferManager)
{
    Graphics = graphics;
    BufferManager = InBufferManager;
    InitializeGrid(5, 5000);
}

void UPrimitiveDrawBatch::ReleaseResources()
{
    // VertexBuffer도 릴리즈
    if (VertexBuffer)
    {
        VertexBuffer->Release();
        VertexBuffer = nullptr;
    }
    ReleaseOBBBuffers();
    ReleaseBoundingBoxBuffers();
    ReleaseConeBuffers();
}

// 3. 그리드 및 배치 준비 관련 함수
void UPrimitiveDrawBatch::InitializeGrid(float Spacing, int GridCount)
{
    GridParameters.GridSpacing = Spacing;
    GridParameters.NumGridLines = GridCount;
    GridParameters.GridOrigin = { 0, 0, 0 };
}

void UPrimitiveDrawBatch::PrepareBatch(FLinePrimitiveBatchArgs& OutLinePrimitiveBatchArgs)
{
    InitializeVertexBuffer();
    UpdateGridConstantBuffer(GridParameters);
    UpdateBoundingBoxBuffers();
    UpdateConeBuffers();
    UpdateOBBBuffers();
    UpdateSphereBuffers();

    int BoundingBoxSize = BoundingBoxes.Num();
    int ConeSize = Cones.Num();
    int OBBSize = OrientedBoundingBoxes.Num();
    int SphereSize = Spheres.Num();

    UpdateLinePrimitiveCountBuffer(BoundingBoxSize, ConeSize, SphereSize);

    OutLinePrimitiveBatchArgs.GridParam = GridParameters;
    OutLinePrimitiveBatchArgs.VertexBuffer = VertexBuffer;
    OutLinePrimitiveBatchArgs.BoundingBoxCount = BoundingBoxSize;
    OutLinePrimitiveBatchArgs.ConeCount = ConeSize;
    OutLinePrimitiveBatchArgs.OBBCount = OBBSize;
    OutLinePrimitiveBatchArgs.SphereCount = SphereSize;
    OutLinePrimitiveBatchArgs.SphereSegmentCount = SphereSegmentCount;
}

void UPrimitiveDrawBatch::RemoveArr()
{
    BoundingBoxes.Empty();
    Cones.Empty();
    OrientedBoundingBoxes.Empty();
    Spheres.Empty();
}

// 4. 버퍼 초기화 및 업데이트
void UPrimitiveDrawBatch::InitializeVertexBuffer()
{
    if (!VertexBuffer)
        VertexBuffer = CreateStaticVertexBuffer();
}

void UPrimitiveDrawBatch::UpdateGridConstantBuffer(const FGridParameters& GridParams) const
{
    BufferManager->UpdateConstantBuffer(TEXT("FGridParameters"), GridParams);
}

void UPrimitiveDrawBatch::UpdateBoundingBoxBuffers()
{
    if (BoundingBoxes.Num() > AllocatedBoundingBoxCapacity)
    {
        AllocatedBoundingBoxCapacity = BoundingBoxes.Num();
        ReleaseBoundingBoxBuffers();
        BoundingBoxBuffer = CreateBoundingBoxBuffer(static_cast<UINT>(AllocatedBoundingBoxCapacity));
        BoundingBoxSRV = CreateBoundingBoxSRV(BoundingBoxBuffer, static_cast<UINT>(AllocatedBoundingBoxCapacity));
    }
    if (BoundingBoxBuffer && BoundingBoxSRV)
    {
        int BoundingBoxCount = BoundingBoxes.Num();
        UpdateBoundingBoxBuffer(BoundingBoxBuffer, BoundingBoxes, BoundingBoxCount);
    }
}

void UPrimitiveDrawBatch::UpdateConeBuffers()
{
    if (Cones.Num() > AllocatedConeCapacity)
    {
        AllocatedConeCapacity = Cones.Num();
        ReleaseConeBuffers();
        ConesBuffer = CreateConeBuffer(static_cast<UINT>(AllocatedConeCapacity));
        ConeSRV = CreateConeSRV(ConesBuffer, static_cast<UINT>(AllocatedConeCapacity));
    }
    if (ConesBuffer && ConeSRV)
    {
        int ConeCount = Cones.Num();
        UpdateConesBuffer(ConesBuffer, Cones, ConeCount);
    }
}

void UPrimitiveDrawBatch::UpdateSphereBuffers()
{
    if (Spheres.Num() > AllocatedSphereCapacity)
    {
        AllocatedSphereCapacity = Spheres.Num();
        ReleaseSphereBuffers();
        SpheresBuffer = CreateSphereBuffer(static_cast<UINT>(AllocatedSphereCapacity));
        SphereSRV = CreateSphereSRV(SpheresBuffer, static_cast<UINT>(AllocatedSphereCapacity));
    }
    if (SpheresBuffer && SphereSRV) {
        int SphereCount = Spheres.Num();
        UpdateSpheresBuffer(SpheresBuffer, Spheres, SphereCount);
    }
}

void UPrimitiveDrawBatch::UpdateOBBBuffers()
{
    if (OrientedBoundingBoxes.Num() > AllocatedOBBCapacity)
    {
        AllocatedOBBCapacity = OrientedBoundingBoxes.Num();
        ReleaseOBBBuffers();
        OBBBuffer = CreateOBBBuffer(static_cast<UINT>(AllocatedOBBCapacity));
        OBBSRV = CreateOBBSRV(OBBBuffer, static_cast<UINT>(AllocatedOBBCapacity));
    }
    if (OBBBuffer && OBBSRV)
    {
        int OBBCount = OrientedBoundingBoxes.Num();
        UpdateOBBBuffer(OBBBuffer, OrientedBoundingBoxes, OBBCount);
    }
}

void UPrimitiveDrawBatch::UpdateLinePrimitiveCountBuffer(int NumBoundingBoxes, int NumCones, int NumSphere) const
{
    FPrimitiveCounts Data{};
    Data.BoundingBoxCount = NumBoundingBoxes;
    Data.ConeCount = NumCones;
    Data.SphereCount = NumSphere;

    BufferManager->UpdateConstantBuffer(TEXT("FPrimitiveCounts"), Data);
}

// 5. 릴리즈 함수들
void UPrimitiveDrawBatch::ReleaseBoundingBoxBuffers()
{
    if (BoundingBoxBuffer)
    {
        BoundingBoxBuffer->Release();
        BoundingBoxBuffer = nullptr;
    }
    if (BoundingBoxSRV)
    {
        BoundingBoxSRV->Release();
        BoundingBoxSRV = nullptr;
    }
}

void UPrimitiveDrawBatch::ReleaseConeBuffers()
{
    if (ConesBuffer)
    {
        ConesBuffer->Release();
        ConesBuffer = nullptr;
    }
    if (ConeSRV)
    {
        ConeSRV->Release();
        ConeSRV = nullptr;
    }
}

void UPrimitiveDrawBatch::ReleaseOBBBuffers()
{
    if (OBBBuffer)
    {
        OBBBuffer->Release();
        OBBBuffer = nullptr;
    }
    if (OBBSRV)
    {
        OBBSRV->Release();
        OBBSRV = nullptr;
    }
}

void UPrimitiveDrawBatch::ReleaseSphereBuffers()
{
    if (SpheresBuffer)
    {
        SpheresBuffer->Release();
        SpheresBuffer = nullptr;
    }
    if (SphereSRV) {
        SphereSRV->Release();
        SphereSRV = nullptr;
    }
}

// 6. 프리미티브 렌더링 관련 함수
void UPrimitiveDrawBatch::AddAABBToBatch(const FBoundingBox& LocalAABB, const FVector& Center, const FMatrix& ModelMatrix)
{
    FVector LocalVertices[8] = {
        { LocalAABB.min.X, LocalAABB.min.Y, LocalAABB.min.Z },
        { LocalAABB.max.X, LocalAABB.min.Y, LocalAABB.min.Z },
        { LocalAABB.min.X, LocalAABB.max.Y, LocalAABB.min.Z },
        { LocalAABB.max.X, LocalAABB.max.Y, LocalAABB.min.Z },
        { LocalAABB.min.X, LocalAABB.min.Y, LocalAABB.max.Z },
        { LocalAABB.max.X, LocalAABB.min.Y, LocalAABB.max.Z },
        { LocalAABB.min.X, LocalAABB.max.Y, LocalAABB.max.Z },
        { LocalAABB.max.X, LocalAABB.max.Y, LocalAABB.max.Z }
    };

    FVector WorldVertices[8];
    WorldVertices[0] = Center + FMatrix::TransformVector(LocalVertices[0], ModelMatrix);

    FVector Min = WorldVertices[0], Max = WorldVertices[0];

    for (int i = 1; i < 8; ++i)
    {
        WorldVertices[i] = Center + FMatrix::TransformVector(LocalVertices[i], ModelMatrix);
        Min.X = (WorldVertices[i].X < Min.X) ? WorldVertices[i].X : Min.X;
        Min.Y = (WorldVertices[i].Y < Min.Y) ? WorldVertices[i].Y : Min.Y;
        Min.Z = (WorldVertices[i].Z < Min.Z) ? WorldVertices[i].Z : Min.Z;
        Max.X = (WorldVertices[i].X > Max.X) ? WorldVertices[i].X : Max.X;
        Max.Y = (WorldVertices[i].Y > Max.Y) ? WorldVertices[i].Y : Max.Y;
        Max.Z = (WorldVertices[i].Z > Max.Z) ? WorldVertices[i].Z : Max.Z;
    }
    FBoundingBox BoundingBox;
    BoundingBox.min = Min;
    BoundingBox.max = Max;
    BoundingBoxes.Add(BoundingBox);
}

void UPrimitiveDrawBatch::AddOBBToBatch(const FBoundingBox& LocalAABB, const FVector& Center, const FMatrix& ModelMatrix)
{
    FVector LocalVertices[8] = {
        { LocalAABB.min.X, LocalAABB.min.Y, LocalAABB.min.Z },
        { LocalAABB.max.X, LocalAABB.min.Y, LocalAABB.min.Z },
        { LocalAABB.min.X, LocalAABB.max.Y, LocalAABB.min.Z },
        { LocalAABB.max.X, LocalAABB.max.Y, LocalAABB.min.Z },
        { LocalAABB.min.X, LocalAABB.min.Y, LocalAABB.max.Z },
        { LocalAABB.max.X, LocalAABB.min.Y, LocalAABB.max.Z },
        { LocalAABB.min.X, LocalAABB.max.Y, LocalAABB.max.Z },
        { LocalAABB.max.X, LocalAABB.max.Y, LocalAABB.max.Z }
    };

    FOBB OBB;
    for (int i = 0; i < 8; ++i)
    {
        OBB.corners[i] = Center + FMatrix::TransformVector(LocalVertices[i], ModelMatrix);
    }
    OrientedBoundingBoxes.Add(OBB);
}

void UPrimitiveDrawBatch::AddConeToBatch(const FVector& Center, float Height, float OuterAngle, const FVector4& Color, const FMatrix& ModelMatrix)
{
    FVector LocalApex = FVector(0, 0, 0);
    FCone Cone;

    Cone.ConeApex = Center + FMatrix::TransformVector(LocalApex, ModelMatrix);
    FVector LocalBaseCenter = FVector(Height, 0, 0);
    Cone.ConeBaseCenter = Center + FMatrix::TransformVector(LocalBaseCenter, ModelMatrix);

    Cone.ConeHeight = (Cone.ConeBaseCenter - Cone.ConeApex).Length();
    Cone.Color = Color;

    FVector LocalUpVector = FVector(0, 0, 1);
    Cone.ConeUpVector = FMatrix::TransformVector(LocalUpVector, ModelMatrix);

    Cone.OuterAngle = OuterAngle;

    Cones.Add(Cone);
}
void UPrimitiveDrawBatch::AddSpehreToBatch(const FVector& Center, float Radius, FVector4 Color, int Segments)
{
    SphereSegmentCount = Segments;
    FSphere Sphere;
    Sphere.Center = Center;
    Sphere.Radius = Radius;
    Sphere.Color = Color;
    Sphere.SegmentCount = Segments;

    Spheres.Add(Sphere);
}


ID3D11Buffer* UPrimitiveDrawBatch::CreateStaticVertexBuffer() const
{
    FSimpleVertex Vertices[2] = { {}, {} };

    D3D11_BUFFER_DESC VBDesc = {};
    VBDesc.Usage = D3D11_USAGE_DEFAULT;
    VBDesc.ByteWidth = sizeof(Vertices);
    VBDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    VBDesc.CPUAccessFlags = 0;
    D3D11_SUBRESOURCE_DATA VBInitData = {};
    VBInitData.pSysMem = Vertices;
    ID3D11Buffer* Buffer = nullptr;
    HRESULT HR = Graphics->Device->CreateBuffer(&VBDesc, &VBInitData, &Buffer);
    return Buffer;
}

ID3D11Buffer* UPrimitiveDrawBatch::CreateBoundingBoxBuffer(UINT NumBoundingBoxes) const
{
    D3D11_BUFFER_DESC BufferDesc;
    BufferDesc.Usage = D3D11_USAGE_DYNAMIC;
    BufferDesc.ByteWidth = sizeof(FBoundingBox) * NumBoundingBoxes;
    BufferDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    BufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    BufferDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
    BufferDesc.StructureByteStride = sizeof(FBoundingBox);

    ID3D11Buffer* Buffer = nullptr;
    Graphics->Device->CreateBuffer(&BufferDesc, nullptr, &Buffer);
    return Buffer;
}

ID3D11Buffer* UPrimitiveDrawBatch::CreateOBBBuffer(UINT NumBoundingBoxes) const
{
    D3D11_BUFFER_DESC BufferDesc;
    BufferDesc.Usage = D3D11_USAGE_DYNAMIC;
    BufferDesc.ByteWidth = sizeof(FOBB) * NumBoundingBoxes;
    BufferDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    BufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    BufferDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
    BufferDesc.StructureByteStride = sizeof(FOBB);

    ID3D11Buffer* Buffer = nullptr;
    Graphics->Device->CreateBuffer(&BufferDesc, nullptr, &Buffer);
    return Buffer;
}

ID3D11Buffer* UPrimitiveDrawBatch::CreateConeBuffer(UINT NumCones) const
{
    D3D11_BUFFER_DESC BufferDesc;
    BufferDesc.Usage = D3D11_USAGE_DYNAMIC;
    BufferDesc.ByteWidth = sizeof(FCone) * NumCones;
    BufferDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    BufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    BufferDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
    BufferDesc.StructureByteStride = sizeof(FCone);

    ID3D11Buffer* Buffer = nullptr;
    Graphics->Device->CreateBuffer(&BufferDesc, nullptr, &Buffer);
    return Buffer;
}

ID3D11Buffer* UPrimitiveDrawBatch::CreateSphereBuffer(UINT NumSpheres) const
{
    D3D11_BUFFER_DESC BufferDesc;
    BufferDesc.Usage = D3D11_USAGE_DYNAMIC;
    BufferDesc.ByteWidth = sizeof(FSphere) * NumSpheres;
    BufferDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    BufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    BufferDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
    BufferDesc.StructureByteStride = sizeof(FSphere);

    ID3D11Buffer* Buffer = nullptr;
    Graphics->Device->CreateBuffer(&BufferDesc, nullptr, &Buffer);
    return Buffer;
}

// 8. SRV 생성 함수들
ID3D11ShaderResourceView* UPrimitiveDrawBatch::CreateBoundingBoxSRV(ID3D11Buffer* Buffer, UINT NumBoundingBoxes)
{
    D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc;
    SRVDesc.Format = DXGI_FORMAT_UNKNOWN;
    SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
    SRVDesc.Buffer.ElementOffset = 0;
    SRVDesc.Buffer.NumElements = NumBoundingBoxes;

    Graphics->Device->CreateShaderResourceView(Buffer, &SRVDesc, &BoundingBoxSRV);
    return BoundingBoxSRV;
}

ID3D11ShaderResourceView* UPrimitiveDrawBatch::CreateOBBSRV(ID3D11Buffer* Buffer, UINT NumBoundingBoxes)
{
    D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc;
    SRVDesc.Format = DXGI_FORMAT_UNKNOWN;
    SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
    SRVDesc.Buffer.ElementOffset = 0;
    SRVDesc.Buffer.NumElements = NumBoundingBoxes;

    Graphics->Device->CreateShaderResourceView(Buffer, &SRVDesc, &OBBSRV);
    return OBBSRV;
}

ID3D11ShaderResourceView* UPrimitiveDrawBatch::CreateConeSRV(ID3D11Buffer* Buffer, UINT NumCones)
{
    D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc;
    SRVDesc.Format = DXGI_FORMAT_UNKNOWN;
    SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
    SRVDesc.Buffer.ElementOffset = 0;
    SRVDesc.Buffer.NumElements = NumCones;

    Graphics->Device->CreateShaderResourceView(Buffer, &SRVDesc, &ConeSRV);
    return ConeSRV;
}

ID3D11ShaderResourceView* UPrimitiveDrawBatch::CreateSphereSRV(ID3D11Buffer* Buffer, UINT NumSpheres)
{
    D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc;
    SRVDesc.Format = DXGI_FORMAT_UNKNOWN;
    SRVDesc.ViewDimension = D3D10_1_SRV_DIMENSION_BUFFER;
    SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
    SRVDesc.Buffer.ElementOffset = 0;
    SRVDesc.Buffer.NumElements = NumSpheres;

    Graphics->Device->CreateShaderResourceView(Buffer, &SRVDesc, &SphereSRV);
    return SphereSRV;
}

// 9. 버퍼 업데이트 (데이터 복사) 함수들
void UPrimitiveDrawBatch::UpdateBoundingBoxBuffer(ID3D11Buffer* Buffer, const TArray<FBoundingBox>& BoundingBoxes, int NumBoundingBoxes) const
{
    if (!Buffer)
        return;
    D3D11_MAPPED_SUBRESOURCE MappedResource;
    Graphics->DeviceContext->Map(Buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource);
    auto Data = static_cast<FBoundingBox*>(MappedResource.pData);
    for (int i = 0; i < BoundingBoxes.Num(); ++i)
    {
        Data[i] = BoundingBoxes[i];
    }
    Graphics->DeviceContext->Unmap(Buffer, 0);
}

void UPrimitiveDrawBatch::UpdateOBBBuffer(ID3D11Buffer* Buffer, const TArray<FOBB>& OBBs, int NumOBBs) const
{
    if (!Buffer)
        return;
    D3D11_MAPPED_SUBRESOURCE MappedResource;
    Graphics->DeviceContext->Map(Buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource);
    auto Data = static_cast<FOBB*>(MappedResource.pData);
    for (int i = 0; i < OBBs.Num(); ++i)
    {
        Data[i] = OBBs[i];
    }
    Graphics->DeviceContext->Unmap(Buffer, 0);
}

void UPrimitiveDrawBatch::UpdateConesBuffer(ID3D11Buffer* Buffer, const TArray<FCone>& Cones, int NumCones) const
{
    if (!Buffer)
        return;
    D3D11_MAPPED_SUBRESOURCE MappedResource;
    Graphics->DeviceContext->Map(Buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource);
    auto Data = static_cast<FCone*>(MappedResource.pData);
    for (int i = 0; i < Cones.Num(); ++i)
    {
        Data[i] = Cones[i];
    }
    Graphics->DeviceContext->Unmap(Buffer, 0);
}

void UPrimitiveDrawBatch::UpdateSpheresBuffer(ID3D11Buffer* Buffer, const TArray<FSphere>& Spheres, int NumSpheres) const
{
    if (!Buffer)
        return;
    D3D11_MAPPED_SUBRESOURCE MappedResource;
    Graphics->DeviceContext->Map(Buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource);
    auto Data = static_cast<FSphere*>(MappedResource.pData);
    for (int i = 0; i < Spheres.Num(); ++i) {
        Data[i] = Spheres[i];
    }
    Graphics->DeviceContext->Unmap(Buffer, 0);
}

void UPrimitiveDrawBatch::PrepareLineResources() const
{
    if (Graphics && Graphics->DeviceContext)
    {

        // BoundingBox, Cone, OBB 데이터 SRV를 각각 등록 (registers 2, 3, 4)
        Graphics->DeviceContext->VSSetShaderResources(2, 1, &BoundingBoxSRV);
        Graphics->DeviceContext->VSSetShaderResources(3, 1, &ConeSRV);
        Graphics->DeviceContext->VSSetShaderResources(4, 1, &OBBSRV);
        Graphics->DeviceContext->VSSetShaderResources(5, 1, &SphereSRV);
    }
}
