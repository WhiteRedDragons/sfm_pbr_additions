//==================================================================================================
//
// Physically Based Rendering shader for brushes and models
// Adopted from Zombie Master: Reborn, modified for SFM compatibility
// https://github.com/zm-reborn/zmr-game/
//
//==================================================================================================

// Includes for all shaders
#include "BaseVSShader.h"
#include "cpp_shader_constant_register_map.h"

#include "vtf/vtf.h"

// Includes for PS30
#include "pbr_vs30.inc"
#include "pbr_mrao_ps30.inc"
#include "pbr_mrao_projtex_ps30.inc"
#include "pbr_sg_ps30.inc"
#include "pbr_sg_projtex_ps30.inc"

#define SFM_BLACKBOX_MODE

// M/R and S/G
const Sampler_t SAMPLER_BASECOLOR		= SHADER_SAMPLER0;
const Sampler_t SAMPLER_DIFFUSE			= SHADER_SAMPLER0;
const Sampler_t SAMPLER_SPECULAR		= SHADER_SAMPLER1;
const Sampler_t SAMPLER_MRAO			= SHADER_SAMPLER1;

const Sampler_t SAMPLER_NORMAL			= SHADER_SAMPLER2;

// Wrinklemapping should follow the other 3
const Sampler_t SAMPLER_COMPRESS		= SHADER_SAMPLER3;
const Sampler_t SAMPLER_STRETCH			= SHADER_SAMPLER4;
const Sampler_t SAMPLER_BUMPCOMPRESS	= SHADER_SAMPLER5;
const Sampler_t SAMPLER_BUMPSTRETCH		= SHADER_SAMPLER6;

// We always have SSAO
const Sampler_t SAMPLER_SSAO			= SHADER_SAMPLER7;

// 8-12 are now usable for other Things
const Sampler_t SAMPLER_EMISSIVE		= SHADER_SAMPLER8; // Can move this to a separate Pass
const Sampler_t SAMPLER_LIGHTWARP		= SHADER_SAMPLER9; // FIXME: Nuke this
const Sampler_t SAMPLER_THICKNESS		= SHADER_SAMPLER10; // FIXME: Nuke this

// Lighting is split into two parts
// Regular Pass and Projected Texture Passes
const Sampler_t SAMPLER_LIGHTMAP		= SHADER_SAMPLER13;
const Sampler_t SAMPLER_ENVMAP			= SHADER_SAMPLER14;
const Sampler_t SAMPLER_PROJTEXCOOKIE	= SHADER_SAMPLER12; // These exclude eachother with EnvMap and Lightmap
const Sampler_t SAMPLER_RANDOMROTATION	= SHADER_SAMPLER13;
const Sampler_t SAMPLER_SHADOWDEPTH		= SHADER_SAMPLER14;

// Convars
static ConVar pbr_version("pbr_version", "1.00", FCVAR_CHEAT);
static ConVar mat_fullbright("mat_fullbright", "0", FCVAR_CHEAT);
static ConVar mat_specular("mat_specular", "1", FCVAR_NONE);
static ConVar mat_pbr_parallaxmap("mat_pbr_parallaxmap", "1");

static ConVar pbr_microshadows_globalstrength("pbr_microshadows_globalstrength", "0.50", FCVAR_NONE);

//==========================================================================//
// Shader Start
//==========================================================================//
BEGIN_VS_SHADER(PBR, "PBR shader")

	// Setting up vmt parameters
	BEGIN_SHADER_PARAMS;

		// Metallic/Roughness
		SHADER_PARAM(BaseColor,					SHADER_PARAM_TYPE_TEXTURE, "", "")
		SHADER_PARAM(MRAOTexture,				SHADER_PARAM_TYPE_TEXTURE, "", "")

		// Specular/Glossiness
		SHADER_PARAM(Diffuse,					SHADER_PARAM_TYPE_TEXTURE, "", "")
		SHADER_PARAM(Specular,					SHADER_PARAM_TYPE_TEXTURE, "", "")

		// Will store the Value determined in Param Init on this Parameter
		SHADER_PARAM(SpecularGlossiness,		SHADER_PARAM_TYPE_BOOL, "", "(Internal Parameter)")

		// Proper Terminology
		SHADER_PARAM(BumpMap,					SHADER_PARAM_TYPE_TEXTURE, "", "") // Required so we can receive Lighting
		SHADER_PARAM(NormalMap,					SHADER_PARAM_TYPE_TEXTURE, "", "")

		SHADER_PARAM(AlphaTestReference,		SHADER_PARAM_TYPE_FLOAT, "0", "")
		SHADER_PARAM(EnvMap,					SHADER_PARAM_TYPE_ENVMAP, "", "Set the cubemap for this material.")
		SHADER_PARAM(EmissionTexture,			SHADER_PARAM_TYPE_TEXTURE, "", "Emission texture")
		SHADER_PARAM(BumpFrame,					SHADER_PARAM_TYPE_INTEGER, "0", "Frame number for $bumpmap")
		SHADER_PARAM(UseEnvAmbient,				SHADER_PARAM_TYPE_BOOL, "0", "Use the cubemaps to compute ambient light.")
		SHADER_PARAM(LightWarpTexture,			SHADER_PARAM_TYPE_TEXTURE, "", "Lightwarp Texture" )
		SHADER_PARAM(ThicknessTexture,			SHADER_PARAM_TYPE_TEXTURE, "", "Thickness map for SSS" )
		SHADER_PARAM(Parallax,					SHADER_PARAM_TYPE_BOOL, "0", "Use Parallax Occlusion Mapping.")
		SHADER_PARAM(ParallaxDepth,				SHADER_PARAM_TYPE_FLOAT, "0.0030", "Depth of the Parallax Map")
		SHADER_PARAM(ParallaxCenter,			SHADER_PARAM_TYPE_FLOAT, "0.5", "Center depth of the Parallax Map")
		SHADER_PARAM(EmissiveFactor,			SHADER_PARAM_TYPE_FLOAT, "1.0", "Emissive factor" )
		SHADER_PARAM(SpecularFactor,			SHADER_PARAM_TYPE_FLOAT, "1.0", "Specular factor" )
		SHADER_PARAM(SSSColor,					SHADER_PARAM_TYPE_COLOR, "[1 1 1]", "Subsurface scattering color")
		SHADER_PARAM(SSSIntensity,				SHADER_PARAM_TYPE_FLOAT, "1.0", "SSS intensity")
		SHADER_PARAM(SSSPowerScale,				SHADER_PARAM_TYPE_FLOAT, "1.0", "SSS power scale")
		SHADER_PARAM(Compress,					SHADER_PARAM_TYPE_TEXTURE, "", "Compression wrinklemap")
		SHADER_PARAM(BumpCompress,				SHADER_PARAM_TYPE_TEXTURE, "", "Stretch bumpmap" )
		SHADER_PARAM(Stretch,					SHADER_PARAM_TYPE_TEXTURE, "", "Stretch wrinklemap")
		SHADER_PARAM(BumpStretch,				SHADER_PARAM_TYPE_TEXTURE, "", "Compression bumpmap" )
		SHADER_PARAM(MRAOBias,					SHADER_PARAM_TYPE_VEC4, "", "")

		SHADER_PARAM(DualLobe,					SHADER_PARAM_TYPE_BOOL, "", "")
		SHADER_PARAM(DualLobe_RoughnessBias,	SHADER_PARAM_TYPE_FLOAT, "", "")
		SHADER_PARAM(DualLobe_LerpFactor,		SHADER_PARAM_TYPE_FLOAT, "", "")
	END_SHADER_PARAMS;

	// Initializing parameters
	SHADER_INIT_PARAMS()
	{
		// Whichever we are using, we need it on $BaseTexture in case other Parts of the Engine need it
		// ( VRAD for Example that uses the $BaseTexture's Reflectiviy Value for bounced Lighting )
		if(params[BaseColor]->IsDefined())
		{
			params[BaseTexture]->SetStringValue(params[BaseColor]->GetStringValue());
		}
		else if(params[Diffuse]->IsDefined())
		{
			params[BaseTexture]->SetStringValue(params[Diffuse]->GetStringValue());
			params[SpecularGlossiness]->SetIntValue(1);
		}
		else if (params[BaseTexture]->IsDefined())
		{
			// Expect MetallicRoughness if there isn't anything more specific
			params[BaseColor]->SetStringValue(params[BaseTexture]->GetStringValue());
		}

		// In case there is no Diffuse/BaseTexture you might still have a $Specular Texture
		if (params[Specular]->IsDefined())
		{
			params[SpecularGlossiness]->SetIntValue(1);
		}

		if(params[BumpMap]->IsDefined())
		{
			params[NormalMap]->SetStringValue(params[BumpMap]->GetStringValue());
		}
		else if(params[NormalMap]->IsDefined())
		{
			params[BumpMap]->SetStringValue(params[NormalMap]->GetStringValue());
		}
		else
		{
			// Need something on $BumpMap or we won't get Lighting on Static Props
			// And instead of setting it to "..." I will set it to this so Texture Loads work correctly with $BumpFrame and Proxies
			params[BumpMap]->SetStringValue("dev/flat_normal");
		}

		// PBR relies heavily on envmaps
		// NOTE: It was not considered that SFM Users use literally black Maps for Scene Building
		// They don't *HAVE* actual Cubemaps
		if (!params[EnvMap]->IsDefined())
			params[EnvMap]->SetStringValue("env_cubemap");

		// If using wrinklemaps, all the textures need to be filled in
		if (params[Compress]->IsDefined() || params[BumpCompress]->IsDefined() ||
			params[Stretch]->IsDefined() || params[BumpStretch]->IsDefined())
		{
			if (!params[Compress]->IsDefined())
				params[Compress]->SetStringValue(params[BaseTexture]->GetStringValue());
			if (!params[BumpCompress]->IsDefined())
				params[BumpCompress]->SetStringValue(params[BumpMap]->GetStringValue());
		
			if (!params[Stretch]->IsDefined())
				params[Stretch]->SetStringValue(params[BaseTexture]->GetStringValue());
			if (!params[BumpStretch]->IsDefined())
				params[BumpStretch]->SetStringValue(params[BumpMap]->GetStringValue());
		}

		// NUKE: Default Value is 0 even if you don't set it
		InitIntParam(BumpFrame, params, 0);

		// FIXME: Bracket Spacing
		InitFloatParam(EmissiveFactor, params, 1.0f);
		InitFloatParam(SpecularFactor, params, 1.0f);
		InitFloatParam(SSSIntensity, params, 1.0f);
		InitFloatParam(SSSPowerScale, params, 1.0f);

		// NUKE: This is a Color Param, it's default Value is 1 1 1 if you set it here or not
		InitVecParam(SSSColor, params, 1, 1, 1);

		InitFloatParam(DualLobe_RoughnessBias, params, -0.2f);
		InitFloatParam(DualLobe_LerpFactor, params, 0.5f);

		// "Parallax and wrinkle are incompatible"
		if (!mat_pbr_parallaxmap.GetBool() || params[Compress]->IsDefined())
		{
			params[Parallax]->SetIntValue(0);
		}

		// If no MRAO is defined && not using SpecularGlossiness
		// Set some default MRAO Values by subtracting from the White Texture
		if(!params[MRAOTexture]->IsDefined() && params[SpecularGlossiness]->GetIntValue() == 0)
		{
			InitVecParam(MRAOBias, params, -1.0f, -0.2f, 0.0f, 0.0f);
		}
		else if (!params[Specular]->IsDefined() && params[SpecularGlossiness]->GetIntValue() != 0)
		{
			InitVecParam(MRAOBias, params, -1.0f, 0.0f, 0.0f, 0.0f);
		}
	};

	// Define shader fallback
	SHADER_FALLBACK
	{
		return 0;
	};

	SHADER_INIT
	{
		// Load all the Texture
		LoadTexture(BaseTexture, TEXTUREFLAGS_SRGB);
		LoadTexture(BaseColor, TEXTUREFLAGS_SRGB);
		LoadTexture(Diffuse, TEXTUREFLAGS_SRGB);

		// Material Values
		LoadTexture(MRAOTexture, NULL);
		LoadTexture(Specular, NULL);

		// Normal Maps
		LoadBumpMap(BumpMap);
		LoadBumpMap(NormalMap);

		int nEnvMapFlags = g_pHardwareConfig->GetHDRType() == HDR_TYPE_NONE ? TEXTUREFLAGS_SRGB : 0;
		nEnvMapFlags |= TEXTUREFLAGS_ALL_MIPS;
		LoadCubeMap(EnvMap, nEnvMapFlags);

		LoadTexture(EmissionTexture, TEXTUREFLAGS_SRGB);

		// FIXME: This wasted AN ENTIRE samplers for some Greyscale Information that could be derived from an existing free Color Channel
		LoadTexture(ThicknessTexture);

		// NUKE: This is everything but PBR why is this even a Thing?
		// If someone needs this, stop using PBR because this is not PBR
		LoadTexture(LightWarpTexture);

		if (params[Compress]->IsDefined())
		{
			LoadTexture(Compress, TEXTUREFLAGS_SRGB);
			LoadBumpMap(BumpCompress);
			LoadTexture(Stretch, TEXTUREFLAGS_SRGB);
			LoadBumpMap(BumpStretch);
		}

		// FIXME3: Half of these Flags are unneeded, filter them out
		if (IS_FLAG_SET(MATERIAL_VAR_MODEL))
		{
			SET_FLAGS2(MATERIAL_VAR2_SUPPORTS_HW_SKINNING);             // Required for skinning
			SET_FLAGS2(MATERIAL_VAR2_DIFFUSE_BUMPMAPPED_MODEL);         // Required for dynamic lighting
			SET_FLAGS2(MATERIAL_VAR2_NEEDS_TANGENT_SPACES);             // Required for dynamic lighting
			SET_FLAGS2(MATERIAL_VAR2_LIGHTING_VERTEX_LIT);              // Required for dynamic lighting
			SET_FLAGS2(MATERIAL_VAR2_NEEDS_BAKED_LIGHTING_SNAPSHOTS);   // Required for ambient cube
			SET_FLAGS2(MATERIAL_VAR2_SUPPORTS_FLASHLIGHT);              // Required for flashlight
			SET_FLAGS2(MATERIAL_VAR2_USE_FLASHLIGHT);                   // Required for flashlight
		}
		else // Brushes and Displacements and also everything else which is wrong
		{
			SET_FLAGS2(MATERIAL_VAR2_LIGHTING_LIGHTMAP);                // Required for lightmaps
			SET_FLAGS2(MATERIAL_VAR2_LIGHTING_BUMPED_LIGHTMAP);         // Required for lightmaps
			SET_FLAGS2(MATERIAL_VAR2_SUPPORTS_FLASHLIGHT);              // Required for flashlight
			SET_FLAGS2(MATERIAL_VAR2_USE_FLASHLIGHT);                   // Required for flashlight
		}

		// SFM Shenanigans presumably
		SET_FLAGS2( MATERIAL_VAR2_USE_GBUFFER0 );
		SET_FLAGS2( MATERIAL_VAR2_USE_GBUFFER1 );
	};

	SHADER_DRAW
	{
		bool bHasFlashlight = UsingFlashlight(params); // FIXME: Outdated Variable Name
		bool bIsAlphaTested = IS_FLAG_SET(MATERIAL_VAR_ALPHATEST) != 0; // FIXME: ( != 0) != 0 lol

		// Material Value Booleans
		bool bSpecularGlossiness = params[SpecularGlossiness]->GetIntValue() != 0;
		bool bHasBaseColor = !bSpecularGlossiness && params[BaseColor]->IsTexture();
		bool bHasMRAOTexture = !bSpecularGlossiness && params[MRAOTexture]->IsTexture();
		bool bHasDiffuse = bSpecularGlossiness && params[Diffuse]->IsTexture();
		bool bHasSpecular = bSpecularGlossiness && params[Specular]->IsTexture();
		bool bHasNormalMap = params[NormalMap]->IsTexture();

		bool bHasEmissionTexture = params[EmissionTexture]->IsTexture();
#ifndef SFM_BLACKBOX_MODE
		bool bHasEnvMap = params[EnvMap]->IsTexture();

		// NUKE: Non-sense or force. Why is there two of different ways of deriving Ambient
		// Physically based workarounds
//		bool bUseEnvAmbient = params[UseEnvAmbient]->GetIntValue();
#endif
		bool bHasDualLobe = params[DualLobe]->GetIntValue() != 0;

		// IsDefined() is not real on Shader Draw; This doesn't make any sense
		bool bHasColor = true; // params[Color1]->IsDefined();

		// FIXME: Non-sensical Variable Name, this is bIsModel. Did you know Models can also be lightmapped?
		// FIXME: just nuke Brush Support, who is going to use this on SFM
		bool bLightMapped = !IS_FLAG_SET(MATERIAL_VAR_MODEL);


		bool bThicknessTexture = !bLightMapped && params[ThicknessTexture]->IsTexture();

		// Can't have lightwarp and SSS together
		// ShiroDkxtro2: Why? Modern LUT-based implementations of SSS use a "LightWarpTexture" for SSS
		bool bLightwarpTexture = !bThicknessTexture && params[LightWarpTexture]->IsTexture();

		// Only supported on models
		bool bWrinkleMapping = !bLightMapped && params[Compress]->IsTexture();

		bool bHasParallax = params[Parallax]->GetIntValue() != 0;

		// Determining whether we're dealing with a fully opaque material
		// FIXME: Transluceny on PBR is more than just simple Alphablending
		BlendType_t nBlendType = EvaluateBlendRequirements(BaseTexture, true);
		bool bFullyOpaque = (nBlendType != BT_BLENDADD) && (nBlendType != BT_BLEND) && !bIsAlphaTested;

		//==========================================================================//
		// Static Snapshot of the Shader Settings
		//==========================================================================//
		if (IsSnapshotting())
		{
			//==========================================================================//
			// General Rendering Setup
			//==========================================================================//
			
			// If alphatest is on, enable it
			if(bIsAlphaTested)
			{
				pShaderShadow->EnableAlphaTest(true);

				const float f1AlphaTestReference = params[AlphaTestReference]->GetFloatValue();
				if (f1AlphaTestReference > 0.0f)
				{
					pShaderShadow->AlphaFunc(SHADER_ALPHAFUNC_GEQUAL, f1AlphaTestReference);
				}
			}

			// FIXME: This doesn't consider Translucents like Stock Shaders
			if (bHasFlashlight )
			{
				if(IS_FLAG_SET(MATERIAL_VAR_TRANSLUCENT))
				{
					pShaderShadow->EnableBlending(true);
					pShaderShadow->BlendFunc(SHADER_BLEND_SRC_ALPHA, SHADER_BLEND_ONE); // Additive blending
				}
				else
				{
					pShaderShadow->EnableBlending(true);
					pShaderShadow->BlendFunc(SHADER_BLEND_ONE, SHADER_BLEND_ONE); // Additive blending				
				}
			}
			else
			{
				SetDefaultBlendingShadowState(BaseTexture, true);
			}

			// "See common_ps_fxc.h line 349"
			// The Shader will output Linear Values and the Engine can worry about converting them to Gamma
			pShaderShadow->EnableSRGBWrite(true);

			// Projected Texture fades to black since it renders additively
			if (bHasFlashlight)
				FogToBlack();
			else
				DefaultFog();

			// If we don't use Alpha for Opacity, write Depth to DestAlpha for Particles, or the HeightFogFactor
			pShaderShadow->EnableAlphaWrites(bFullyOpaque);

			//==========================================================================//
			// Vertex Shader - Vertex Format
			//==========================================================================//

			if (IS_FLAG_SET(MATERIAL_VAR_MODEL))
			{
				unsigned int nFlags = VERTEX_POSITION | VERTEX_NORMAL | VERTEX_FORMAT_COMPRESSED;

				int nTexCoords = 1;
				int nUserDataSize = 4;

				pShaderShadow->VertexShaderVertexFormat(nFlags, nTexCoords, NULL, nUserDataSize);
			}
			else
			{
				unsigned int nFlags = VERTEX_POSITION | VERTEX_NORMAL;

				int nTexCoords = 3; // Texture UV, Lightmap UV, Lightmap Offset
				int nUserDataSize = 0;

				pShaderShadow->VertexShaderVertexFormat(nFlags, nTexCoords, NULL, nUserDataSize);
			}

			//==========================================================================//
			// Sampler Setup
			//==========================================================================//

			// s0, s1, s2
			if(bSpecularGlossiness)
			{
				pShaderShadow->EnableTexture(SAMPLER_BASECOLOR, true);
				pShaderShadow->EnableSRGBRead(SAMPLER_BASECOLOR, true);
				pShaderShadow->EnableTexture(SAMPLER_MRAO, true);
				pShaderShadow->EnableSRGBRead(SAMPLER_MRAO, false);
			}
			else
			{
				pShaderShadow->EnableTexture(SAMPLER_DIFFUSE, true);
				pShaderShadow->EnableSRGBRead(SAMPLER_DIFFUSE, true);
				pShaderShadow->EnableTexture(SAMPLER_SPECULAR, true);
				pShaderShadow->EnableSRGBRead(SAMPLER_SPECULAR, false);			
			}
			pShaderShadow->EnableTexture(SAMPLER_NORMAL, true);
			pShaderShadow->EnableSRGBRead(SAMPLER_NORMAL, false);

			// s3, s4, s5, s6
			if (bWrinkleMapping)
			{
				pShaderShadow->EnableTexture(SAMPLER_COMPRESS, true); 
				pShaderShadow->EnableSRGBRead(SAMPLER_COMPRESS, true);
				pShaderShadow->EnableTexture(SAMPLER_STRETCH, true); 
				pShaderShadow->EnableSRGBRead(SAMPLER_STRETCH, true);
				pShaderShadow->EnableTexture(SAMPLER_BUMPCOMPRESS, true); 
				pShaderShadow->EnableSRGBRead(SAMPLER_BUMPCOMPRESS, false);
				pShaderShadow->EnableTexture(SAMPLER_BUMPSTRETCH, true); 
				pShaderShadow->EnableSRGBRead(SAMPLER_BUMPSTRETCH, false);
			}

			// s7
			// Rendertargets are (usually) sRGB
			pShaderShadow->EnableTexture(SAMPLER_SSAO, true);
			pShaderShadow->EnableSRGBRead(SAMPLER_SSAO, true);

			// s8
			if(bHasEmissionTexture)
			{
				pShaderShadow->EnableTexture(SAMPLER_EMISSIVE, true);
				pShaderShadow->EnableSRGBRead(SAMPLER_EMISSIVE, true);
			}

			// s9
			if (bLightwarpTexture)
			{
				pShaderShadow->EnableTexture(SAMPLER_LIGHTWARP, true); 
				pShaderShadow->EnableSRGBRead(SAMPLER_LIGHTWARP, false);
			}
			// s10
			else if (bThicknessTexture)
			{
				pShaderShadow->EnableTexture(SAMPLER_THICKNESS, true); 
				pShaderShadow->EnableSRGBRead(SAMPLER_THICKNESS, false);
			}

			// s12, s13, s14
			if (bHasFlashlight)
			{
				pShaderShadow->EnableTexture(SAMPLER_PROJTEXCOOKIE, true);
				pShaderShadow->EnableSRGBRead(SAMPLER_PROJTEXCOOKIE, true);

				pShaderShadow->EnableTexture(SAMPLER_RANDOMROTATION, true);

				pShaderShadow->EnableTexture(SAMPLER_SHADOWDEPTH, true);
				pShaderShadow->EnableSRGBRead(SAMPLER_SHADOWDEPTH, false);
				pShaderShadow->SetShadowDepthFiltering(SAMPLER_SHADOWDEPTH);
			}
			else
			{
				// s14
#ifndef SFM_BLACKBOX_MODE
				if (bHasEnvMap)
				{
					pShaderShadow->EnableTexture(SAMPLER_ENVMAP, true); // Envmap
					if (g_pHardwareConfig->GetHDRType() == HDR_TYPE_NONE)
						pShaderShadow->EnableSRGBRead(SAMPLER_ENVMAP, true); // Envmap is only sRGB with HDR disabled?
				}
#endif

				// s13
				if (bLightMapped)
				{
					pShaderShadow->EnableTexture(SAMPLER_LIGHTMAP, true);       // Lightmap texture
					pShaderShadow->EnableSRGBRead(SAMPLER_LIGHTMAP, false);     // Lightmaps aren't sRGB
				}
			}

			//==========================================================================//
			// Set Static Shaders
			//==========================================================================//

			// SSAO path
			bool bWorldNormal = ( ENABLE_FIXED_LIGHTING_OUTPUTNORMAL_AND_DEPTH ==
							  ( IS_FLAG2_SET( MATERIAL_VAR2_USE_GBUFFER0 ) + 2 * IS_FLAG2_SET( MATERIAL_VAR2_USE_GBUFFER1 ) ) );

			// FIXME: Split Brushes and Models into two separate Shaders

			// Setting up static vertex shader
			DECLARE_STATIC_VERTEX_SHADER(pbr_vs30);
			SET_STATIC_VERTEX_SHADER_COMBO(WORLD_NORMAL, bWorldNormal);
//          SET_STATIC_VERTEX_SHADER_COMBO(LIGHTMAPPED, bLightMapped);
			SET_STATIC_VERTEX_SHADER(pbr_vs30);

			if(bHasFlashlight)
			{
				// TODO: Check if the ATI Shadow Format Issue was fixed on SFM
				if(bSpecularGlossiness)
				{
					DECLARE_STATIC_PIXEL_SHADER(pbr_sg_projtex_ps30);
					SET_STATIC_PIXEL_SHADER_COMBO(FLASHLIGHTDEPTHFILTERMODE, g_pHardwareConfig->GetShadowFilterMode());
					SET_STATIC_PIXEL_SHADER_COMBO(PARALLAXOCCLUSION, bHasParallax);
					SET_STATIC_PIXEL_SHADER_COMBO(WORLD_NORMAL, bWorldNormal);
					SET_STATIC_PIXEL_SHADER_COMBO(WRINKLEMAP, bWrinkleMapping);
					SET_STATIC_PIXEL_SHADER_COMBO(SUBSURFACESCATTERING, bThicknessTexture);
					SET_STATIC_PIXEL_SHADER_COMBO(DUALLOBE, bHasDualLobe);
					SET_STATIC_PIXEL_SHADER(pbr_sg_projtex_ps30);
				}
				else
				{
					DECLARE_STATIC_PIXEL_SHADER(pbr_mrao_projtex_ps30);
					SET_STATIC_PIXEL_SHADER_COMBO(FLASHLIGHTDEPTHFILTERMODE, g_pHardwareConfig->GetShadowFilterMode());
					SET_STATIC_PIXEL_SHADER_COMBO(PARALLAXOCCLUSION, bHasParallax);
					SET_STATIC_PIXEL_SHADER_COMBO(WORLD_NORMAL, bWorldNormal);
					SET_STATIC_PIXEL_SHADER_COMBO(WRINKLEMAP, bWrinkleMapping);
					SET_STATIC_PIXEL_SHADER_COMBO(SUBSURFACESCATTERING, bThicknessTexture);
					SET_STATIC_PIXEL_SHADER_COMBO(DUALLOBE, bHasDualLobe);
					SET_STATIC_PIXEL_SHADER(pbr_mrao_projtex_ps30);
				}
			}
			else
			{
				if (bSpecularGlossiness)
				{
					DECLARE_STATIC_PIXEL_SHADER(pbr_sg_ps30);
					SET_STATIC_PIXEL_SHADER_COMBO(EMISSIVE, bHasEmissionTexture); // FIXME: Make additively rendered pass to save on Samplers
					SET_STATIC_PIXEL_SHADER_COMBO(PARALLAXOCCLUSION, bHasParallax);
					SET_STATIC_PIXEL_SHADER_COMBO(WORLD_NORMAL, bWorldNormal);
					SET_STATIC_PIXEL_SHADER_COMBO(WRINKLEMAP, bWrinkleMapping);
					SET_STATIC_PIXEL_SHADER_COMBO(SUBSURFACESCATTERING, bThicknessTexture);
					SET_STATIC_PIXEL_SHADER_COMBO(DUALLOBE, bHasDualLobe);
					SET_STATIC_PIXEL_SHADER(pbr_sg_ps30);
				}
				else
				{
					DECLARE_STATIC_PIXEL_SHADER(pbr_mrao_ps30);
					SET_STATIC_PIXEL_SHADER_COMBO(EMISSIVE, bHasEmissionTexture); // FIXME: Make additively rendered pass to save on Samplers
					SET_STATIC_PIXEL_SHADER_COMBO(PARALLAXOCCLUSION, bHasParallax);
					SET_STATIC_PIXEL_SHADER_COMBO(WORLD_NORMAL, bWorldNormal);
					SET_STATIC_PIXEL_SHADER_COMBO(WRINKLEMAP, bWrinkleMapping);
					SET_STATIC_PIXEL_SHADER_COMBO(SUBSURFACESCATTERING, bThicknessTexture);
					SET_STATIC_PIXEL_SHADER_COMBO(DUALLOBE, bHasDualLobe);
					SET_STATIC_PIXEL_SHADER(pbr_mrao_ps30);
				}
			}

			//==========================================================================//
			// PI Command Buffer
			//==========================================================================//

			float flLScale = pShaderShadow->GetLightMapScaleFactor();

			PI_BeginCommandBuffer();

			if(!bLightMapped)
			{
#ifndef SFM_BLACKBOX_MODE
				// Send ambient cube to the pixel sh
				PI_SetPixelShaderAmbientLightCube( PSREG_AMBIENT_CUBE );
#endif

				// Send lighting array to the pixel shader
				PI_SetPixelShaderLocalLighting( PSREG_LIGHT_INFO_ARRAY );
			}

			// Set up shader modulation color
			PI_SetModulationPixelShaderDynamicState_LinearScale_ScaleInW( PSREG_DIFFUSE_MODULATION, flLScale );			

			PI_EndCommandBuffer();
		}

		//==========================================================================//
		// Entirely Dynamic Commands
		//==========================================================================//
		if(pShaderAPI)
		{
			//==========================================================================//
			// Bind Textures
			//==========================================================================//

			// FIXME: Order by Sampler

			bool bLightingOnly = mat_fullbright.GetInt() == 2 && !IS_FLAG_SET(MATERIAL_VAR_NO_DEBUG_OVERRIDE);

			if(bSpecularGlossiness)
			{
				// Setting up albedo texture
				if (!bLightingOnly && bHasDiffuse)
				{
					BindTexture(SAMPLER_DIFFUSE, Diffuse, Frame);
				}
				else
				{
					pShaderAPI->BindStandardTexture(SAMPLER_DIFFUSE, TEXTURE_GREY);
				}
			}
			else
			{
				// Setting up albedo texture
				if (!bLightingOnly && bHasBaseColor)
				{
					BindTexture(SAMPLER_BASECOLOR, BaseColor, Frame);
				}
				else
				{
					pShaderAPI->BindStandardTexture(SAMPLER_BASECOLOR, TEXTURE_GREY);
				}
			}

			// Setting up environment map
#ifndef SFM_BLACKBOX_MODE
			if (mat_specular.GetBool() && bHasEnvMap)
			{
				// FIXME: EnvMapFrame
				BindTexture(SAMPLER_ENVMAP, EnvMap, 0); // FIXME: Missing Frame Parameter
			}
			else
			{
				// This is also the mat_specular 0 Case that for some Reason is handled way below
				pShaderAPI->BindStandardTexture(SAMPLER_ENVMAP, TEXTURE_BLACK);
			}
#endif

			// Setting up emissive texture
			if (bHasEmissionTexture)
			{
				BindTexture(SAMPLER_EMISSIVE, EmissionTexture, 0); // FIXME: Missing Frame Parameter
			}

			// Setting up normal map
			// NOTE: A default Normal Map is defined in Param Init, but there is still a Fallback here
			if (bHasNormalMap)
			{
				BindTexture(SAMPLER_NORMAL, NormalMap, BumpFrame);
			}
			else
			{
				pShaderAPI->BindStandardTexture(SAMPLER_NORMAL, TEXTURE_NORMALMAP_FLAT);
			}

			if (bSpecularGlossiness)
			{
				if (bHasSpecular)
				{
					BindTexture(SAMPLER_SPECULAR, Specular, 0); // FIXME: Missing Frame Parameter
				}
				else
				{
					pShaderAPI->BindStandardTexture(SAMPLER_SPECULAR, TEXTURE_GREY_ALPHA_ZERO);
				}
			}
			else
			{
				if (bHasMRAOTexture)
				{
					BindTexture(SAMPLER_MRAO, MRAOTexture, 0); // FIXME: Missing Frame Parameter
				}
				else
				{
					pShaderAPI->BindStandardTexture(SAMPLER_MRAO, TEXTURE_WHITE);
				}
			}

			if (bThicknessTexture)
			{
				BindTexture(SAMPLER_THICKNESS, ThicknessTexture, 0); // FIXME: Missing Frame Parameter
			}
			else if (bLightwarpTexture)
			{
				BindTexture(SAMPLER_LIGHTWARP, LightWarpTexture, 0); // FIXME: Missing Frame Parameter
			}

			if (bWrinkleMapping)
			{
				BindTexture(SAMPLER_COMPRESS, Compress, 0); // FIXME: Missing Frame Parameter
				BindTexture(SAMPLER_STRETCH, Stretch, 0); // FIXME: Missing Frame Parameter
				BindTexture(SAMPLER_BUMPCOMPRESS, BumpCompress, 0); // FIXME: Missing Frame Parameter
				BindTexture(SAMPLER_BUMPSTRETCH, BumpStretch, 0); // FIXME: Missing Frame Parameter
			}

			// Setting lightmap texture
			if (bLightMapped)
				s_pShaderAPI->BindStandardTexture(SAMPLER_LIGHTMAP, TEXTURE_LIGHTMAP);

			// Ambient occlusion
			ITexture* pAOTexture = pShaderAPI->GetTextureRenderingParameter(TEXTURE_RENDERPARM_AMBIENT_OCCLUSION);
			if (pAOTexture)
				BindTexture(SAMPLER_SSAO, pAOTexture);
			else
				pShaderAPI->BindStandardTexture(SAMPLER_SSAO, TEXTURE_WHITE);

			//==========================================================================//
			// Setup Constant Registers
			//==========================================================================//

			// FIXME: Order by Constant Register

			// Setting up vmt color
			// FIXME: Standardise
			Vector4D color(0, 0, 0, 0);
			if (bHasColor)
			{
				params[Color1]->GetVecValue(color.Base(), 3);
			}
			else
			{
				color.Init(1, 1, 1);
			}
			color.w = float(mat_fullbright.GetInt() == 1);
			pShaderAPI->SetPixelShaderConstant(PSREG_SELFILLUMTINT, color.Base());

			// Getting the light state
			// FIXME: Model only State
			LightState_t lightState;
			pShaderAPI->GetDX9LightState(&lightState);

			if (bHasDualLobe)
			{
				float cDualLobeControls[4] =
				{
					params[DualLobe_RoughnessBias]->GetFloatValue(),
					clamp(params[DualLobe_LerpFactor]->GetFloatValue(), 0.0f, 1.0f),
					0.0f,
					0.0f
				};
				pShaderAPI->SetPixelShaderConstant(PSREG_SELFILLUM_SCALE_BIAS_EXP, cDualLobeControls);
			}

			// Brushes don't need ambient cubes or dynamic lights
			if (!IS_FLAG_SET(MATERIAL_VAR_MODEL))
			{
				lightState.m_bAmbientLight = false;
				lightState.m_nNumLights = 0;
			}

			// FIXME: Standardise in a LUX Way because this is like Stock Shaders, all over the place ( probably because it's copy-pasted Code )
			// Setting up the flashlight related textures and variables
			FlashlightState_t flashlightState;
			VMatrix flashlightWorldToTexture;
			bool bFlashlightShadows = false;
			if (bHasFlashlight)
			{
				ITexture* pFlashlightDepthTexture;
				flashlightState = pShaderAPI->GetFlashlightStateEx(flashlightWorldToTexture, &pFlashlightDepthTexture);
				bFlashlightShadows = flashlightState.m_bEnableShadows && (pFlashlightDepthTexture != NULL);

				SetFlashLightColorFromState(flashlightState, pShaderAPI, false, PSREG_FLASHLIGHT_COLOR);

				if (pFlashlightDepthTexture && g_pConfig->ShadowDepthTexture() && flashlightState.m_bEnableShadows)
				{
					// FIXME: Move to BindTexture Stage
					BindTexture(SAMPLER_SHADOWDEPTH, pFlashlightDepthTexture, 0);
					pShaderAPI->BindStandardTexture(SAMPLER_RANDOMROTATION, TEXTURE_SHADOW_NOISE_2D);
				}
				else
				{
					// Always bind Things to your Samplers or expect corrupted Results
					pShaderAPI->BindStandardTexture(SAMPLER_SHADOWDEPTH, TEXTURE_BLACK);
					pShaderAPI->BindStandardTexture(SAMPLER_RANDOMROTATION, TEXTURE_BLACK);
				}
			}

			float vEyePos_SpecExponent[4];
			pShaderAPI->GetWorldSpaceCameraPosition(vEyePos_SpecExponent);

#ifndef SFM_BLACKBOX_MODE
			// FIXME: Add a warning, force people to have an adequate EnvMap Resolution
			// FIXME: Could be stored in Context Data?
			int iEnvMapLOD = 6;
			auto envTexture = params[EnvMap]->GetTextureValue();
			if (envTexture)
			{
				// Get power of 2 of texture width
				int width = envTexture->GetMappingWidth();
				int mips = 0;
				while (width >>= 1)
					++mips;

				// Cubemap has 4 sides so 2 mips less
				iEnvMapLOD = mips;
			}

			// Dealing with very high and low resolution cubemaps
			if (iEnvMapLOD > 12)
				iEnvMapLOD = 12;
			if (iEnvMapLOD < 4)
				iEnvMapLOD = 4;

			// This has some spare space
			vEyePos_SpecExponent[3] = iEnvMapLOD;
#endif

			pShaderAPI->SetPixelShaderConstant(PSREG_EYEPOS_SPEC_EXPONENT, vEyePos_SpecExponent, 1);

			// Setting up base texture transform
			// FIXME: Use a Macro Map for this because I don't trust these random Enums that love to vary across branches
			SetVertexShaderTextureTransform(VERTEX_SHADER_SHADER_SPECIFIC_CONST_0, BaseTextureTransform);

			// Sending fog info to the pixel shader
			pShaderAPI->SetPixelShaderFogParams(PSREG_FOG_PARAMS);

			// Metalness, roughtness, ambient occlusion, SSAO Factors
			float flSSAOStrength = 1.0f;
			if (bHasFlashlight)
				flSSAOStrength *= flashlightState.m_flAmbientOcclusion;

			// FIXME: Use Bias Values because multiplication does not f'n work with 0.0 and makes anything hard to pinpoint to an exact Value
			// FIXME: Single Parameter not 4
			float cMRAOFactors[4];
			params[MRAOBias]->GetVecValue(cMRAOFactors, 4);
			cMRAOFactors[3] += pbr_microshadows_globalstrength.GetFloat();
			cMRAOFactors[3] = clamp(cMRAOFactors[3], 0.0f, 1.0f); // Saturate to avoid Explosions
			pShaderAPI->SetPixelShaderConstant(PSREG_PBR_MRAO_FACTORS, cMRAOFactors);

			// Emissive, specular factors, SSS intensity and power scale 
			float vExtraFactors[4] =
			{
				GetFloatParam(EmissiveFactor, params, 1.0f),
				GetFloatParam(SpecularFactor, params, 1.0f), // Wat? What is the point of $MetalnessFactor if we end up having two Parameters to handle the same Thing
				GetFloatParam(SSSIntensity, params, 1.0f),
				GetFloatParam(SSSPowerScale, params, 1.0f)
			};
			pShaderAPI->SetPixelShaderConstant(PSREG_PBR_EXTRA_FACTORS, vExtraFactors, 1);

			float vSSSColor[4] = { 0, 0, 0, 0 };
			params[SSSColor]->GetVecValue(vSSSColor, 3);
			pShaderAPI->SetPixelShaderConstant(PSREG_PBR_SSS_COLOR, vSSSColor, 1);

			// Need this for sampling SSAO
			pShaderAPI->SetScreenSizeForVPOS();

			// Pass FarZ for SSAO
			int nLightingPreviewMode = pShaderAPI->GetIntRenderingParameter(INT_RENDERPARM_ENABLE_FIXED_LIGHTING);
			if (nLightingPreviewMode == ENABLE_FIXED_LIGHTING_OUTPUTNORMAL_AND_DEPTH)
			{
				float vEyeDir[4];
				pShaderAPI->GetWorldSpaceCameraDirection(vEyeDir);

				float flFarZ = pShaderAPI->GetFarZ();
				vEyeDir[0] /= flFarZ;	// Divide by farZ for SSAO algorithm
				vEyeDir[1] /= flFarZ;
				vEyeDir[2] /= flFarZ;
				pShaderAPI->SetVertexShaderConstant(VERTEX_SHADER_SHADER_SPECIFIC_CONST_8, vEyeDir);
			}

			// More flashlight related stuff
			// FIXME: Same as above, standardise this
			if (bHasFlashlight)
			{
				float atten[4], pos[4], tweaks[4];
				SetFlashLightColorFromState(flashlightState, pShaderAPI, false, PSREG_FLASHLIGHT_COLOR);

				BindTexture(SAMPLER_PROJTEXCOOKIE, flashlightState.m_pSpotlightTexture, flashlightState.m_nSpotlightTextureFrame);

				// Set the flashlight attenuation factors
				atten[0] = flashlightState.m_fConstantAtten;
				atten[1] = flashlightState.m_fLinearAtten;
				atten[2] = flashlightState.m_fQuadraticAtten;
				atten[3] = flashlightState.m_FarZAtten;
				pShaderAPI->SetPixelShaderConstant(PSREG_FLASHLIGHT_ATTENUATION, atten, 1);

				// Set the flashlight origin
				pos[0] = flashlightState.m_vecLightOrigin[0];
				pos[1] = flashlightState.m_vecLightOrigin[1];
				pos[2] = flashlightState.m_vecLightOrigin[2];
				pShaderAPI->SetPixelShaderConstant(PSREG_FLASHLIGHT_POSITION_RIM_BOOST, pos, 1);

				pShaderAPI->SetPixelShaderConstant(PSREG_FLASHLIGHT_TO_WORLD_TEXTURE, flashlightWorldToTexture.Base(), 4);

				// Tweaks associated with a given flashlight
				tweaks[0] = ShadowFilterFromState(flashlightState);
				tweaks[1] = ShadowAttenFromState(flashlightState);
				HashShadow2DJitter(flashlightState.m_flShadowJitterSeed, &tweaks[2], &tweaks[3]);
				pShaderAPI->SetPixelShaderConstant(PSREG_ENVMAP_TINT__SHADOW_TWEAKS, tweaks, 1);

				// Uberlight
				SetupUberlightFromState(pShaderAPI, flashlightState);
			}

			if(bHasParallax)
			{			
				float flParams[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
				// Parallax Depth (the strength of the effect)
				flParams[0] = GetFloatParam(ParallaxDepth, params, 3.0f);
				// Parallax Center (the height at which it's not moved)
				flParams[1] = GetFloatParam(ParallaxCenter, params, 3.0f);
				pShaderAPI->SetPixelShaderConstant(PSREG_SHADER_CONTROLS, flParams, 1);
			}

			//==========================================================================//
			// Set Dynamic Shaders
			//==========================================================================//

			// FIXME: All of this
			// ---
			// Getting fog info
			MaterialFogMode_t fogType = pShaderAPI->GetSceneFogMode();
//          int fogIndex = (fogType == MATERIAL_FOG_LINEAR_BELOW_FOG_Z) ? 1 : 0;

			// Getting skinning info
			int numBones = pShaderAPI->GetCurrentNumBones();

			// Some debugging stuff
			bool bWriteDepthToAlpha = false;
			bool bWriteWaterFogToAlpha = false;
			if (bFullyOpaque)
			{
				bWriteDepthToAlpha = pShaderAPI->ShouldWriteDepthToDestAlpha();
				bWriteWaterFogToAlpha = (fogType == MATERIAL_FOG_LINEAR_BELOW_FOG_Z);
				AssertMsg(!(bWriteDepthToAlpha && bWriteWaterFogToAlpha),
						"Can't write two values to alpha at the same time.");
			}
			// ---

			// Setting up dynamic vertex shader
			DECLARE_DYNAMIC_VERTEX_SHADER(pbr_vs30);
//          SET_DYNAMIC_VERTEX_SHADER_COMBO(DOWATERFOG, fogIndex);
			SET_DYNAMIC_VERTEX_SHADER_COMBO(SKINNING, numBones > 0);
			SET_DYNAMIC_VERTEX_SHADER_COMBO(COMPRESSED_VERTS, (int)vertexCompression);
//          SET_DYNAMIC_VERTEX_SHADER_COMBO(NUM_LIGHTS, lightState.m_nNumLights);
			SET_DYNAMIC_VERTEX_SHADER(pbr_vs30);

			// Setting up dynamic pixel shader
			// FIXME: Optimize Dynamic Combos. This is long compiletimes for no Reason
			if (bHasFlashlight)
			{
				if (bSpecularGlossiness)
				{
					DECLARE_DYNAMIC_PIXEL_SHADER(pbr_sg_projtex_ps30);
					SET_DYNAMIC_PIXEL_SHADER_COMBO(PIXELFOGTYPE, pShaderAPI->GetPixelFogCombo());
					SET_DYNAMIC_PIXEL_SHADER_COMBO(FLASHLIGHTSHADOWS, bFlashlightShadows);
					SET_DYNAMIC_PIXEL_SHADER_COMBO(UBERLIGHT, flashlightState.m_bUberlight);
					SET_DYNAMIC_PIXEL_SHADER(pbr_sg_projtex_ps30);
				}
				else
				{
					DECLARE_DYNAMIC_PIXEL_SHADER(pbr_mrao_projtex_ps30);
					SET_DYNAMIC_PIXEL_SHADER_COMBO(PIXELFOGTYPE, pShaderAPI->GetPixelFogCombo());
					SET_DYNAMIC_PIXEL_SHADER_COMBO(FLASHLIGHTSHADOWS, bFlashlightShadows);
					SET_DYNAMIC_PIXEL_SHADER_COMBO(UBERLIGHT, flashlightState.m_bUberlight);
					SET_DYNAMIC_PIXEL_SHADER(pbr_mrao_projtex_ps30);
				}
			}
			else
			{
				if (bSpecularGlossiness)
				{
					DECLARE_DYNAMIC_PIXEL_SHADER(pbr_sg_ps30);
					SET_DYNAMIC_PIXEL_SHADER_COMBO(NUM_LIGHTS, lightState.m_nNumLights);
					SET_DYNAMIC_PIXEL_SHADER_COMBO(WRITEWATERFOGTODESTALPHA, bWriteWaterFogToAlpha);
					SET_DYNAMIC_PIXEL_SHADER_COMBO(WRITE_DEPTH_TO_DESTALPHA, bWriteDepthToAlpha);
					SET_DYNAMIC_PIXEL_SHADER_COMBO(PIXELFOGTYPE, pShaderAPI->GetPixelFogCombo());
					SET_DYNAMIC_PIXEL_SHADER(pbr_sg_ps30);
				}
				else
				{
					DECLARE_DYNAMIC_PIXEL_SHADER(pbr_mrao_ps30);
					SET_DYNAMIC_PIXEL_SHADER_COMBO(NUM_LIGHTS, lightState.m_nNumLights);
					SET_DYNAMIC_PIXEL_SHADER_COMBO(WRITEWATERFOGTODESTALPHA, bWriteWaterFogToAlpha);
					SET_DYNAMIC_PIXEL_SHADER_COMBO(WRITE_DEPTH_TO_DESTALPHA, bWriteDepthToAlpha);
					SET_DYNAMIC_PIXEL_SHADER_COMBO(PIXELFOGTYPE, pShaderAPI->GetPixelFogCombo());
					SET_DYNAMIC_PIXEL_SHADER(pbr_mrao_ps30);
				}
			}
		}

	   Draw();

	   // TODO: DepthToDestAlpha for Alphatested Materials?
	};
END_SHADER