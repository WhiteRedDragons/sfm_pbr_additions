#ifndef PBR_PROJTEX_H_
#define PBR_PROJTEX_H_

// Need the Functions from here
#include "pbr_common_ps2_3_x.h"

// Using LUX as a Base for this
#include "lux_common_flashlight.h"

// NOTE: Roughness exposed for Dual-Lobe
void PBR_ComputeProjectedTexture(PBR_Data_t info, float2 f2ScreenPos, float f1Roughness, const bool bDoShadows, const bool bDoUberlight,
	out float3 f3DiffuseLight, out float3 f3SpecularLight, out float3 f3LightColorOut, out float3 f3LightDirOut, out float3 f3ShadowOut)
{
	float4 f4ProjTexPos = mul(float4(info.f3WorldPos, 1.0f), g_f4x4ProjTexWorldToTexture);

	// Clip all Pixels behind the Spotlight
	clip(f4ProjTexPos.w);

	// Perspective Divide is a must
	float3 f3ProjPos = f4ProjTexPos.xyz / f4ProjTexPos.www;

	// Color * Cookie
	float3 f3ProjTexColor = g_f3ProjTexColor * tex2D(Sampler_FlashlightCookie, f3ProjPos.xy).rgb;

	if(bDoUberlight)
	{
		float4 f4UberLightPosition = mul(float4(info.f3WorldPos.xyz, 1.0f), xmFlashlightWorldToLight).yzxw;
		f3ProjTexColor *= PerformUberlight(f4UberLightPosition.xyz, cSmoothEdge0, cSmoothEdge1,
			cSmoothOneOverWidth, cShearRound.xy, cAABB, cShearRound.zw);
	}

	float3 f3Delta = g_f3ProjTexPos - info.f3WorldPos; // Non-Incident Vector!!!
	float f1DistSquared = dot(f3Delta, f3Delta);
	float f1Dist = sqrt(f1DistSquared);
	float3 f3LightDir = f3Delta / f1Dist; // The true Nature of normalize()

	// The 0.6f here is probably a Magic Number..
	float f1EndFalloffFactor = RemapValClamped(f1Dist, g_f1ProjTexFarZ, 0.6f * g_f1ProjTexFarZ, 0.0f, 1.0f);

	// "Attenuation for light and to fade out shadow over distance"
	float f1Attenuation = saturate(dot(g_f3ProjTexDistanceAtten, float3(1.0f, 1.0f / f1Dist, 1.0f / f1DistSquared)));

	// Compute Projected Texture Shadows
	float3 f3Shadow = InternalProjectedTextureShadow(f3ProjPos.xy, f2ScreenPos, min(f3ProjPos.z, 0.999999f), f1Attenuation, bDoShadows);
	f3ShadowOut = f3Shadow;

	float3 f3LightColor = f3ProjTexColor;
	f3LightColor *= f1Attenuation;
	f3LightColor *= f1EndFalloffFactor;

	// Want this for SSS without Shadow
	f3LightColorOut = f3LightColor;
	f3LightDirOut = f3LightDir;

	// N.L applied in here
	// Function returns into our Outputs
	f3LightColor *= f3Shadow;
	calculateLight(info, f3LightDir, f3LightColor, f1Roughness, f3DiffuseLight, f3SpecularLight);
}

#endif