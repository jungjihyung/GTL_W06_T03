//#define MAX_LIGHTS 256
//#define TILE_SIZE 16
//#define MAX_LIGHTS_PER_TILE 256
//#define MAX_LIGHTS 256

//#define POINT_LIGHT         1
//#define SPOT_LIGHT          2
//#define DIRECTIONAL_LIGHT   3

//struct LIGHT
//{
//    float3 m_cBaseColor;
//    float pad3;

//    float3 m_vPosition;
//    float PositionPad;

//    float3 m_vDirection;
//    float pad4;

//    float m_fAttenuation;       // 거리 기반 감쇠 계수
//    float m_fIntensity;         // 광원 강도
//    float m_fAttRadius;         // 감쇠 반경 (Attenuation Radius)
//    float m_fInnerConeAngle;
    
//    float m_fOuterConeAngle;
//    float m_fFalloff; // 스팟라이트의 감쇠 인자 (스포트 각도에 따른 감쇠)
//    float2 OuterConeAnglePadding;
    
//    int m_bEnable;
//    int m_nType;
//    int2 Padding;
//};

//StructuredBuffer<uint> VisibleLightIndices : register(t2);
//Buffer<uint> LightIndexCount : register(t3);

//StructuredBuffer<LIGHT> gLights : register(t4); // 광원 정보

//cbuffer cbLights : register(b2)
//{
//    //LIGHT gLights[MAX_LIGHTS];
//    float4 gcGlobalAmbientLight;
//    int gnLights;
//    float3 padCB;
//};


//cbuffer ScreenConstants : register(b6)
//{
//    uint2 ScreenSize; // 전체 화면 크기 (w, h)
//    float2 ScreenUVOffset; // 뷰포트 시작 UV (x/sw, y/sh)
//    float2 UVScale; // 뷰포트 크기 비율 (w/sw, h/sh)
//    float2 Padding;
//};

//float4 CalcLight(int nIndex, float3 vPosition, float3 vNormal)
//{
//    float4 litColor = float4(0, 0, 0, 1);
    
//    if (!gLights[nIndex].m_bEnable)
//    {
//        return litColor;
//    }
    
//    if (gLights[nIndex].m_nType == POINT_LIGHT)
//    {
//        float3 vToLight = gLights[nIndex].m_vPosition - vPosition;
//        float fDistance = length(vToLight);

//        // 감쇠 반경을 벗어나면 기여하지 않음
//        if (fDistance > gLights[nIndex].m_fAttRadius)
//        {
//            return float4(0.0f, 0.0f, 0.0f, 1.0f);
//        }
    
//        float fSpecularFactor = 0.0f;
//        vToLight /= fDistance; // 정규화
        
//        float fDiffuseFactor = max(dot(vNormal, vToLight), 0);
        

//        float3 litResult = float3(0, 0, 0);
//        float fAttenuationFactor = 1.0f / (1.0f + gLights[nIndex].m_fAttenuation * fDistance * fDistance);
   
//#if LIGHTING_MODEL_LAMBERT
//    litResult  = Material.m_cBaseColor.rgb + gLights[nIndex].m_cBaseColor.rgb * fDiffuseFactor * Material.DiffuseColor.rgb;
//#else   
//        if (fDiffuseFactor > 0.0f)
//        {
//            float3 vView = normalize(CameraPosition - vPosition);
//            float3 vHalf = normalize(vToLight + vView);
//            fSpecularFactor = pow(max(dot(normalize(vNormal), vHalf), 0.0f), 1);
//        }

    
//        litResult = (gcGlobalAmbientLight * Material.AmbientColor.rgb) +
//                 (gLights[nIndex].m_cBaseColor.rgb * fDiffuseFactor * Material.DiffuseColor) +
//                 (gLights[nIndex].m_cBaseColor.rgb * fSpecularFactor * Material.SpecularColor);
//#endif
//        return float4(litResult * fAttenuationFactor * gLights[nIndex].m_fIntensity, 1.0f);
//    }
//    else if (gLights[nIndex].m_nType == SPOT_LIGHT)
//    {
//        float3 vToLight = gLights[nIndex].m_vPosition - vPosition;
//        float fDistance = length(vToLight);
//        if (fDistance < gLights[nIndex].m_fAttRadius)
//        {
//            vToLight /= fDistance;
//            float fDiffuseFactor = saturate(dot(vNormal, vToLight));
//            float fSpecularFactor = 0.0;
//            if (fDiffuseFactor > 0.0)
//            {
//                float3 vView = normalize(CameraPosition - vPosition);
//                float3 vHalf = normalize(vToLight + vView);
//                fSpecularFactor = pow(saturate(dot(vNormal, vHalf)), Material.SpecularScalar);
//            }
            
//            float fAngleCos = dot(-vToLight, normalize(gLights[nIndex].m_vDirection));
//            if (fAngleCos > 0.0)
//            {
//                float fSpotFactor = pow(fAngleCos, gLights[nIndex].m_fFalloff);
//                float fAttenuationFactor = 1.0 / (1.0 + gLights[nIndex].m_fAttenuation * fDistance * fDistance);
                
//                litColor = float4(
//                    (gcGlobalAmbientLight.rgb * Material.AmbientColor.rgb) +
//                    (gLights[nIndex].m_cBaseColor.rgb * fDiffuseFactor * Material.DiffuseColor.rgb) +
//                    (gLights[nIndex].m_cBaseColor.rgb * fSpecularFactor * Material.SpecularColor.rgb),
//                    1.0
//                );
//                litColor *= fAttenuationFactor * fSpotFactor * gLights[nIndex].m_fIntensity;
//            }
//        }
//    }
//    else if (gLights[nIndex].m_nType == DIRECTIONAL_LIGHT)
//    {
//        float3 lightDir = normalize(-gLights[nIndex].m_vDirection);
//        float fDiffuseFactor = saturate(dot(vNormal, lightDir));
//        float fSpecularFactor = 0.0;
//        if (fDiffuseFactor > 0.0)
//        {
//            float3 vView = normalize(CameraPosition - vPosition);
//            float3 vHalf = normalize(lightDir + vView);
//            fSpecularFactor = pow(saturate(dot(vNormal, vHalf)), Material.SpecularScalar);
//        }
        
//        litColor = float4(
//            (Material.AmbientColor.rgb) +
//            (gLights[nIndex].m_cBaseColor.rgb * fDiffuseFactor * Material.DiffuseColor.rgb) +
//            (gLights[nIndex].m_cBaseColor.rgb * fSpecularFactor * Material.SpecularColor.rgb),
//            1.0
//        );
//        litColor *= gLights[nIndex].m_fIntensity;
//    }
    
//    return litColor;
//}
//float4 Lighting(float3 vPosition, float3 vNormal)
//{
//    float4 cColor = float4(0.0, 0.0, 0.0, 0.0);
//    [unroll(16)]
//    for (int i = 0; i < gnLights; i++)
//    {
//        cColor += CalcLight(i, vPosition, vNormal);
//    }
    
//    cColor += gcGlobalAmbientLight;
//    cColor.a = 1.0;
    
//    return cColor;
//}


//float4 CalculateTileBasedLighting(uint2 screenPos, float3 worldPos, float3 normal)
//{
//    float4 result = gcGlobalAmbientLight;
    
//    // 현재 픽셀의 타일 id 계산
//    uint tilesPerRow = (ScreenSize.x + TILE_SIZE - 1) / TILE_SIZE;
    
//    uint tileX = screenPos.x / TILE_SIZE;
//    uint tileY = screenPos.y / TILE_SIZE;
    
//    uint tileIndex = tileY * tilesPerRow + tileX;
    
//    // 2. 해당 타일의 가시광원 수 조회
//    uint lightCount = LightIndexCount.Load(tileIndex);
//    lightCount = min(lightCount, MAX_LIGHTS_PER_TILE);
    
//    // 3. 가시광원만 선택적으로 라이팅 계산
//    uint baseIndex = tileIndex * MAX_LIGHTS_PER_TILE;
//    for (uint i = 0; i < lightCount; ++i)
//    {
//        uint lightIdx = VisibleLightIndices.Load(baseIndex + i);
//        result += CalcLight(lightIdx, worldPos, normal);
//    }
    
//    result += gcGlobalAmbientLight;
//    result.a = 1.0;
//        litResult = (gLights[nIndex].m_cBaseColor.rgb * fDiffuseFactor * Material.DiffuseColor) +
//                 (gLights[nIndex].m_cBaseColor.rgb * fSpecularFactor * Material.SpecularColor);
//#endif
//        return float4(litResult * fAttenuationFactor * gLights[nIndex].m_fIntensity, 1.0f);
//    }
//    else if (gLights[nIndex].m_nType == SPOT_LIGHT)
//    {
//        float3 vToLight = gLights[nIndex].m_vPosition - vPosition;
//        float fDistance = length(vToLight);
//        if (fDistance < gLights[nIndex].m_fAttRadius)
//        {
//            vToLight /= fDistance;
//            float fDiffuseFactor = saturate(dot(vNormal, vToLight));
//            float fSpecularFactor = 0.0;
//            if (fDiffuseFactor > 0.0)
//            {
//                float3 vView = normalize(CameraPosition - vPosition);
//                float3 vHalf = normalize(vToLight + vView);
//                fSpecularFactor = pow(saturate(dot(vNormal, vHalf)), Material.SpecularScalar);
//            }
            
//            float fAngleCos = dot(-vToLight, normalize(gLights[nIndex].m_vDirection));
//            float cosInner = cos(gLights[nIndex].m_fInnerConeAngle * 0.0174533f);
//            float cosOuter = cos(gLights[nIndex].m_fOuterConeAngle * 0.0174533f);
            
//                if (fAngleCos > 0.0)
//            {
//                //float fSpotFactor = pow(fAngleCos, gLights[nIndex].m_fFalloff);
//                float fSpotFactor = saturate((fAngleCos - cosOuter) / (cosInner - cosOuter));
//                float fAttenuationFactor = 1.0 / (1.0 + gLights[nIndex].m_fAttenuation * fDistance * fDistance);
                
//                litColor = float4(
//                    (gLights[nIndex].m_cBaseColor.rgb * fDiffuseFactor * Material.DiffuseColor.rgb) +
//                    (gLights[nIndex].m_cBaseColor.rgb * fSpecularFactor * Material.SpecularColor.rgb),
//                    1.0
//                );
//                litColor *= fAttenuationFactor * fSpotFactor * gLights[nIndex].m_fIntensity;
//            }
//        }
//    }
//    else if (gLights[nIndex].m_nType == DIRECTIONAL_LIGHT)
//    {
//        float3 lightDir = normalize(-gLights[nIndex].m_vDirection);
//        float fDiffuseFactor = saturate(dot(vNormal, lightDir));
//        //if (fDiffuseFactor < 0.f)
//        //    return float4(0, 0, 0, 1);
        
//        float fSpecularFactor = 0.0;
//        if (fDiffuseFactor > 0.0)
//        {
//            float3 vView = normalize(CameraPosition - vPosition);
//            float3 vHalf = normalize(lightDir + vView);
//            fSpecularFactor = pow(saturate(dot(vNormal, vHalf)), Material.SpecularScalar *0.3);
//            fSpecularFactor *= fDiffuseFactor;

//        }
        
//        litColor = float4(
//            //Material.AmbientColor.rgb+
//            (gLights[nIndex].m_cBaseColor.rgb * fDiffuseFactor) +
//            (gLights[nIndex].m_cBaseColor.rgb * fSpecularFactor),
//            1.0f
//        );
//        //litColor *= gLights[nIndex].m_fIntensity;
//    }
    
//    return result;
//}


//    return litColor;
//}
//float4 Lighting(float3 vPosition, float3 vNormal)
//{
    
//    //float4 cColor = (0.2, 0.2, 0.2, 0.2);
//    float4 cColor = float4(gcGlobalAmbientLight.rgb * Material.AmbientColor.rgb, 1.0f);
//    [unroll(MAX_LIGHTS)]
//    for (int i = 0; i < gnLights; i++)
//    {
//        cColor += CalcLight(i, vPosition, vNormal);
//    }
    
//    cColor += gcGlobalAmbientLight;
//    cColor.a = 1.0f;
    
//    return cColor;
//}
