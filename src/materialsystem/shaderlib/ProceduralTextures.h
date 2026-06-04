#ifndef PROCEDURALTEXTURES_H
#define PROCEDURALTEXTURES_H

#ifdef _WIN32		   
#pragma once
#endif

#include "materialsystem/ishadersystem.h"
#include "materialsystem/itexture.h"

// I want to use floatx for this
#include "stdshaders/cpp_floatx.h"

#define TONEMAP_A 0.15f
#define TONEMAP_B 0.50f
#define TONEMAP_C 0.10f
#define TONEMAP_D 0.20f
#define TONEMAP_E 0.02f
#define TONEMAP_F 0.30f
#define TONEMAP_W 11.2f
#define DIFFUSESCATTER_SAMPLES 64
class CSkinSSSPreintegrationLUT : public ITextureRegenerator
{
	void RegenerateTextureBits(ITexture* pTexture, IVTFTexture* pVTFTexture, Rect_t* pRect) override;
	void Release() override {};

	float Gaussian(float v, float r)
	{
		return 1.0 / sqrt(2.0 * 3.1415926535f * v) * exp(-(r * r) / (2.0 * v));
	}

	float3 DiffusionProfile(float r)
	{
		return float3(0.0, 0.0, 0.0)
			+ Gaussian(0.0064, r) * float3(0.233, 0.455, 0.649)
			+ Gaussian(0.0484, r) * float3(0.100, 0.336, 0.344)
			+ Gaussian(0.187, r) * float3(0.118, 0.198, 0.0)
			+ Gaussian(0.567, r) * float3(0.113, 0.007, 0.007)
			+ Gaussian(1.99, r) * float3(0.358, 0.004, 0.0)
			+ Gaussian(7.41, r) * float3(0.233, 0.0, 0.0);
	}

	float3 Tonemap(float3 x)
	{
		return ((x * (TONEMAP_A * x + TONEMAP_C * TONEMAP_B) + TONEMAP_D * TONEMAP_E) / (x * (TONEMAP_A * x + TONEMAP_B) + TONEMAP_D * TONEMAP_F)) - TONEMAP_E / TONEMAP_F;
	}
};
extern CSkinSSSPreintegrationLUT g_TextureGeneraetor_SkinPreintegration;

enum ProceduralTextures_t
{
	SKIN_SSSPREINTEGRATION_LUT = 0,

	MAX_PROCEDURAL_TEXTURES
};

extern ITexture* g_ProceduralTextures[MAX_PROCEDURAL_TEXTURES];
void InitProceduralTextures(IMaterialSystem* pMatSys);
void ShutdownProceduralTextures();

#endif // PROCEDURALTEXTURES_H
