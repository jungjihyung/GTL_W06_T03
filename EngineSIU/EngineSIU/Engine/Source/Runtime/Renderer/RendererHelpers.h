#pragma once
#include "Launch/EngineLoop.h"

namespace RendererHelpers {

    inline FMatrix CalculateMVP(const FMatrix& Model, const FMatrix& View, const FMatrix& Projection) {
        return Model * View * Projection;
    }

    inline FMatrix CalculateNormalMatrix(const FMatrix& Model) {
        return FMatrix::Transpose(FMatrix::Inverse(Model));
    }
}

namespace MaterialUtils {
    inline void UpdateMaterial(FDXDBufferManager* BufferManager, FGraphicsDevice* Graphics, const FObjMaterialInfo& MaterialInfo) {
        FMaterialConstants data;
        data.DiffuseColor = MaterialInfo.Diffuse;
        data.TransparencyScalar = MaterialInfo.TransparencyScalar;
        data.AmbientColor = MaterialInfo.Ambient;
        data.DensityScalar = MaterialInfo.DensityScalar;
        data.SpecularColor = MaterialInfo.Specular;
        data.SpecularScalar = MaterialInfo.SpecularScalar;
        data.EmmisiveColor = MaterialInfo.Emissive;

        BufferManager->UpdateConstantBuffer(TEXT("FMaterialConstants"), data);

        if (MaterialInfo.bHasTexture)
        {
            std::shared_ptr<FTexture> DiffuseTexture = FEngineLoop::ResourceManager.GetTexture(MaterialInfo.DiffuseTexturePath);
            Graphics->DeviceContext->PSSetShaderResources(0, 1, &DiffuseTexture->TextureSRV);
            Graphics->DeviceContext->PSSetSamplers(0, 1, &DiffuseTexture->SamplerState);
        }
        else
        {
            std::shared_ptr<FTexture> NoneTexture = FEngineLoop::ResourceManager.GetTexture(L"NoneTexture");
            Graphics->DeviceContext->PSSetShaderResources(0, 1, &NoneTexture->TextureSRV);
            Graphics->DeviceContext->PSSetSamplers(0, 1, &NoneTexture->SamplerState);
        }

        if (MaterialInfo.bHasNormalMap)
        {
            std::shared_ptr<FTexture> BumpTexture = FEngineLoop::ResourceManager.GetTexture(MaterialInfo.BumpTexturePath);
            Graphics->DeviceContext->PSSetShaderResources(1, 1, &BumpTexture->TextureSRV);
            Graphics->DeviceContext->PSSetSamplers(1, 1, &BumpTexture->SamplerState);
        }
        else
        {
            ID3D11ShaderResourceView* nullSRV[1] = { nullptr };
            ID3D11SamplerState* nullSampler[1] = { nullptr };
            Graphics->DeviceContext->PSSetShaderResources(1, 1, nullSRV);
            Graphics->DeviceContext->PSSetSamplers(1, 1, nullSampler);
        }
    }
}
