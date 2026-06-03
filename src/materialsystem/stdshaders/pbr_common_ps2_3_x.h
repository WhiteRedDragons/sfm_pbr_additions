//==================================================================================================
//
// Physically Based Rendering Header for brushes and models
//
//==================================================================================================

#ifndef PBR_COMMON_H_
#define PBR_COMMON_H_

// Universal Constants
static const float EPSILON = 0.00001;

sampler Sampler_SSSLUT : register(s9);

// I'm tried of passing 2000 Variables into every Function. Just fill this Thing and pass it on
struct PBR_Data_t
{
	float3 f3DiffuseColor;
	float3 f3SpecularColor;
	float3 f3WorldPos;
	float3 f3NormalWS;
	float3 f3ViewDir;
	float3 f3Reflect;
	float f1NdotV;
	float f1Roughness;
	float f1AmbientOcclusion;
	float f1MicroShadowStrength;
	float f1Thickness;
	float f1Curvature;
};

// Can't add a Constructor in HLSL, this will work though
PBR_Data_t PBR_Data_t_Constructor()
{
	PBR_Data_t info;
	info.f3DiffuseColor = 1.0f;
	info.f3SpecularColor = 0.0f;
	info.f3WorldPos = 0.0f;
	info.f3NormalWS = float3(0.0f, 0.0, 1.0f);
	info.f3ViewDir = float3(1.0f, 0.0f, 0.0f);
	info.f3Reflect = float3(0.0f, 1.0f, 0.0f);
	info.f1NdotV = 1.0f;
	info.f1Roughness = 1.0f;
	info.f1AmbientOcclusion = 1.0f;
	info.f1MicroShadowStrength = 1.0f;
	info.f1Thickness = 1.0f;
	info.f1Curvature = 0.0f;
	return info;
}

// Source
// https://advances.realtimerendering.com/other/2016/naughty_dog/NaughtyDog_TechArt_Final.pdf
float ApplyMicroShadow(float ao, float3 N, float3 L, float shadow)
{
	float aperture = 2.0 * ao * ao;
	float microShadow = saturate(abs(dot(L, N)) + aperture - 1.0f);
	return shadow * microShadow;
}

// Shlick's approximation of the Fresnel factor
float3 fresnelSchlick(float3 F0, float cosTheta)
{
	return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

// Shlick's approximation of the Fresnel factor with account for roughness
float3 fresnelSchlickRoughness(float3 F0, float cosTheta, float roughness)
{
	return F0 + max(0.0, (1.0 - roughness) - F0) * pow(1.0 - cosTheta, 5.0);
}

// GGX/Towbridge-Reitz normal distribution function
// Uses Disney's reparametrization of alpha = roughness^2
float ndfGGX(float cosLh, float roughness)
{
	float alpha   = roughness * roughness;
	float alphaSq = alpha * alpha;

	float denom = (cosLh * cosLh) * (alphaSq - 1.0) + 1.0;
	return alphaSq / (PI * denom * denom);
}

// Single term for separable Schlick-GGX below
float gaSchlickG1(float cosTheta, float k)
{
	return cosTheta / (cosTheta * (1.0 - k) + k);
}

// Schlick-GGX approximation of geometric attenuation function using Smith's method
float gaSchlickGGX(float cosLi, float cosLo, float roughness)
{
	float r = roughness + 1.0;
	float k = (r * r) / 8.0; // Epic suggests using this roughness remapping for analytic lights
	return gaSchlickG1(cosLi, k) * gaSchlickG1(cosLo, k);
}

// Monte Carlo integration, approximate analytic version based on Dimitar Lazarov's work
// https://www.unrealengine.com/en-US/blog/physically-based-shading-on-mobile
float3 EnvBRDFApprox(float3 SpecularColor, float Roughness, float NoV)
{
	const float4 c0 = { -1, -0.0275, -0.572, 0.022 };
	const float4 c1 = { 1, 0.0425, 1.04, -0.04 };
	float4 r = Roughness * c0 + c1;
	float a004 = min(r.x * r.x, exp2(-9.28 * NoV)) * r.x + r.y;
	float2 AB = float2(-1.04, 1.04) * a004 + r.zw;
	return SpecularColor * AB.x + AB.y;
}

// Compute the matrix used to transform tangent space normals to world space
// This expects DirectX normal maps in Mikk Tangent Space http://www.mikktspace.com
float3x3 compute_tangent_frame(float3 N, float3 P, float2 uv, out float3 T, out float3 B, out float sign_det)
{
	float3 dp1 = ddx(P);
	float3 dp2 = ddy(P);
	float2 duv1 = ddx(uv);
	float2 duv2 = ddy(uv);

	sign_det = dot(dp2, cross(N, dp1)) > 0.0 ? -1 : 1;

	float3x3 M = float3x3(dp1, dp2, cross(dp1, dp2));
	float2x3 inverseM = float2x3(cross(M[1], M[2]), cross(M[2], M[0]));
	T = normalize(mul(float2(duv1.x, duv2.x), inverseM));
	B = normalize(mul(float2(duv1.y, duv2.y), inverseM));
	return float3x3(T, B, N);
}

// Calculate direct light for one source
// Note that Roughness is exposed as an individual Input here ( that is used over PBR_Data )
// This is so a separate lobe can be computed for dual-lobe
void calculateLight(PBR_Data_t info, float3 f3LightDir, float3 f3LightIntensity, float f1Roughness, out float3 f3DiffuseLight, out float3 f3SpecularLight)
{
	// Lh
	float3 HalfAngle = normalize(f3LightDir + info.f3ViewDir);		// L+V ( lightIn + lightOut )
	float NdotL = max(0.0f, dot(info.f3NormalWS, f3LightDir));		// N.L
	float NdotV = max(0.0f, dot(info.f3NormalWS, info.f3ViewDir));	// N.V
	float cosHalfAngle = max(0.0, dot(info.f3NormalWS, HalfAngle)); // N.H

	// Apply MicroShadows to LightIntensity ( that way it applies to both Specular and Diffuse Color )
	float f1MicroShadows = ApplyMicroShadow(info.f1AmbientOcclusion, info.f3NormalWS, f3LightDir, 1.0f);
	f1MicroShadows = lerp(1.0f, f1MicroShadows, info.f1MicroShadowStrength);

	// F - Calculate Fresnel term for direct lighting
	float3 F = fresnelSchlick(info.f3SpecularColor, max(0.0, dot(HalfAngle, info.f3ViewDir)));

	// D - Calculate normal distribution for specular BRDF
	float D = ndfGGX(cosHalfAngle, f1Roughness);

	// Calculate geometric attenuation for specular BRDF
	float G = gaSchlickGGX(NdotL, NdotV, f1Roughness);

	// Diffuse scattering happens due to light being refracted multiple times by a dielectric medium
	// Metals on the other hand either reflect or absorb energso diffuse contribution is always, zero
	// To be energy conserving we must scale diffuse BRDF contribution based on Fresnel factor & metalness
	float3 kd = float3(1, 1, 1) - F;
	float3 diffuseBRDF = info.f3DiffuseColor * kd;

	// Cook-Torrance specular microfacet BRDF
	float3 specularBRDF = (F * D * G) / max(EPSILON, 4.0 * NdotL * NdotV);

	// Remap N.L (and Curvature) using the Preintegration LUT
	#if SUBSURFACESCATTERING
		float f1Attenuation = dot(info.f3NormalWS, f3LightDir);
		float f1TexCoordX = (f1Attenuation * 0.5f + 0.5f);
		float f1TexCoordY = /info.f1Curvature);

		float3 f3LightWrap = tex2Dlod(Sampler_SSSLUT, float4(f1TexCoordX, f1TexCoordY, 0.0f, 0.0f)).rgb;

		// N.L < -0.8f will have some Artefacts because the Surfaces become incredibly thin.
		// Especially on projected Textures!
		// Avoid Backfaces being lit the further away from N.L == 0 it is
		// Everything above > 0.0, just *1.0f
		#if SUBSURFACESCATTERING
			float f1BackScatterMask = 1.0f - saturate(-dot(info.f3NormalWS, f3LightDir));
			f3LightWrap *= f1BackScatterMask;
		#endif

		// Lerp between SSS Results and regular N.L based on the Thickness of the Material
		// With increasing Thickness, less SSS
		f3LightWrap = lerp(f3LightWrap, (float3)NdotL, info.f1Thickness);

		// Only apply SSS to Diffuse, not Specular!
		f3DiffuseLight = diffuseBRDF * f3LightIntensity * f3LightWrap * f1MicroShadows;
		f3SpecularLight = specularBRDF * f3LightIntensity * NdotL * f1MicroShadows;
	#else

		// (Shadow * Light Color) *= AO * N.L
		f3LightIntensity *= f1MicroShadows * NdotL;

		// Return Results
		f3DiffuseLight = diffuseBRDF * f3LightIntensity;
		f3SpecularLight = specularBRDF * f3LightIntensity;
	#endif
}

#if PARALLAXOCCLUSION
float2 parallaxCorrect(float2 texCoord, float3 viewRelativeDir, float3 worldSpaceWorldToEye, float3 worldSpaceNormal, sampler depthMap, float parallaxDepth, float parallaxCenter)
{
	float fLength = length( viewRelativeDir );
	float fParallaxLength = sqrt( fLength * fLength - viewRelativeDir.z * viewRelativeDir.z ) / viewRelativeDir.z; 
	float2 vParallaxDirection = normalize(  viewRelativeDir.xy );
	float2 vParallaxOffsetTS = vParallaxDirection * fParallaxLength;
	float fViewDotHorizonFactor = min(saturate(dot(normalize(worldSpaceNormal), normalize(worldSpaceWorldToEye))), 0.5) * 2;
	vParallaxOffsetTS *= saturate(parallaxDepth * fViewDotHorizonFactor);

	 // Compute all the derivatives:
	float2 dx = ddx( texCoord );
	float2 dy = ddy( texCoord );

	int nNumSteps = 20;

	float fCurrHeight = 0.0;
	float fStepSize   = 1.0 / (float) nNumSteps;
	float fPrevHeight = 1.0;
	float fNextHeight = 0.0;

	int    nStepIndex = 0;
	bool   bCondition = true;

	float2 vTexOffsetPerStep = fStepSize * vParallaxOffsetTS;
	float2 vTexCurrentOffset = texCoord;
	float  fCurrentBound     = 1.0;
	float  fParallaxAmount   = 0.0;

	float2 pt1 = 0;
	float2 pt2 = 0;

	float2 texOffset2 = 0;

	while ( nStepIndex < nNumSteps ) 
	{
		vTexCurrentOffset -= vTexOffsetPerStep;

		// Sample height map which in this case is stored in the alpha channel of the normal map:
		fCurrHeight = parallaxCenter + tex2Dgrad( depthMap, vTexCurrentOffset, dx, dy ).a;

		fCurrentBound -= fStepSize;

		if ( fCurrHeight > fCurrentBound ) 
		{     
			pt1 = float2( fCurrentBound, fCurrHeight );
			pt2 = float2( fCurrentBound + fStepSize, fPrevHeight );

			texOffset2 = vTexCurrentOffset - vTexOffsetPerStep;

			nStepIndex = nNumSteps + 1;
		}
		else
		{
			nStepIndex++;
			fPrevHeight = fCurrHeight;
		}
	}   // End of while ( nStepIndex < nNumSteps )

	float fDelta2 = pt2.x - pt2.y;
	float fDelta1 = pt1.x - pt1.y;
	fParallaxAmount = (pt1.x * fDelta2 - pt2.x * fDelta1 ) / ( fDelta2 - fDelta1 );
	float2 vParallaxOffset = vParallaxOffsetTS * (1 - fParallaxAmount);
	// The computed texture offset for the displaced point on the pseudo-extruded surface:
	float2 texSample = texCoord - vParallaxOffset;
	return texSample;
}
#endif

float3 worldToRelative(float3 worldVector, float3 surfTangent, float3 surfBasis, float3 surfNormal)
{
   return float3(
	   dot(worldVector, surfTangent),
	   dot(worldVector, surfBasis),
	   dot(worldVector, surfNormal)
   );
}

#endif // PBR_COMMON_H_