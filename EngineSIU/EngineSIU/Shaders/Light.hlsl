// light.hlsl

#define MAX_LIGHTS 16 

#define POINT_LIGHT         1
#define SPOT_LIGHT          2

struct LIGHT
{
     float3 m_cDiffuse;
    float pad2;

    float3 m_cSpecular;
    float pad3;

    float3 m_vPosition;
    float m_fFalloff; // 스팟라이트의 감쇠 인자

    float3 m_vDirection;
    float pad4;

    float m_fAttenuation; // 거리 기반 감쇠 계수
    int m_bEnable;
    int m_nType;
    float m_fIntensity; // 광원 강도
    
    float m_fAttRadius; // 감쇠 반경 (Attenuation Radius)
    float3 LightPad;
};

cbuffer cbLights : register(b2)
{
    LIGHT gLights[MAX_LIGHTS];
    float4 gcGlobalAmbientLight;
    int gnLights;
    float3 padCB;
};

float4 SpotLight(int nIndex, float3 vPosition, float3 vNormal)
{
    // 광원과 픽셀 위치 간 벡터 계산
    float3 vToLight = gLights[nIndex].m_vPosition - vPosition;
    float fDistance = length(vToLight);

    // 감쇠 반경을 벗어나면 기여하지 않음
    if (fDistance > gLights[nIndex].m_fAttRadius)
    {
        return float4(0.0f, 0.0f, 0.0f, 0.0f);
    }

    vToLight /= fDistance; // 정규화
    
    float fDiffuseFactor = saturate(dot(vNormal, vToLight));

    // 정반사
    float fSpecularFactor = 0.0f;
    if (fDiffuseFactor > 0.0f)
    {
        float3 vView = normalize(CameraPosition - vPosition);
        float3 vHalf = normalize(vToLight + vView);
        fSpecularFactor = pow(saturate(dot(vNormal, vHalf)), Material.SpecularScalar);
    }
    
    float fSpotFactor = pow(saturate(dot(-vToLight, gLights[nIndex].m_vDirection)), gLights[nIndex].m_fFalloff);
    
    float fAttenuationFactor = 1.0f / (1.0f + gLights[nIndex].m_fAttenuation * fDistance * fDistance);
    
    float3 ambient = gcGlobalAmbientLight * Material.AmbientColor;
    float3 diffuse = gLights[nIndex].m_cDiffuse.rgb * fDiffuseFactor * Material.DiffuseColor;
    float3 specular = gLights[nIndex].m_cSpecular.rgb * fSpecularFactor * Material.SpecularColor;
    float3 emissive = Material.EmissiveColor;

    float3 lit = ambient + diffuse + specular + emissive;

    // 7. 최종 색상에 감쇠, 스팁 효과, 광원 intensity 적용  
    lit *= fAttenuationFactor * fSpotFactor * gLights[nIndex].m_fIntensity;
    
    // Material의 투명도 값(TransparencyScalar)를 alpha로 사용  
    return float4(lit, Material.TransparencyScalar);
}

float4 PointLight(int nIndex, float3 vPosition, float3 vNormal)
{
    // 광원과 픽셀 위치 간 벡터 계산
    float3 vToLight = gLights[nIndex].m_vPosition - vPosition;
    float fDistance = length(vToLight);

    // 감쇠 반경을 벗어나면 기여하지 않음
    if (fDistance > gLights[nIndex].m_fAttRadius)
    {
        return float4(0.0f, 0.0f, 0.0f, 1.0f);
    }
    
    float fSpecularFactor = 0.0f;
    vToLight /= fDistance; // 정규화
    float fDiffuseFactor = saturate(dot(vNormal, vToLight));

    if (fDiffuseFactor > 0.0f)
    {
        float3 vView = normalize(CameraPosition - vPosition);
        float3 vHalf = normalize(vToLight + vView);
        fSpecularFactor = pow(max(dot(normalize(vNormal), vHalf), 0.0f), 1);
    }

    float fAttenuationFactor = 1.0f / (1.0f + gLights[nIndex].m_fAttenuation * fDistance * fDistance);
   
    float3 lit = (gcGlobalAmbientLight * Material.AmbientColor.rgb) +
                 (gLights[nIndex].m_cDiffuse.rgb * fDiffuseFactor * Material.DiffuseColor) +
                 (gLights[nIndex].m_cSpecular.rgb * fSpecularFactor * Material.SpecularColor);

    return float4(lit * fAttenuationFactor * gLights[nIndex].m_fIntensity, 1.0f);
}

float4 Lighting(float3 vPosition, float3 vNormal)
{
    float4 cColor = float4(0.0f, 0.0f, 0.0f, 0.0f);
    [unroll(MAX_LIGHTS)]
    for (int i = 0; i < gnLights; i++)
    {
        if (gLights[i].m_bEnable)
        {
            if (gLights[i].m_nType == POINT_LIGHT)
            {
                cColor += PointLight(i, vPosition, vNormal);
            }
            else if (gLights[i].m_nType == SPOT_LIGHT)
            {
                cColor += SpotLight(i, vPosition, vNormal);
            }
        }
    }
    
    // 전역 환경광 추가
    cColor += gcGlobalAmbientLight;
    cColor.a = 1;
    
    return cColor;
}





