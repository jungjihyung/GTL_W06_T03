
#define SPHERE_SEGMENT_COUNT 32
#define CONE_SEGMENT_COUNT 32

cbuffer MatrixBuffer : register(b0)
{
    row_major float4x4 MVP;
};

cbuffer GridParametersData : register(b1)
{
    float GridSpacing;
    float3 GridSpacingPad;
    float3 GridOrigin; // Grid의 중심
    float OriginPad;
    int GridCount; // 총 grid 라인 수
    int Padding[3];
};
cbuffer CameraConstants : register(b2)
{
    row_major float4x4 View;
    row_major float4x4 Projection;
    row_major float4x4 InvProjection;
    float3 CameraPosition;
    float cameraPad1;
    
    float nearPlane;
    float farPlane;
    float2 cameraPad2;
};

cbuffer PrimitiveCounts : register(b3)
{
    int BoundingBoxCount; // 렌더링할 AABB의 개수
    int pad;
    int ConeCount; // 렌더링할 cone의 개수
    int pad1;
    int SphereCount;
    float3 PrimitivePadding;
};

struct FBoundingBoxData
{
    float3 bbMin;
    float padding0;
    float3 bbMax;
    float padding1;
};
struct FConeData
{
    float3 ConeApex; // 원뿔의 꼭짓점
    float ConeRadius; // 원뿔 밑면 반지름
    
    float3 ConeBaseCenter; // 원뿔 밑면 중심
    float ConeHeight; // 원뿔 높이 (Apex와 BaseCenter 간 차이)
    
    float4 Color;
    
    float3 ConeUpVector;
    float ConeUpVectorPad;
    
    float OuterAngle;
    float3 OuterAnglePad;
};
struct FOrientedBoxCornerData
{
    float3 corners[8]; // 회전/이동 된 월드 공간상의 8꼭짓점
};

struct FSphereData
{
    float3 Center;
    float Radius;
    float4 Color;
    int SegmentCount;
    float pad[3];
};

StructuredBuffer<FBoundingBoxData> g_BoundingBoxes : register(t2);
StructuredBuffer<FConeData> g_ConeData : register(t3);
StructuredBuffer<FOrientedBoxCornerData> g_OrientedBoxes : register(t4);
StructuredBuffer<FSphereData> g_SphereData : register(t5);

static const int BB_EdgeIndices[12][2] =
{
    { 0, 1 },
    { 1, 3 },
    { 3, 2 },
    { 2, 0 }, // 앞면
    { 4, 5 },
    { 5, 7 },
    { 7, 6 },
    { 6, 4 }, // 뒷면
    { 0, 4 },
    { 1, 5 },
    { 2, 6 },
    { 3, 7 } // 측면
};

struct VS_INPUT
{
    uint vertexID : SV_VertexID; // 0 또는 1: 각 라인의 시작과 끝
    uint instanceID : SV_InstanceID; // 인스턴스 ID로 grid, axis, bounding box를 구분
};

struct PS_INPUT
{
    float4 Position : SV_Position;
    float4 WorldPosition : POSITION;
    float4 Color : COLOR;
    uint instanceID : SV_InstanceID;
};

/////////////////////////////////////////////////////////////////////////
// Grid 위치 계산 함수
/////////////////////////////////////////////////////////////////////////
float3 ComputeGridPosition(uint instanceID, uint vertexID)
{
    int halfCount = GridCount / 2;
    float centerOffset = halfCount * 0.5; // grid 중심이 원점에 오도록

    float3 startPos;
    float3 endPos;
    
    if (instanceID < halfCount)
    {
        // 수직선: X 좌표 변화, Y는 -centerOffset ~ +centerOffset
        float x = GridOrigin.x + (instanceID - centerOffset) * GridSpacing;
        if (abs(x - GridOrigin.x) < 0.001)
        {
            startPos = float3(0, 0, 0);
            endPos = float3(0, (GridOrigin.y - centerOffset * GridSpacing), 0);
        }
        else
        {
            startPos = float3(x, GridOrigin.y - centerOffset * GridSpacing, GridOrigin.z);
            endPos = float3(x, GridOrigin.y + centerOffset * GridSpacing, GridOrigin.z);
        }
    }
    else
    {
        // 수평선: Y 좌표 변화, X는 -centerOffset ~ +centerOffset
        int idx = instanceID - halfCount;
        float y = GridOrigin.y + (idx - centerOffset) * GridSpacing;
        if (abs(y - GridOrigin.y) < 0.001)
        {
            startPos = float3(0, 0, 0);
            endPos = float3(-(GridOrigin.x + centerOffset * GridSpacing), 0, 0);
        }
        else
        {
            startPos = float3(GridOrigin.x - centerOffset * GridSpacing, y, GridOrigin.z);
            endPos = float3(GridOrigin.x + centerOffset * GridSpacing, y, GridOrigin.z);
        }

    }
    return (vertexID == 0) ? startPos : endPos;
}

/////////////////////////////////////////////////////////////////////////
// Axis 위치 계산 함수 (총 3개: X, Y, Z)
/////////////////////////////////////////////////////////////////////////
float3 ComputeAxisPosition(uint axisInstanceID, uint vertexID)
{
    float3 start = float3(0.0, 0.0, 0.0);
    float3 end;
    float zOffset = 0.f;
    if (axisInstanceID == 0)
    {
        // X 축: 빨간색
        end = float3(1000000.0, 0.0, zOffset);
    }
    else if (axisInstanceID == 1)
    {
        // Y 축: 초록색
        end = float3(0.0, 1000000.0, zOffset);
    }
    else if (axisInstanceID == 2)
    {
        // Z 축: 파란색
        end = float3(0.0, 0.0, 1000000.0 + zOffset);
    }
    else
    {
        end = start;
    }
    return (vertexID == 0) ? start : end;
}

/////////////////////////////////////////////////////////////////////////
// Bounding Box 위치 계산 함수
// bbInstanceID: StructuredBuffer에서 몇 번째 bounding box인지
// edgeIndex: 해당 bounding box의 12개 엣지 중 어느 것인지
/////////////////////////////////////////////////////////////////////////
float3 ComputeBoundingBoxPosition(uint bbInstanceID, uint edgeIndex, uint vertexID)
{
    FBoundingBoxData box = g_BoundingBoxes[bbInstanceID];
  
//    0: (bbMin.x, bbMin.y, bbMin.z)
//    1: (bbMax.x, bbMin.y, bbMin.z)
//    2: (bbMin.x, bbMax.y, bbMin.z)
//    3: (bbMax.x, bbMax.y, bbMin.z)
//    4: (bbMin.x, bbMin.y, bbMax.z)
//    5: (bbMax.x, bbMin.y, bbMax.z)
//    6: (bbMin.x, bbMax.y, bbMax.z)
//    7: (bbMax.x, bbMax.y, bbMax.z)
    int vertIndex = BB_EdgeIndices[edgeIndex][vertexID];
    float x = ((vertIndex & 1) == 0) ? box.bbMin.x : box.bbMax.x;
    float y = ((vertIndex & 2) == 0) ? box.bbMin.y : box.bbMax.y;
    float z = ((vertIndex & 4) == 0) ? box.bbMin.z : box.bbMax.z;
    return float3(x, y, z);
}

/////////////////////////////////////////////////////////////////////////
// Axis 위치 계산 함수 (총 3개: X, Y, Z)
/////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////
// Cone 계산 함수
/////////////////////////////////////////////////
// Helper: 계산된 각도에 따른 밑면 꼭짓점 위치 계산

float3 ComputeConePosition(uint globalInstanceID, uint vertexID)
{
    
    uint coneIndex = globalInstanceID / (2 * CONE_SEGMENT_COUNT);
    uint lineIndex = globalInstanceID % (2 * CONE_SEGMENT_COUNT);
    
    // cone 데이터 읽기
    FConeData cone = g_ConeData[coneIndex];
    
    // cone의 축 계산
    float3 axis = normalize(cone.ConeApex - cone.ConeBaseCenter);
    
    // CPU에서 전달된 ConeUpVector를 사용해, axis에 수직인 두 벡터 u, v 계산
    float3 suppliedUp = normalize(cone.ConeUpVector);
    float3 u = normalize(suppliedUp - dot(suppliedUp, axis) * axis);
    float3 v = cross(axis, u);
    
    // OuterAngle을 이용해 효과적인 밑면 반지름 계산.
    float effectiveRadius = cone.ConeHeight * tan(cone.OuterAngle * 0.0174533f);
    
    if (lineIndex < (uint) CONE_SEGMENT_COUNT)
    {
        // 측면 선분: cone의 꼭짓점과 밑면의 한 점을 잇는다.
        float angle = lineIndex * 6.28318530718 / CONE_SEGMENT_COUNT;
        float3 baseVertex = cone.ConeBaseCenter + (cos(angle) * u + sin(angle) * v) * effectiveRadius;
        return (vertexID == 0) ? cone.ConeApex : baseVertex;
    }
    else
    {
        // 밑면 둘레 선분: 밑면상의 인접한 두 점을 잇는다.
        uint idx = lineIndex - CONE_SEGMENT_COUNT;
        float angle0 = idx * 6.28318530718 / CONE_SEGMENT_COUNT;
        float angle1 = ((idx + 1) % CONE_SEGMENT_COUNT) * 6.28318530718 / CONE_SEGMENT_COUNT;
        float3 v0 = cone.ConeBaseCenter + (cos(angle0) * u + sin(angle0) * v) * effectiveRadius;
        float3 v1 = cone.ConeBaseCenter + (cos(angle1) * u + sin(angle1) * v) * effectiveRadius;
        return (vertexID == 0) ? v0 : v1;
    }
}
/////////////////////////////////////////////////////////////////////////
// OBB
/////////////////////////////////////////////////////////////////////////
float3 ComputeOrientedBoxPosition(uint obIndex, uint edgeIndex, uint vertexID)
{
    FOrientedBoxCornerData ob = g_OrientedBoxes[obIndex];
    int cornerID = BB_EdgeIndices[edgeIndex][vertexID];
    return ob.corners[cornerID];
}

/////////////////////////////////////////////////////////////////////////
// Sphere
/////////////////////////////////////////////////////////////////////////
float3 ComputeSpherePosition(uint globalInstanceID, uint vertexID)
{
    FSphereData sphere = g_SphereData[globalInstanceID / (3 * SPHERE_SEGMENT_COUNT)];
    
    uint localLineIndex = globalInstanceID % (3 * sphere.SegmentCount);
    uint ringType = localLineIndex / sphere.SegmentCount;
    uint segment = localLineIndex % sphere.SegmentCount;
    
    float angle0 = segment * 6.283183 / sphere.SegmentCount;
    float angle1 = ((segment + 1) % sphere.SegmentCount) * 6.283183 / sphere.SegmentCount;
    
    float3 p0, p1;
    
    if (ringType == 0)
    {
        // XY 평면
        p0 = float3(cos(angle0), sin(angle0), 0);
        p1 = float3(cos(angle1), sin(angle1), 0);
    }
    else if (ringType ==1)
    {
        // YZ 평면
        p0 = float3(0, cos(angle0), sin(angle0));
        p1 = float3(0, cos(angle1), sin(angle1));
    }
    else
    {
        // ZX 평면
        p0 = float3(cos(angle0), 0, sin(angle0));
        p1 = float3(cos(angle1), 0, sin(angle1));
    }
    
    return (vertexID == 0) ? sphere.Center + p0 * sphere.Radius : sphere.Center + p1 * sphere.Radius;
}
/////////////////////////////////////////////////////////////////////////
// 메인 버텍스 셰이더
/////////////////////////////////////////////////////////////////////////
PS_INPUT mainVS(VS_INPUT input)
{
    PS_INPUT output;
    float3 pos;
    float4 color;
    
    uint sphereSegmentCount = g_SphereData[0].SegmentCount;
    uint sphereInstCnt = 3 * sphereSegmentCount;
    
    // Cone 하나당 (2 * SegmentCount) 선분.
    // ConeCount 개수만큼이므로 총 (2 * SegmentCount * ConeCount).
    uint coneInstCnt = ConeCount * 2 * CONE_SEGMENT_COUNT;

    // Grid / Axis / AABB 인스턴스 개수 계산
    uint gridLineCount = GridCount; // 그리드 라인
    uint axisCount = 3; // X, Y, Z 축 (월드 좌표축)
    uint aabbInstanceCount = 12 * BoundingBoxCount; // AABB 하나당 12개 엣지

    uint coneStart = gridLineCount + axisCount + aabbInstanceCount;
    uint sphereStart = coneStart + coneInstCnt;
    uint obbStart = sphereStart + sphereInstCnt;


    // 이제 instanceID를 기준으로 분기
    if (input.instanceID < gridLineCount)
    {
        // 0 ~ (GridCount-1): 그리드
        pos = ComputeGridPosition(input.instanceID, input.vertexID);
        color = float4(0.1, 0.1, 0.1, 1.0);
    }
    else if (input.instanceID < gridLineCount + axisCount)
    {
        // 그 다음 (axisCount)개: 축(Axis)
        uint axisInstanceID = input.instanceID - gridLineCount;
        pos = ComputeAxisPosition(axisInstanceID, input.vertexID);

        // 축마다 색상
        if (axisInstanceID == 0)
            color = float4(1.0, 0.0, 0.0, 1.0); // X: 빨강
        else if (axisInstanceID == 1)
            color = float4(0.0, 1.0, 0.0, 1.0); // Y: 초록
        else
            color = float4(0.0, 0.0, 1.0, 1.0); // Z: 파랑
    }
    else if (input.instanceID < gridLineCount + axisCount + aabbInstanceCount)
    {
        // 그 다음 AABB 인스턴스 구간
        uint index = input.instanceID - (gridLineCount + axisCount);
        uint bbInstanceID = index / 12; // 12개가 1박스
        uint bbEdgeIndex = index % 12;
        
        pos = ComputeBoundingBoxPosition(bbInstanceID, bbEdgeIndex, input.vertexID);
        color = float4(1.0, 1.0, 1.0, 1.0);
    }
    else if (input.instanceID < sphereStart)
    {
        // 그 다음 콘(Cone) 구간 - 인스턴스 한 개로 가정
        uint coneInstanceID = input.instanceID - coneStart;
        pos = ComputeConePosition(coneInstanceID, input.vertexID);
        color = g_ConeData[0].Color;
    }
    else if (input.instanceID < obbStart)
    {
        // Sphere 구간 - 인스턴스 한 개로 가정
        uint sphereLineID = input.instanceID - sphereStart;
        pos = ComputeSpherePosition(sphereLineID, input.vertexID);
        color = g_SphereData[0].Color;
    }
    else
    {
        // OBB
        uint obbLocalID = input.instanceID - obbStart;
        uint obbIndex = obbLocalID / 12;
        uint edgeIndex = obbLocalID % 12;

        pos = ComputeOrientedBoxPosition(obbIndex, edgeIndex, input.vertexID);
        color = float4(0.4, 1.0, 0.4, 1.0);
    }

    // 출력 변환
    output.WorldPosition = float4(pos, 1.0);
    output.Position = mul(float4(pos, 1.0), MVP);
    output.Color = color;
    output.instanceID = input.instanceID;
    return output;
}

float4 mainPS(PS_INPUT input) : SV_Target
{
    if (input.instanceID < GridCount || input.instanceID < GridCount + 3)
    {
        float Dist = length(input.WorldPosition.xyz - CameraPosition);

        float MaxDist = 300 * 1.2f;
        float MinDist = MaxDist * 0.3f;

         // Fade out grid
        float Fade = saturate(1.f - (Dist - MinDist) / (MaxDist - MinDist));
        input.Color.a *= Fade * Fade * Fade;
    }
    return input.Color;
}
