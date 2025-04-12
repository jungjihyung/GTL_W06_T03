#include "Runtime/Engine/MeshGenerator/GeometryGenerator.h"

void GeoMetryGenerator::CreateSphere(float radius, int sliceCount, int stackCount, TArray<FStaticMeshVertex>& vertices, TArray<UINT>& indices)
{
    vertices.Empty();
    indices.Empty();

    FStaticMeshVertex newVertex;
    newVertex.X = 0.0f;
    newVertex.Y = radius;
    newVertex.Z = 0.0f;
    // TODO : 컬러 추가

    vertices.Add(newVertex);

    for (int i = 1; i <= stackCount - 1; i++) {
        float phi = DirectX::XM_PI * i / stackCount;
        for (int j = 0; j <= sliceCount; j++) {
            float theta = 2.0f * DirectX::XM_PI * j / sliceCount;

            FStaticMeshVertex newVertex2;
            newVertex2.X = radius * sinf(phi) * cosf(theta);
            newVertex2.Y = radius * cosf(phi);
            newVertex2.Z = radius * sinf(phi) * sinf(theta);
            vertices.Add(newVertex2);
        }
    }

    FStaticMeshVertex newVertex3;
    newVertex3.X = 0.0f;
    newVertex3.Y = -radius;
    newVertex3.Z = 0.0f;
    vertices.Add(newVertex3);

    for (int i = 1; i <= sliceCount; i++)
    {
        indices.Add(0);
        indices.Add(i);
        indices.Add(i + 1);
    }

    // 인덱스 버퍼: 중간 영역
    int baseIndex = 1;
    int ringVertexCount = sliceCount + 1;
    for (int i = 0; i < stackCount - 2; i++)
    {
        for (int j = 0; j < sliceCount; j++)
        {
            indices.Add(baseIndex + i * ringVertexCount + j);
            indices.Add(baseIndex + i * ringVertexCount + j + 1);
            indices.Add(baseIndex + (i + 1) * ringVertexCount + j);

            indices.Add(baseIndex + (i + 1) * ringVertexCount + j);
            indices.Add(baseIndex + i * ringVertexCount + j + 1);
            indices.Add(baseIndex + (i + 1) * ringVertexCount + j + 1);
        }
    }

    // 인덱스 버퍼: bottom cap
    int southPoleIndex = (UINT)vertices.Num() - 1;
    baseIndex = southPoleIndex - ringVertexCount;
    for (int i = 0; i < sliceCount; i++)
    {
        indices.Add(southPoleIndex);
        indices.Add(baseIndex + i + 1);
        indices.Add(baseIndex + i);
    }

}
