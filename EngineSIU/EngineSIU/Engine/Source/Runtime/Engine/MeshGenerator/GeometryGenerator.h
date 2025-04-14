#pragma once
#include <Define.h>
#include <DirectXMath.h>

class GeoMetryGenerator {
public:
    static void CreateSphere(float radius, int sliceCount, int stackCount, TArray<FStaticMeshVertex>& vertices, TArray<UINT>& indices);

};
