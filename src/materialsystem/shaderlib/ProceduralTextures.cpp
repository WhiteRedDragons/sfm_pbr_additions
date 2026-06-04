#include "ProceduralTextures.h"

// NOTE: This must be the last include File in a .cpp File!
#include "tier0/memdbgon.h"

// Externs
CSkinSSSPreintegrationLUT g_TextureGeneraetor_SkinPreintegration;
ITexture* g_ProceduralTextures[MAX_PROCEDURAL_TEXTURES] = { NULL };

void CSkinSSSPreintegrationLUT::RegenerateTextureBits(ITexture* pTexture, IVTFTexture* pVTFTexture, Rect_t* pRect)
{
	int nWidth = pVTFTexture->Width();
	int nHeight = pVTFTexture->Height();
	uint8_t* pImageData = pVTFTexture->ImageData();

	// NOTE: Adapted from
	// https://www.slideshare.net/slideshow/penner-preintegrated-skin-rendering-siggraph-2011-advances-in-realtime-rendering-course/13966747
	// This is only available as Powerpoint Slides. There is a book worth reading though.
	// Also credits go to flw300 ( See also https://www.shadertoy.com/view/NdBGDz )
	// I use the Gaussian based Diffusion Profile and Tonemapping after giving up on my original Diffusion Profile
	// I also realized the Results are blue'sh, the Tonemapping gets rid of it

	const float PI = 3.1415926535f;

	for (int nY = 0; nY < nHeight; ++nY)
	{
		float y = float(nY) / float(nHeight);

		// Account for Texture Flip on DX
		y = 1.0f - y;

		// 1/r
		float Curvature = 1.0f / y;

		for (int nX = 0; nX < nWidth; ++nX)
		{
			float LinearX = float(nX) / float(nWidth);
			float f1Theta = acosf(LinearX * 2.0f - 1.0f);

			// Go over the Integral as a float
			// Where Min = -PI/2 and Max = PI/2
			// ( Half Circle )
			float3 f3Numerator = 0.0f;
			float3 f3Denominator = 0.0f;
			for (float x = -PI / 2.0f; x < PI / 2.0f; x += PI / float(DIFFUSESCATTER_SAMPLES))
			{
				// This is 2*sin(x/2)
				// The Function shown in the Slides doesn't show Curvature being used
				// It can be interpreted from the Graphic next to the Function however
				// There it says 2r*sin(theta/2)
				// r becomes Curvature and theta is the Angle (x) from the Integral
				float f1Distance = 2.0f * Curvature * sin(x / 2.0f);

				// This might be better if it was using the Burley Diffusion Profile
				// Saturation is mentioned at the very End of the Slides
				f3Numerator += MAX(0.0f, cosf(f1Theta + x)) * DiffusionProfile(f1Distance);
				f3Denominator += DiffusionProfile(f1Distance);
			}
			float3 f3Result = (f3Numerator / f3Denominator);

			// Apply Tonemapping so the Results aren't Blue ( Not sure why they are )
			float3 f3TonedResult = Tonemap(f3Result * 12.0);
			float3 f3WhiteScale = 1.0 / Tonemap(float3(TONEMAP_W, TONEMAP_W, TONEMAP_W));
			f3Result = f3TonedResult * f3WhiteScale;

			// I asked for RGB888. But whatever... BGRA8888 Layout for IVTFTexture*
			int nLinearCoord = nX + nY * nWidth;
			pImageData[nLinearCoord * 4 + 0] = (uint8_t)(clamp(f3Result.z, 0.0f, 1.0f) * 255.0f);
			pImageData[nLinearCoord * 4 + 1] = (uint8_t)(clamp(f3Result.y, 0.0f, 1.0f) * 255.0f);
			pImageData[nLinearCoord * 4 + 2] = (uint8_t)(clamp(f3Result.x, 0.0f, 1.0f) * 255.0f);
			pImageData[nLinearCoord * 4 + 3] = 255; // Alpha
		}
	}
}

void InitProceduralTextures(IMaterialSystem* pMatSys)
{
	for(int n = 0; n < MAX_PROCEDURAL_TEXTURES; ++n)
	{
		switch(n)
		{
			case SKIN_SSSPREINTEGRATION_LUT:
			{
				g_ProceduralTextures[SKIN_SSSPREINTEGRATION_LUT] = pMatSys->CreateProceduralTexture(
					"LUT_SSSPreintegration", TEXTURE_GROUP_OTHER, 256, 256, IMAGE_FORMAT_RGB888, TEXTUREFLAGS_NOMIP | TEXTUREFLAGS_NOLOD);

				g_ProceduralTextures[SKIN_SSSPREINTEGRATION_LUT]->SetTextureRegenerator(&g_TextureGeneraetor_SkinPreintegration);
				g_ProceduralTextures[SKIN_SSSPREINTEGRATION_LUT]->Download();
			}

			default:
			break;
		}
	}
}

void ShutdownProceduralTextures()
{
	for (int n = 0; n < MAX_PROCEDURAL_TEXTURES; ++n)
	{
		if(g_ProceduralTextures[n])
			g_ProceduralTextures[n]->Release();
	}
}
