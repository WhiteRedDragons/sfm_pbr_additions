//==========================================================================//
//	Define non-existant Combos
//==========================================================================//

#if !defined(PROJTEX)
	#define PROJTEX 0
#endif

#if !defined(NUM_LIGHTS)
	#define NUM_LIGHTS 0
#endif

#if !defined(WORLD_NORMAL)
	#define WORLD_NORMAL 0
#endif

//==========================================================================//
//	Unpack Combos
//==========================================================================//

// .. TBD

//==========================================================================//
//	Common Definitions
//==========================================================================//

//#define TONEMAP_SCALE_NONE
#define TONEMAP_SCALE_LINEAR
//#define TONEMAP_SCALE_GAMMA

//==========================================================================//
//	Constants, Functions, Includes
//==========================================================================//

// Include for all Pixel Shaders
#include "lux_common_ps_fxc.h"
#include "pbr_common_ps2_3_x.h"

#if PROJTEX
	#include "pbr_common_projtex.h"
#endif

// Register Map for this Shader
#include "pbr_registermap.h"

const float4 cControls1				: register(PBR_PS_FLOAT_CONTROLS1);
#define g_f1Fullbright					(cControls1.x)
#define g_f1MicroShadowFactor			(cControls1.y)

const float4 cControls2				: register(PBR_PS_FLOAT_CONTROLS2);
#define g_f1DualLobe_RoughnessBias		(cControls2.x)
#define g_f1DualLobe_LerpFactor			(cControls2.y)
#define g_f1ParallaxDepth				(cControls2.z)
#define g_f1ParallaxCenter				(cControls2.w)

const float4 cSSSControls1			: register(PBR_PS_FLOAT_SSSCONTROLS1);
#define g_f3SSSColor					(cSSSControls1.rgb)
#define g_f1SSSIntensity				(cSSSControls1.w)

const float4 cSSSControls2			: register(PBR_PS_FLOAT_SSSCONTROLS2);
#define g_f1SSSPower					(cSSSControls2.x)

const float4 cNormalMapControls		: register(PBR_PS_FLOAT_NORMALMAPCONTROLS);
#define g_f3NormalMapFlips				(cNormalMapControls.xyz)
#define g_f1NormalMapFactor				(cNormalMapControls.w)

const float4 cMRAOMultiplier		: register(PBR_PS_FLOAT_MRAO_SCALE);
#define g_f3MRAOMultiplier				(cMRAOMultiplier.xyz)

const float4 cMRAOBias				: register(PBR_PS_FLOAT_MRAO_BIAS);
#define g_f3MRAOBias					(cMRAOBias.xyz)

const float4 cMRAOExponent			: register(PBR_PS_FLOAT_MRAO_EXPONENT);
#define g_f3MRAOExponent				(cMRAOExponent.xyz)

const float4 cScreenSizes			: register(LUX_PS_FLOAT_ASW_SCREENSIZE);
#define g_f2ScreenTexelSize				(cScreenSizes.xy)
#define g_f2ScreenHalfTexel				(cScreenSizes.zw)

const float4 cSSAOControls			: register(LUX_PS_FLOAT_ASW_SSAOCONTROLS);
#define g_f1SSAOStrength				(cSSAOControls.x)

#if !PROJTEX
const float3 cAmbientCube[6]		: register(LUX_PS_FLOAT_AMBIENTCUBE);
PixelShaderLightInfo cLightInfo[3]	: register(LUX_PS_FLOAT_LIGHTDATA);
#endif

//==================================================================================================
// Samplers
//==================================================================================================

#if SPECULARGLOSSINESS
	sampler Sampler_Diffuse				: register(s0);
	sampler Sampler_Specular			: register(s1);
#else
	sampler Sampler_BaseColor			: register(s0);
	sampler Sampler_MRAOTexture			: register(s1);
#endif
sampler Sampler_NormalTexture		: register(s2);

#if WRINKLEMAPS
	sampler Sampler_Compress		: register(s3);
	sampler Sampler_Stretch			: register(s4);
	sampler Sampler_NormalCompress	: register(s5);
	sampler Sampler_NormalStretch	: register(s6);
#endif

sampler Sampler_SSAO				: register(s7);

sampler Sampler_ThicknessTexture	: register(s10);

#if !PROJTEX
	sampler Sampler_Envmap				: register(s14);
#endif


struct PS_INPUT
{
	float2 ScreenPos				: VPOS;
	float4 WorldPos_ProjPosZ		: TEXCOORD0;
	float4 TexCoords1				: TEXCOORD1;

#if !PROJTEX
	float4	LightAtten				: TEXCOORD3;
#endif
		
#if WRINKLEMAPS
	float WrinkleWeight				: TEXCOORD4;
#endif
	
#if WORLD_NORMAL
	float SSAOFactor				: COLOR;
#endif

	float3 Tangent					: TANGENT;
	float3 Binormal					: BINORMAL;
	float3 Normal					: NORMAL;

	float NoCullDirection			: VFACE;
};

// Entry point
// FIXME: Move Entry Point and VS Output Struct to a Header
float4 main(PS_INPUT i) : COLOR
{
	PBR_Data_t info = PBR_Data_t_Constructor();
	info.f3WorldPos = i.WorldPos_ProjPosZ.xyz;
	info.f1MicroShadowStrength = g_f1MicroShadowFactor;

	float2 f2ScreenUV = i.ScreenPos * g_f2ScreenTexelSize + g_f2ScreenHalfTexel;
	float f1Depth = i.WorldPos_ProjPosZ.w;

	// Use a proper TBN Matrix instead of the cursed and broken Screenspace Reconstructed Tangents
	float3x3 xmTBN = float3x3(i.Tangent, i.Binormal, i.Normal);
	
	// Need unnormalized ViewDir for Parallax Mapping
	info.f3ViewDir = g_f3EyePos - info.f3WorldPos;
	
	#if PARALLAXOCCLUSION
		float3 f3ViewDirTS = worldToRelative(info.f3ViewDir, i.Tangent, i.Binormal, i.Normal);
		float2 f2TexCoord = parallaxCorrect(i.TexCoords1.xy, f3ViewDirTS, info.f3ViewDir, i.Normal, Sampler_NormalTexture, g_f1ParallaxDepth, g_f1ParallaxCenter);
	#else
		float2 f2TexCoord = i.TexCoords1.xy;
	#endif
	
	// Creation was non-normalized so normalize it now
	info.f3ViewDir = normalize(info.f3ViewDir);

	float4 f4BaseTexture;
	#if SPECULARGLOSSINESS
		f4BaseTexture = tex2D(Sampler_Diffuse, f2TexCoord);
	#else
		f4BaseTexture = tex2D(Sampler_BaseColor, f2TexCoord);
	#endif

	float4 f4NormalTS = tex2D(Sampler_NormalTexture, f2TexCoord);
	
	float f1WrinkleAmount, f1StretchAmount, f1TextureAmount;
	#if WRINKLEMAPS
		float f1WrinkleWeight = i.WrinkleWeight;
	
		f1WrinkleAmount = saturate(-f1WrinkleWeight);	// One of these two is zero
		f1StretchAmount = saturate(f1WrinkleWeight);	// while the other is in the 0..1 range
	
		f1TextureAmount = 1.0f - f1WrinkleAmount - f1StretchAmount; // These should sum to one
	
		float3 f3WrinkleColor = tex2D(Sampler_Compress, f2TexCoord).rgb;
		float3 f3StretchColor = tex2D(Sampler_Stretch, f2TexCoord).rgb;
	
		// Apply only to RGB for consistency with the Normal Map ( also makes the Results more predictable )
		f4BaseTexture.rgb = f1TextureAmount * f4BaseTexture.rgb
						+ f1WrinkleAmount * f3WrinkleColor
						+ f1StretchAmount * f3StretchColor;
	
		// NOTE: We use Normal Alpha for Height
		// Therefore, we MUST not use it
		float3 f3WrinkleNormalTS = tex2D(Sampler_NormalCompress, f2TexCoord).xyz;
		float3 f3StretchNormalTS = tex2D(Sampler_NormalStretch, f2TexCoord).xyz;
		f4NormalTS.xyz = f1TextureAmount * f4NormalTS.xyz
					   + f1WrinkleAmount * f3WrinkleNormalTS
					   + f1StretchAmount * f3StretchNormalTS;
	#endif
	
	f4BaseTexture.rgb *= g_f3DefaultTint;
	
	// Decompress TangentSpace NormalMap
	float3 f3NormalTS = f4NormalTS.xyz * 2.0f - 1.0f;
	
	// Fix Lighting when using $NoCull, caused by inverted Normals
	f3NormalTS *= i.NoCullDirection;

	// Flip desired Channels 
	f3NormalTS *= g_f3NormalMapFlips;

	// Requested: A way to weaken Normal Maps
	f3NormalTS = lerp(float3(0.0f, 0.0f, 1.0f), f3NormalTS, g_f1NormalMapFactor);

	info.f3NormalWS = normalize(mul(f3NormalTS, xmTBN));
	#if WORLD_NORMAL
		float fSSAODepth = i.SSAOFactor;
		return float4(info.f3NormalWS, fSSAODepth); // Does it want WS or TS? Original Code here used WS
	#endif
	
	#if SPECULARGLOSSINESS
		float4 f4SpecularTexture = tex2D(Sampler_Specular, f2TexCoord);

		float3 f3DiffuseColor = f4BaseTexture.rgb;		// Diffuse
		float3 f3SpecularColor = f4SpecularTexture.rgb;	// Specular

		float f1Roughness = 1.0f - f4SpecularTexture.a;	// Glossiness

		// Need to take the Ambient Occlusion from somewhere
		// BaseTexture Alpha it is!
		float f1AmbientOcclusion = f4BaseTexture.a;

		// We can still apply MRAOBias this way
		// The whole shebang, nothing else I can do
		f3SpecularColor		= saturate(g_f3MRAOMultiplier.r * pow(max(f3SpecularColor, 0.0f),		g_f3MRAOExponent.r) + g_f3MRAOBias.r);
		f1Roughness			= saturate(g_f3MRAOMultiplier.g * pow(max(f1Roughness, 0.0f),			g_f3MRAOExponent.g) + g_f3MRAOBias.b);
		f1AmbientOcclusion	= saturate(g_f3MRAOMultiplier.b * pow(max(f1AmbientOcclusion, 0.0f),	g_f3MRAOExponent.b) + g_f3MRAOBias.b);
	#else
		// Unused Alpha Channel
		float4 f4MRAOTexture = tex2D(Sampler_MRAOTexture, f2TexCoord);

		// The whole shebang, nothing else I can do
		f4MRAOTexture.rgb = saturate(g_f3MRAOMultiplier * pow(max(f4MRAOTexture.rgb, 0.0f), g_f3MRAOExponent) + g_f3MRAOBias);

		float f1Metalness = f4MRAOTexture.r;
		float3 f3DiffuseColor = (1.0f - f1Metalness) * f4BaseTexture.rgb;
		float3 f3SpecularColor = lerp(0.04f, f4BaseTexture.rgb, f1Metalness);

		float f1Roughness = f4MRAOTexture.g;
		float f1AmbientOcclusion = f4MRAOTexture.b;
	#endif

	// Fill the Struct
	info.f3DiffuseColor = f3DiffuseColor;
	info.f3SpecularColor = f3SpecularColor;
	info.f1Roughness = f1Roughness;

	#if DUALLOBE
		float f1SecondaryRoughness = saturate(info.f1Roughness + g_f1DualLobe_RoughnessBias);
	#endif

	#if SUBSURFACESCATTERING
		float f1Thickness = tex2D(Sampler_ThicknessTexture, f2TexCoord).r;
	#endif

	// Finalize AO with SSAO
	float f1SSAO = tex2Dlod(Sampler_SSAO, float4(f2ScreenUV, 0.0f, 0.0f)).r;
	f1SSAO = lerp(1.0f, f1SSAO, g_f1SSAOStrength);
	info.f1AmbientOcclusion = min(f1AmbientOcclusion, f1SSAO);

	// N.V
	info.f1NdotV = max(0, dot(info.f3NormalWS, info.f3ViewDir));
	
	// TODO: Use Reflect()
	info.f3Reflect = 2.0 * info.f1NdotV * info.f3NormalWS - info.f3ViewDir;
	
	//==================================================================================================
	// Direct Lighting
	//==================================================================================================
	
	// Start direct
	float3 f3DirectDiffuse = 0.0;	
	float3 f3DirectSpecular = 0.0f;

	// Only do regular World Lights with > 0 Lights
	#if (!PROJTEX && NUM_LIGHTS > 0)
		float4 f4LightAtten = i.LightAtten;

		// Unroll at when not looping > 1
		#if (NUM_LIGHTS == 1)
		[unroll]
		#endif
		for (uint n = 0; n < NUM_LIGHTS; ++n)
		{
			float3 f3LightColor;
			float3 f3LightDir;
			if (n == 3)
			{
				f3LightColor = float3(cLightInfo[0].color.w, cLightInfo[0].pos.w, cLightInfo[1].color.w) * f4LightAtten[n];
				f3LightDir = normalize(info.f3WorldPos - float3(cLightInfo[1].pos.w, cLightInfo[2].color.w, cLightInfo[2].pos.w));
			}
			else
			{
				f3LightColor = cLightInfo[n].color.xyz * f4LightAtten[n];
				f3LightDir = normalize(info.f3WorldPos - cLightInfo[n].pos.xyz);
			}

			// Non-Incident, I assume the Compiler will just reverse the subtraction Order above
			f3LightDir = -f3LightDir;
	
			float3 f3CurrentDiffuse, f3CurrentSpecular;
			calculateLight(info, f3LightDir, f3LightColor, info.f1Roughness, f3CurrentDiffuse, f3CurrentSpecular);

			#if DUALLOBE
				float3 f3SecondDiffuse, f3SecondSpecular;
				calculateLight(info, f3LightDir, f3LightColor, f1SecondaryRoughness, f3SecondDiffuse, f3SecondSpecular);
			
				// NOTE: Diffuse is not made from Roughness at the Moment, therefore only need to lerp Specular
				f3CurrentSpecular = lerp(f3CurrentSpecular, f3SecondSpecular, g_f1DualLobe_LerpFactor);
			#endif

			// FIXME: This should all be one BRDF
			#if SUBSURFACESCATTERING
				float3 f3SSSContribution = ComputeSubsurfaceScattering(info.f3NormalWS, f3LightDir, info.f3ViewDir, f1Thickness, g_f3SSSColor, g_f1SSSIntensity, g_f1SSSPower);

				// SSS is part of the Diffuse Contribution
				f3CurrentDiffuse += f3SSSContribution * f3LightColor;
			#endif

			f3DirectDiffuse += f3CurrentDiffuse;
			f3DirectSpecular += f3CurrentSpecular;
		}
	#elif PROJTEX
		float3 f3CurrentDiffuse, f3CurrentSpecular, f3LightColor, f3LightDir, f3Shadow;
		PBR_ComputeProjectedTexture(info, f2ScreenUV, info.f1Roughness, PROJTEXSHADOWS, UBERLIGHT,
			f3CurrentDiffuse, f3CurrentSpecular, f3LightColor, f3LightDir, f3Shadow);

		#if DUALLOBE
			float3 f3SecondDiffuse, f3SecondSpecular;

			// This needs to use LightColor * Shadow so the occlusion is the same
			calculateLight(info, f3LightDir, f3LightColor * f3Shadow, f1SecondaryRoughness, f3SecondDiffuse, f3SecondSpecular);
		
			// NOTE: Diffuse is not made from Roughness at the Moment, therefore only need to lerp Specular
			f3CurrentSpecular = lerp(f3CurrentSpecular, f3SecondSpecular, g_f1DualLobe_LerpFactor);
		#endif
	
		// FIXME: This should all be one BRDF
		#if SUBSURFACESCATTERING
			float3 f3SSSContribution = ComputeSubsurfaceScattering(info.f3NormalWS, f3LightDir, info.f3ViewDir, f1Thickness, g_f3SSSColor, g_f1SSSIntensity, g_f1SSSPower);

			// SSS is part of the Diffuse Contribution
			// NOTE: Unshadowed Light Color
			f3CurrentDiffuse += f3SSSContribution * f3LightColor;
		#endif

		f3DirectDiffuse += f3CurrentDiffuse;
		f3DirectSpecular += f3CurrentSpecular;
	#endif

	float f1Alpha = f4BaseTexture.a * g_f1AlphaModulation; // Need $Alpha and $Alpha2 for Premultiplied Alpha

	// On Materials like Glass the Background is basically the Diffuse Result
	// Technically the Surface itself can have *some* Diffuse ( Plastics ), but for now this will work for most translucents
	// Specular Contributions are unaffected by Opacity.
	// Another way to solve this is to predivide Specular by the Alpha, but it will lead to visual Artifacts
	#if PREMULTIPLIEDALPHA
		f3DirectDiffuse *= f1Alpha;
	#endif

	// No indirect Lighting right now
	float3 f3CombinedLighting = f3DirectDiffuse + f3DirectSpecular;

	// When Lighting is disabled the Ambient Cube is fullbright'ed
	// Since I disabled all the Indirect Lighting Code we need to account for it differently.
	// This will essentially do the same Thing:
	#if !PROJTEX
		f3CombinedLighting = info.f3DiffuseColor * info.f1AmbientOcclusion * g_f1Fullbright;
	#endif
	
	return LUX_Finalise(float4(f3CombinedLighting, f1Alpha), info.f3WorldPos, f1Depth);
}
