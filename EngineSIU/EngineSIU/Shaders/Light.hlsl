#define MAX_LIGHTS 16 

#define POINT_LIGHT         1
#define SPOT_LIGHT          2
#define DIRECTIONAL_LIGHT   3

struct LIGHT
{
    float3 m_cBaseColor;
    float pad3;

    float3 m_vPosition;
    float m_fFalloff; // 스팟라이트의 감쇠 인자 (스포트 각도에 따른 감쇠)

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

float4 CalcLight(int nIndex, float3 vPosition, float3 vNormal)
{
    float4 litColor = float4(0, 0, 0, 1);
    
    if (!gLights[nIndex].m_bEnable)
    {
        return litColor;
    }
    
    if (gLights[nIndex].m_nType == POINT_LIGHT)
    {
        float3 vToLight = gLights[nIndex].m_vPosition - vPosition;
        float fDistance = length(vToLight);

        // 감쇠 반경을 벗어나면 기여하지 않음
        if (fDistance > gLights[nIndex].m_fAttRadius)
        {
            return float4(0.0f, 0.0f, 0.0f, 1.0f);
        }
    
        float fSpecularFactor = 0.0f;
        vToLight /= fDistance; // 정규화
        
        float fDiffuseFactor = dot(vNormal, vToLight);
        
        if (fDiffuseFactor < 0)
        {
            return float4(0, 0, 0, 1);
        }
        
        fDiffuseFactor = saturate(fDiffuseFactor);
        float3 litResult = float3(0, 0, 0);
        float fAttenuationFactor = 1.0f / (1.0f + gLights[nIndex].m_fAttenuation * fDistance * fDistance);
   
#if LIGHTING_MODEL_LAMBERT
    litResult  = Material.AmbientColor.rgb + gLights[nIndex].m_cBaseColor.rgb * fDiffuseFactor * Material.DiffuseColor.rgb;
#else   
        if (fDiffuseFactor > 0.0f)
        {
            float3 vView = normalize(CameraPosition - vPosition);
            float3 vHalf = normalize(vToLight + vView);
            fSpecularFactor = pow(max(dot(normalize(vNormal), vHalf), 0.0f), 1);
        }

    
        litResult = (gcGlobalAmbientLight * Material.AmbientColor.rgb) +
                 (gLights[nIndex].m_cBaseColor.rgb * fDiffuseFactor * Material.DiffuseColor) +
                 (gLights[nIndex].m_cBaseColor.rgb * fSpecularFactor * Material.SpecularColor);
#endif
        return float4(litResult * fAttenuationFactor * gLights[nIndex].m_fIntensity, 1.0f);
    }
    else if (gLights[nIndex].m_nType == SPOT_LIGHT)
    {
        float3 vToLight = gLights[nIndex].m_vPosition - vPosition;
        float fDistance = length(vToLight);
        if (fDistance < gLights[nIndex].m_fAttRadius)
        {
            vToLight /= fDistance;
            float fDiffuseFactor = saturate(dot(vNormal, vToLight));
            float fSpecularFactor = 0.0;
            if (fDiffuseFactor > 0.0)
            {
                float3 vView = normalize(CameraPosition - vPosition);
                float3 vHalf = normalize(vToLight + vView);
                fSpecularFactor = pow(saturate(dot(vNormal, vHalf)), Material.SpecularScalar);
            }
            
            float fAngleCos = dot(-vToLight, normalize(gLights[nIndex].m_vDirection));
            if (fAngleCos > 0.0)
            {
                float fSpotFactor = pow(fAngleCos, gLights[nIndex].m_fFalloff);
                float fAttenuationFactor = 1.0 / (1.0 + gLights[nIndex].m_fAttenuation * fDistance * fDistance);
                
                litColor = float4(
                    (gcGlobalAmbientLight.rgb * Material.AmbientColor.rgb) +
                    (gLights[nIndex].m_cBaseColor.rgb * fDiffuseFactor * Material.DiffuseColor.rgb) +
                    (gLights[nIndex].m_cBaseColor.rgb * fSpecularFactor * Material.SpecularColor.rgb),
                    1.0
                );
                litColor *= fAttenuationFactor * fSpotFactor * gLights[nIndex].m_fIntensity;
            }
        }
    }
    else if (gLights[nIndex].m_nType == DIRECTIONAL_LIGHT)
    {
        float3 lightDir = normalize(-gLights[nIndex].m_vDirection);
        float fDiffuseFactor = saturate(dot(vNormal, lightDir));
        float fSpecularFactor = 0.0;
        if (fDiffuseFactor > 0.0)
        {
            float3 vView = normalize(CameraPosition - vPosition);
            float3 vHalf = normalize(lightDir + vView);
            fSpecularFactor = pow(saturate(dot(vNormal, vHalf)), Material.SpecularScalar);
        }
        
        litColor = float4(
            (Material.AmbientColor.rgb) +
            (gLights[nIndex].m_cBaseColor.rgb * fDiffuseFactor * Material.DiffuseColor.rgb) +
            (gLights[nIndex].m_cBaseColor.rgb * fSpecularFactor * Material.SpecularColor.rgb),
            1.0
        );
        litColor *= gLights[nIndex].m_fIntensity;
    }
    
    return litColor;
}
float4 Lighting(float3 vPosition, float3 vNormal)
{
    float4 cColor = float4(0.0, 0.0, 0.0, 0.0);
    [unroll(MAX_LIGHTS)]
    for (int i = 0; i < gnLights; i++)
    {
        cColor += CalcLight(i, vPosition, vNormal);
    }
    
    cColor += gcGlobalAmbientLight;
    cColor.a = 1.0;
    
    return cColor;
}
