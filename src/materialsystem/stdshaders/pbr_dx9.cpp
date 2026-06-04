//==================================================================================================
//
// Physically Based Rendering shader for brushes and models
// Adopted from Zombie Master: Reborn, modified for SFM compatibility
// https://github.com/zm-reborn/zmr-game/
//
//==================================================================================================

// Includes for all shaders
#include "cpp_lux_shared.h"
#include "pbr_registermap.h"

#include "vtf/vtf.h"

// Includes for PS30
#include "pbr_vs30.inc"
#include "pbr_mrao_ps30.inc"
#include "pbr_mrao_projtex_ps30.inc"
#include "pbr_sg_ps30.inc"
#include "pbr_sg_projtex_ps30.inc"
#include "pbr_worldnormal_ps30.inc"

// M/R and S/G
const Sampler_t SAMPLER_BASECOLOR		= SHADER_SAMPLER0;
const Sampler_t SAMPLER_DIFFUSE			= SHADER_SAMPLER0;

const Sampler_t SAMPLER_SPECULAR		= SHADER_SAMPLER1;
const Sampler_t SAMPLER_MRAO			= SHADER_SAMPLER1;

const Sampler_t SAMPLER_NORMAL			= SHADER_SAMPLER2;
const Sampler_t SAMPLER_COMPRESS		= SHADER_SAMPLER3;
const Sampler_t SAMPLER_STRETCH			= SHADER_SAMPLER4;
const Sampler_t SAMPLER_BUMPCOMPRESS	= SHADER_SAMPLER5;
const Sampler_t SAMPLER_BUMPSTRETCH		= SHADER_SAMPLER6;
const Sampler_t SAMPLER_SSAO			= SHADER_SAMPLER7;

const Sampler_t SAMPLER_SSSLUT			= SHADER_SAMPLER9;
const Sampler_t SAMPLER_SSSCONTROLS		= SHADER_SAMPLER10;

// Convars
static ConVar pbr_version("pbr_version", "1.14", FCVAR_CHEAT);
static ConVar mat_pbr_parallaxmap("mat_pbr_parallaxmap", "1");

static ConVar pbr_microshadows_globalstrength("pbr_microshadows_globalstrength", "0.50", FCVAR_NONE);

//==========================================================================//
// CommandBuffer Setup
//==========================================================================//
class PBRContext : public LUXPerMaterialContextData
{
public:
	// Snapshot / Dynamic State
	BlendType_t m_nBlendType = BT_NONE;
	bool m_bIsFullyOpaque = false;
};

//==========================================================================//
// Shader Start
//==========================================================================//
BEGIN_VS_SHADER(PBR, "PBR shader")

	// Setting up vmt parameters
	BEGIN_SHADER_PARAMS

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

		// Separate Parameters so for a OpenGL Normal you only need $..FlipG
		SHADER_PARAM(NormalMap_FlipR,			SHADER_PARAM_TYPE_BOOL, "", "")
		SHADER_PARAM(NormalMap_FlipG,			SHADER_PARAM_TYPE_BOOL, "", "")
		SHADER_PARAM(NormalMap_FlipB,			SHADER_PARAM_TYPE_BOOL, "", "")
		SHADER_PARAM(NormalMapFactor,			SHADER_PARAM_TYPE_FLOAT, "", "")

		SHADER_PARAM(AmbientOcclusion,			SHADER_PARAM_TYPE_FLOAT, "", "")

		SHADER_PARAM(BumpFrame,					SHADER_PARAM_TYPE_INTEGER, "0", "Frame number for $bumpmap")
		SHADER_PARAM(Parallax,					SHADER_PARAM_TYPE_BOOL, "0", "Use Parallax Occlusion Mapping.")
		SHADER_PARAM(ParallaxDepth,				SHADER_PARAM_TYPE_FLOAT, "0.0030", "Depth of the Parallax Map")
		SHADER_PARAM(ParallaxCenter,			SHADER_PARAM_TYPE_FLOAT, "0.5", "Center depth of the Parallax Map")
		SHADER_PARAM(EmissiveFactor,			SHADER_PARAM_TYPE_FLOAT, "1.0", "Emissive factor" )
		SHADER_PARAM(SpecularFactor,			SHADER_PARAM_TYPE_FLOAT, "1.0", "Specular factor" )
		SHADER_PARAM(Compress,					SHADER_PARAM_TYPE_TEXTURE, "", "Compression wrinklemap")
		SHADER_PARAM(BumpCompress,				SHADER_PARAM_TYPE_TEXTURE, "", "Stretch bumpmap" )
		SHADER_PARAM(Stretch,					SHADER_PARAM_TYPE_TEXTURE, "", "Stretch wrinklemap")
		SHADER_PARAM(BumpStretch,				SHADER_PARAM_TYPE_TEXTURE, "", "Compression bumpmap" )

		SHADER_PARAM(MRAOMultiplier,			SHADER_PARAM_TYPE_VEC3, "", "")
		SHADER_PARAM(MRAOBias,					SHADER_PARAM_TYPE_VEC3, "", "")
		SHADER_PARAM(MRAOExponent,				SHADER_PARAM_TYPE_VEC3, "", "")
		SHADER_PARAM(MicroShadowBias,			SHADER_PARAM_TYPE_FLOAT, "", "")

		SHADER_PARAM(DualLobe,					SHADER_PARAM_TYPE_BOOL, "", "")
		SHADER_PARAM(DualLobe_RoughnessBias,	SHADER_PARAM_TYPE_FLOAT, "", "")
		SHADER_PARAM(DualLobe_LerpFactor,		SHADER_PARAM_TYPE_FLOAT, "", "")

		SHADER_PARAM(PremultipliedAlpha,		SHADER_PARAM_TYPE_BOOL, "", "")

		SHADER_PARAM(SSS_ControlTexture,		SHADER_PARAM_TYPE_BOOL, "", "Subsurface Scattering Data\n[R] Thickness.\n[G] Curvature.")
		SHADER_PARAM(SSS_ThicknessFlip,			SHADER_PARAM_TYPE_BOOL, "", "Flips the Thickness Texture from $SSSControlTexture.")
		SHADER_PARAM(SSS_ThicknessExponent,		SHADER_PARAM_TYPE_FLOAT, "", "Exponent Factor for the Thickness Texture.")
		SHADER_PARAM(SSS_CurvatureFlip,			SHADER_PARAM_TYPE_BOOL, "", "Flips the Curvature texture from $SSSControlTexture.")
		SHADER_PARAM(SSS_CurvatureExponent,		SHADER_PARAM_TYPE_FLOAT, "", "Exponent Factor for the Curvature Texture.")
		SHADER_PARAM(SSS_PreintegrationLUT,		SHADER_PARAM_TYPE_TEXTURE, "", "[RGB] Preintegration Lookup-Texture.\nX is Theta, Y is 1/Curvature.")
	END_SHADER_PARAMS

	// Initializing parameters
	SHADER_INIT_PARAMS()
	{
		// Whichever we are using, we need it on $BaseTexture in case other Parts of the Engine need it
		// ( VRAD for Example that uses the $BaseTexture's Reflectiviy Value for bounced Lighting )
		if(IsDefined(BaseColor))
		{
			SetString(BaseTexture, GetString(BaseColor));
		}
		else if(IsDefined(Diffuse))
		{
			SetString(BaseTexture, GetString(Diffuse));
			SetBool(SpecularGlossiness, true);
		}
		else if (IsDefined(BaseTexture))
		{
			// Expect MetallicRoughness if there isn't anything more specific
			SetString(BaseColor, GetString(BaseTexture));
		}

		// In case there is no Diffuse/BaseTexture you might still have a $Specular Texture
		if (IsDefined(Specular))
		{
			SetBool(SpecularGlossiness, true);
		}

		if(IsDefined(BumpMap))
		{
			SetString(NormalMap, GetString(BumpMap));
		}
		else if(IsDefined(NormalMap))
		{
			SetString(BumpMap, GetString(NormalMap));
		}
		else
		{
			// Need something on $BumpMap or we won't get Lighting on Static Props
			// And instead of setting it to "..." I will set it to this so Texture Loads work correctly with $BumpFrame and Proxies
			SetString(BumpMap, "dev/flat_normal");
		}

		// If using wrinklemaps, all the textures need to be filled in
		if (IsDefined(Compress) || IsDefined(BumpCompress) ||
			IsDefined(Stretch) || IsDefined(BumpStretch))
		{
			if (!IsDefined(Compress))
				SetString(Compress, GetString(BaseTexture));
			if (!IsDefined(BumpCompress))
				SetString(BumpCompress, GetString(BumpMap));
		
			if (!IsDefined(Stretch))
				SetString(Stretch, GetString(BaseTexture));
			if (!IsDefined(BumpStretch))
				SetString(BumpStretch, GetString(BumpMap));
		}

		if(!IsDefined(SSS_PreintegrationLUT))
			SetString(SSS_PreintegrationLUT, "LUT_SSSPreintegration");

		DefaultFloat(SSS_ThicknessExponent, 1.0f);
		DefaultFloat(SSS_CurvatureExponent, 1.0f);

		DefaultFloat(NormalMapFactor, 1.0f);

		DefaultFloat(EmissiveFactor, 1.0f);
		DefaultFloat(SpecularFactor, 1.0f);

		DefaultFloat(DualLobe_RoughnessBias, -0.2f);
		DefaultFloat(DualLobe_LerpFactor, 0.5f);

		// "Parallax and wrinkle are incompatible"
		if (!mat_pbr_parallaxmap.GetBool() || IsDefined(Compress))
		{
			SetBool(Parallax, false);
		}

		DefaultFloat3(MRAOMultiplier, 1.0f, 1.0f, 1.0f);
		DefaultFloat3(MRAOExponent, 1.0f, 1.0f, 1.0f);
		DefaultFloat(MicroShadowBias, 0.0f);

		// If no MRAO is defined && not using SpecularGlossiness
		// Set some default MRAO Values by subtracting from the White Texture
		if(!IsDefined(MRAOTexture) && !GetBool(SpecularGlossiness))
		{
			DefaultFloat4(MRAOBias, -1.0f, -0.2f, 0.0f, 0.0f);
		}
		else if (!IsDefined(Specular) && GetBool(SpecularGlossiness))
		{
			DefaultFloat4(MRAOBias, -1.0f, 0.0f, 0.0f, 0.0f);
		}
		else
		{
			DefaultFloat3(MRAOBias, 0.0f, 0.0f, 0.0f);
		}

		DefaultBool(PremultipliedAlpha, true);
		DefaultFloat(AmbientOcclusion, 1.0f);
	}

	// Define shader fallback
	SHADER_FALLBACK
	{
		return 0;
	}

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

		// Avoid Warnings about missing $SSSPreintegrationLUT when not actually using SSS
		if(IsDefined(SSS_ControlTexture))
		{
			LoadTexture(SSS_ControlTexture);
			LoadTexture(SSS_PreintegrationLUT);
		}

		if (IsDefined(Compress))
		{
			LoadTexture(Compress, TEXTUREFLAGS_SRGB);
			LoadBumpMap(BumpCompress);
			LoadTexture(Stretch, TEXTUREFLAGS_SRGB);
			LoadBumpMap(BumpStretch);
		}

		// FIXME3: Half of these Flags are unneeded, filter them out
		if (HasFlag(MATERIAL_VAR_MODEL))
		{
			SetFlag2(MATERIAL_VAR2_SUPPORTS_HW_SKINNING);             // Required for skinning
			SetFlag2(MATERIAL_VAR2_DIFFUSE_BUMPMAPPED_MODEL);         // Required for dynamic lighting
			SetFlag2(MATERIAL_VAR2_NEEDS_TANGENT_SPACES);             // Required for dynamic lighting
			SetFlag2(MATERIAL_VAR2_LIGHTING_VERTEX_LIT);              // Required for dynamic lighting
			SetFlag2(MATERIAL_VAR2_NEEDS_BAKED_LIGHTING_SNAPSHOTS);   // Required for ambient cube
			SetFlag2(MATERIAL_VAR2_SUPPORTS_FLASHLIGHT);              // Required for flashlight
			SetFlag2(MATERIAL_VAR2_USE_FLASHLIGHT);                   // Required for flashlight
		}
		else // Brushes and Displacements and also everything else which is wrong
		{
			SetFlag2(MATERIAL_VAR2_LIGHTING_LIGHTMAP);                // Required for lightmaps
			SetFlag2(MATERIAL_VAR2_LIGHTING_BUMPED_LIGHTMAP);         // Required for lightmaps
			SetFlag2(MATERIAL_VAR2_SUPPORTS_FLASHLIGHT);              // Required for flashlight
			SetFlag2(MATERIAL_VAR2_USE_FLASHLIGHT);                   // Required for flashlight
		}

		// SFM Shenanigans presumably
		SetFlag2(MATERIAL_VAR2_USE_GBUFFER0);
		SetFlag2(MATERIAL_VAR2_USE_GBUFFER1);
	};

	// Virtual Void Override for Context Data
	PBRContext* CreateMaterialContextData() override
	{
		return new PBRContext();
	}

	void PBR_Draw_Internal(IShaderShadow* pShaderShadow, IShaderDynamicAPI* pShaderAPI, CBasePerMaterialContextData** pContextDataPtr)
	{
		// Get Context Data. BaseShader handles creation for us, using the CreateMaterialContextData() virtual
		auto* pContextData = GetMaterialContextData<PBRContext>(pContextDataPtr);

		bool bProjTex = UsingFlashlight(); // FIXME: Outdated Variable Name

		// Material Value Booleans
		bool bSpecularGlossiness = GetBool(SpecularGlossiness);
		bool bHasBaseColor = !bSpecularGlossiness && IsTextureLoaded(BaseColor);
		bool bHasMRAOTexture = !bSpecularGlossiness && IsTextureLoaded(MRAOTexture);
		bool bHasDiffuse = bSpecularGlossiness && IsTextureLoaded(Diffuse);
		bool bHasSpecular = bSpecularGlossiness && IsTextureLoaded(Specular);
		bool bHasNormalMap = IsTextureLoaded(NormalMap);

		bool bHasDualLobe = GetBool(DualLobe);

		bool bHasSubSurfaceScattering = IsTextureLoaded(SSS_ControlTexture);
		bool bWrinkleMapping = IsTextureLoaded(Compress);
		bool bHasParallax = GetBool(Parallax);

		//==========================================================================//
		// Pre-Snapshot Context Data Variables
		//==========================================================================//
		if (IsSnapshottingCommands())
		{
			pContextData->m_nBlendType = ComputeBlendType(BaseTexture, true);
			pContextData->m_bIsFullyOpaque = IsFullyOpaque(pContextData->m_nBlendType);
		}

		//==========================================================================//
		// Static Snapshot of the Shader Settings
		//==========================================================================//
		if (IsSnapshotting())
		{
			//==========================================================================//
			// General Rendering Setup
			//==========================================================================//
			
			// This handles : $IgnoreZ, $Decal, $Nocull, $Znearer, $Wireframe, $AllowAlphaToCoverage
			SetInitialShadowState();

			// For Premultiplied Alpha we need something less conventional
			bool bTranslucent = (pContextData->m_nBlendType == BT_BLEND || pContextData->m_nBlendType == BT_BLENDADD);
			bool bPremultipliedAlpha = !bHasDualLobe && bTranslucent && GetBool(PremultipliedAlpha);
			if (bPremultipliedAlpha)
			{
				// NOTE: No $AlphaTest here

				// The Idea here is
				// specular + (alpha * diffuse) + (alpha * dst)
				EnableAlphaBlending(SHADER_BLEND_ONE, SHADER_BLEND_ONE_MINUS_SRC_ALPHA);
			}
			else
			{
				// Everything Transparency is packed into this Function
				EnableTransparency(pContextData->m_nBlendType);
			}

			// We always need this
			pShaderShadow->EnableAlphaWrites(pContextData->m_bIsFullyOpaque);

			// Weird name, what it actually means : We output linear values
			pShaderShadow->EnableSRGBWrite(true);

			//==========================================================================//
			// Vertex Shader - Vertex Format
			//==========================================================================//

			// Compressed Verts get Normal + Tangent through vNormal ( NORMAL Stream )
			unsigned int nFlags = VERTEX_POSITION | VERTEX_NORMAL | VERTEX_FORMAT_COMPRESSED;

			// Always just one..
			int nTexCoords = 1;

			// Uncompressed Verts get Tangent + Binormal Sign through vUserData ( TANGENT Stream )
			int nUserDataSize = 4;

			pShaderShadow->VertexShaderVertexFormat(nFlags, nTexCoords, NULL, nUserDataSize);

			//==========================================================================//
			// Sampler Setup
			//==========================================================================//

			// s0, s1, s2
			if(bSpecularGlossiness)
			{
				EnableSampler(SAMPLER_DIFFUSE, true);
				EnableSampler(SAMPLER_SPECULAR, true);
			}
			else
			{
				EnableSampler(SAMPLER_BASECOLOR, true);
				EnableSampler(SAMPLER_MRAO, false);
			}
			EnableSampler(SAMPLER_NORMAL, false);

			// s3, s4, s5, s6
			if (bWrinkleMapping)
			{
				EnableSampler(SAMPLER_COMPRESS, true); 
				EnableSampler(SAMPLER_STRETCH, true); 
				EnableSampler(SAMPLER_BUMPCOMPRESS, false); 
				EnableSampler(SAMPLER_BUMPSTRETCH, false); 
			}

			// s7
			// Rendertargets are (usually) sRGB
			EnableSampler(SAMPLER_SSAO, true);

			// s10
			if (bHasSubSurfaceScattering)
			{
				EnableSampler(SAMPLER_SSSCONTROLS, false);
				EnableSampler(SAMPLER_SSSLUT, true); // sRGB
			}

			// s13, s14, s15
			SetupFlashlightSamplers();

			//==========================================================================//
			// Set Static Shaders
			//==========================================================================//

			// Setting up static vertex shader
			DECLARE_STATIC_VERTEX_SHADER(pbr_vs30);
			SET_STATIC_VERTEX_SHADER_COMBO(PROJTEX, bProjTex); // Don't want to compute Lighting
			SET_STATIC_VERTEX_SHADER_COMBO(WORLD_NORMAL, false); // Not SSAO Normal Pass
			SET_STATIC_VERTEX_SHADER_COMBO(WRINKLEMAPS, bWrinkleMapping);
			SET_STATIC_VERTEX_SHADER(pbr_vs30);

			if(bProjTex)
			{
				// TODO: Check if the ATI Shadow Format Issue was fixed on SFM
				if(bSpecularGlossiness)
				{
					DECLARE_STATIC_PIXEL_SHADER(pbr_sg_projtex_ps30);
					SET_STATIC_PIXEL_SHADER_COMBO(PARALLAXOCCLUSION, bHasParallax);
					SET_STATIC_PIXEL_SHADER_COMBO(WRINKLEMAPS, bWrinkleMapping);
					SET_STATIC_PIXEL_SHADER_COMBO(SUBSURFACESCATTERING, bHasSubSurfaceScattering);
					SET_STATIC_PIXEL_SHADER_COMBO(DUALLOBE, bHasDualLobe);
					SET_STATIC_PIXEL_SHADER_COMBO(PREMULTIPLIEDALPHA, bPremultipliedAlpha);
					SET_STATIC_PIXEL_SHADER(pbr_sg_projtex_ps30);
				}
				else
				{
					DECLARE_STATIC_PIXEL_SHADER(pbr_mrao_projtex_ps30);
					SET_STATIC_PIXEL_SHADER_COMBO(PARALLAXOCCLUSION, bHasParallax);
					SET_STATIC_PIXEL_SHADER_COMBO(WRINKLEMAPS, bWrinkleMapping);
					SET_STATIC_PIXEL_SHADER_COMBO(SUBSURFACESCATTERING, bHasSubSurfaceScattering);
					SET_STATIC_PIXEL_SHADER_COMBO(DUALLOBE, bHasDualLobe);
					SET_STATIC_PIXEL_SHADER_COMBO(PREMULTIPLIEDALPHA, bPremultipliedAlpha);
					SET_STATIC_PIXEL_SHADER(pbr_mrao_projtex_ps30);
				}
			}
			else
			{
				if (bSpecularGlossiness)
				{
					DECLARE_STATIC_PIXEL_SHADER(pbr_sg_ps30);
					SET_STATIC_PIXEL_SHADER_COMBO(PARALLAXOCCLUSION, bHasParallax);
					SET_STATIC_PIXEL_SHADER_COMBO(WRINKLEMAPS, bWrinkleMapping);
					SET_STATIC_PIXEL_SHADER_COMBO(SUBSURFACESCATTERING, bHasSubSurfaceScattering);
					SET_STATIC_PIXEL_SHADER_COMBO(DUALLOBE, bHasDualLobe);
					SET_STATIC_PIXEL_SHADER_COMBO(PREMULTIPLIEDALPHA, bPremultipliedAlpha);
					SET_STATIC_PIXEL_SHADER(pbr_sg_ps30);
				}
				else
				{
					DECLARE_STATIC_PIXEL_SHADER(pbr_mrao_ps30);
					SET_STATIC_PIXEL_SHADER_COMBO(PARALLAXOCCLUSION, bHasParallax);
					SET_STATIC_PIXEL_SHADER_COMBO(WRINKLEMAPS, bWrinkleMapping);
					SET_STATIC_PIXEL_SHADER_COMBO(SUBSURFACESCATTERING, bHasSubSurfaceScattering);
					SET_STATIC_PIXEL_SHADER_COMBO(DUALLOBE, bHasDualLobe);
					SET_STATIC_PIXEL_SHADER_COMBO(PREMULTIPLIEDALPHA, bPremultipliedAlpha);
					SET_STATIC_PIXEL_SHADER(pbr_mrao_ps30);
				}
			}

			//==========================================================================//
			// PI Command Buffer
			//==========================================================================//


			if(!bProjTex)
			{
				PI_BeginCommandBuffer();

				// Send ambient cube to the pixel sh
//				PI_SetPixelShaderAmbientLightCube(LUX_PS_FLOAT_AMBIENTCUBE);

				// Send lighting array to the pixel shader
				PI_SetPixelShaderLocalLighting(LUX_PS_FLOAT_LIGHTDATA);

				PI_EndCommandBuffer();
			}
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

			bool bLightingOnly = (mat_fullbright() == 2) && !HasFlag(MATERIAL_VAR_NO_DEBUG_OVERRIDE);

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
					BindTexture(SAMPLER_SPECULAR, Specular, -1); // FIXME: Missing Frame Parameter
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
					BindTexture(SAMPLER_MRAO, MRAOTexture, -1); // FIXME: Missing Frame Parameter
				}
				else
				{
					pShaderAPI->BindStandardTexture(SAMPLER_MRAO, TEXTURE_WHITE);
				}
			}

			if (bHasSubSurfaceScattering)
			{
				BindTexture(SAMPLER_SSSCONTROLS, SSS_ControlTexture, -1); // FIXME: Missing Frame Parameter
				BindTexture(SAMPLER_SSSLUT, SSS_PreintegrationLUT, -1);
			}

			if (bWrinkleMapping)
			{
				BindTexture(SAMPLER_COMPRESS, Compress, Frame); // FIXME: Missing Frame Parameter
				BindTexture(SAMPLER_STRETCH, Stretch, Frame); // FIXME: Missing Frame Parameter
				BindTexture(SAMPLER_BUMPCOMPRESS, BumpCompress, BumpCompress); // FIXME: Missing Frame Parameter
				BindTexture(SAMPLER_BUMPSTRETCH, BumpStretch, BumpFrame); // FIXME: Missing Frame Parameter
			}

			// Ambient occlusion
			// NOTE: If an Object is not opaque it should not have AO ( it will get the AO of the Surfaces behind it )
			// Projected Textures are additive by Nature ( bIsFullyOpaque will be false )
			// In that Case, we have to determine if the Original Pass was additive or translucent.
			bool bBasePassNotOpaque;
			if(bProjTex)
			{	
				// $Translucent will put us on BT_BLENDADD so we can check that
				// Otherwise we need to see if $Additive is set
				bBasePassNotOpaque = pContextData->m_nBlendType == BT_BLENDADD || HasFlag(MATERIAL_VAR_ADDITIVE);
			}
			else
				bBasePassNotOpaque = !pContextData->m_bIsFullyOpaque;

			if(bBasePassNotOpaque)
			{
				pShaderAPI->BindStandardTexture(SAMPLER_SSAO, TEXTURE_WHITE);
			}
			else
			{			
				ITexture* pAOTexture = pShaderAPI->GetTextureRenderingParameter(TEXTURE_RENDERPARM_AMBIENT_OCCLUSION);
				if (pAOTexture)
					BindTexture(SAMPLER_SSAO, pAOTexture);
				else
					pShaderAPI->BindStandardTexture(SAMPLER_SSAO, TEXTURE_WHITE);
			}

			bool bUberlight = false;
			bool bProjTexShadows = SetupFlashlight(&bUberlight);

			//==========================================================================//
			// Setup Constant Registers
			//==========================================================================//

			// VS c223, c224 - $BaseTextureTransform
			SetVertexShaderTextureTransform(LUX_VS_TEXTURETRANSFORM_01, BaseTextureTransform);

			// c0 Controls1
			float4 cControls1 = 0.0f;
			cControls1.x = mat_fullbright() == 1 ? 1.0f : 0.0f;
			cControls1.y = GetFloat(MicroShadowBias) + pbr_microshadows_globalstrength.GetFloat();
			cControls1.y = fxsaturate(cControls1.y);
			pShaderAPI->SetPixelShaderConstant(PBR_PS_FLOAT_CONTROLS1, cControls1);

			// c1
			if(bHasParallax || bHasDualLobe)
			{
				float4 cControls2;
				cControls2.x = GetFloat(DualLobe_RoughnessBias);
				cControls2.y = fxsaturate(GetFloat(DualLobe_LerpFactor));
				cControls2.z = GetFloat(ParallaxDepth);
				cControls2.w = GetFloat(ParallaxCenter);
				pShaderAPI->SetPixelShaderConstant(PBR_PS_FLOAT_CONTROLS2, cControls2);					 
			}

			// c4, c5
			if(bHasSubSurfaceScattering)
			{
				// No additional Controls right now
				float4 cSSSControls1;
				cSSSControls1.x = GetBool(SSS_ThicknessFlip) ? 1.0f : 0.0f;
				cSSSControls1.y = GetFloat(SSS_ThicknessExponent);
				cSSSControls1.z = GetBool(SSS_CurvatureFlip) ? 1.0f : 0.0f;
				cSSSControls1.w = GetFloat(SSS_CurvatureExponent);
				pShaderAPI->SetPixelShaderConstant(PBR_PS_FLOAT_SSSCONTROLS1, cSSSControls1);

				/*
				float4 cSSSControls2 = 0.0f;
				cSSSControls2.x = GetFloat(SSSPowerScale);
				pShaderAPI->SetPixelShaderConstant(PBR_PS_FLOAT_SSSCONTROLS2, cSSSControls2);
				*/
			}

			// c6
			float4 cNormalMapControls;
			cNormalMapControls.x = GetBool(NormalMap_FlipR) ? -1.0f : 1.0f;
			cNormalMapControls.y = GetBool(NormalMap_FlipG) ? -1.0f : 1.0f;
			cNormalMapControls.z = GetBool(NormalMap_FlipB) ? -1.0f : 1.0f;
			cNormalMapControls.w = fxsaturate(GetFloat(NormalMapFactor));
			pShaderAPI->SetPixelShaderConstant(PBR_PS_FLOAT_NORMALMAPCONTROLS, cNormalMapControls);

			// c7
			float4 cMRAOScale = 0.0f;
			cMRAOScale.xyz = GetFloat3(MRAOMultiplier);
			pShaderAPI->SetPixelShaderConstant(PBR_PS_FLOAT_MRAO_SCALE, cMRAOScale);

			// c8
			float4 cMRAOBias = 0.0f;
			cMRAOBias.xyz = GetFloat3(MRAOBias);
			pShaderAPI->SetPixelShaderConstant(PBR_PS_FLOAT_MRAO_BIAS, cMRAOBias);

			// c9
			float4 cMRAOExponent = 0.0f;
			cMRAOExponent.xyz = GetFloat3(MRAOExponent);
			pShaderAPI->SetPixelShaderConstant(PBR_PS_FLOAT_MRAO_EXPONENT, cMRAOExponent);

			// c11
			pShaderAPI->SetScreenSizeForVPOS(LUX_PS_FLOAT_ASW_SCREENSIZE);

			// c12
			float4 cSSAOControls = 1.0f;

			// Some duplicate Code here, FlashlightState has an Ambient Occlusion Factor, so we have to get it
			if (bProjTex)
			{
				ITexture* pFlashlightDepthTexture;
				FlashlightState_t FlashlightState;
				VMatrix xmWorldToTexture;
				FlashlightState = pShaderAPI->GetFlashlightStateEx(xmWorldToTexture, &pFlashlightDepthTexture);
				cSSAOControls.x *= FlashlightState.m_flAmbientOcclusion;
			}

			cSSAOControls.x *= GetFloat(AmbientOcclusion);
			cSSAOControls.x = fxsaturate(cSSAOControls.x); // Make sure this doesn't go out of Range
			pShaderAPI->SetPixelShaderConstant(LUX_PS_FLOAT_ASW_SSAOCONTROLS, cSSAOControls);

			// c26
			SetPixelShaderCameraPosition(LUX_PS_FLOAT_CAMERAPOSITION);

			// c27
			pShaderAPI->SetPixelShaderFogParams(LUX_PS_FLOAT_FOGPARAMETERS);

			// c28 - Modulation Constant
			bool bIsBrush = false;
			bool bApplySSBumpMathFix = false;
			float4 f4ModulationConstant = GetModulationConstant(bIsBrush, bApplySSBumpMathFix);
			pShaderAPI->SetPixelShaderConstant(LUX_PS_FLOAT_MODULATIONCONSTANTS, f4ModulationConstant);

			// c31
			// $Alpha2 not actually needed here, the Function just expects a Parameter for the .w of the Constant
			float4 f4BaseTextureTint = ComputeTint(!GetBool(NoTint) && GetBool(AllowDiffuseModulation), Alpha2);
			pShaderAPI->SetPixelShaderConstant(LUX_PS_FLOAT_DEFAULTCONTROLS, f4BaseTextureTint);

			BOOL BBools[REGISTER_BOOL_MAX] = { false };

			// b13, b14, b15
			BBools[LUX_PS_BOOL_HEIGHTFOG] = WriteWaterFogToDestAlpha(pContextData->m_bIsFullyOpaque);
			BBools[LUX_PS_BOOL_RADIALFOG] = HasRadialFog();
			BBools[LUX_PS_BOOL_DEPTHTODESTALPHA] = WriteDepthToDestAlpha(pContextData->m_bIsFullyOpaque);

			// Always set Boolean registers
			pShaderAPI->SetBooleanPixelShaderConstant(REGISTER_BOOL_START, BBools, REGISTER_BOOL_MAX);

			//==========================================================================//
			// Set Dynamic Shaders
			//==========================================================================//

			// Setting up dynamic vertex shader
			DECLARE_DYNAMIC_VERTEX_SHADER(pbr_vs30);
			SET_DYNAMIC_VERTEX_SHADER_COMBO(SKINNING, HasSkinning());
			SET_DYNAMIC_VERTEX_SHADER_COMBO(COMPRESSION, HasVertexCompression());
			SET_DYNAMIC_VERTEX_SHADER(pbr_vs30);

			// Setting up dynamic pixel shader
			// FIXME: Optimize Dynamic Combos. This is long compiletimes for no Reason
			if (bProjTex)
			{
				if (bSpecularGlossiness)
				{
					DECLARE_DYNAMIC_PIXEL_SHADER(pbr_sg_projtex_ps30);
					SET_DYNAMIC_PIXEL_SHADER_COMBO(PROJTEXSHADOWS, bProjTexShadows);
					SET_DYNAMIC_PIXEL_SHADER_COMBO(UBERLIGHT, bUberlight);
					SET_DYNAMIC_PIXEL_SHADER(pbr_sg_projtex_ps30);
				}
				else
				{
					DECLARE_DYNAMIC_PIXEL_SHADER(pbr_mrao_projtex_ps30);
					SET_DYNAMIC_PIXEL_SHADER_COMBO(PROJTEXSHADOWS, bProjTexShadows);
					SET_DYNAMIC_PIXEL_SHADER_COMBO(UBERLIGHT, bUberlight);
					SET_DYNAMIC_PIXEL_SHADER(pbr_mrao_projtex_ps30);
				}
			}
			else
			{
				LightState_t lightState;
				pShaderAPI->GetDX9LightState(&lightState);

				if (bSpecularGlossiness)
				{
					DECLARE_DYNAMIC_PIXEL_SHADER(pbr_sg_ps30);
					SET_DYNAMIC_PIXEL_SHADER_COMBO(NUM_LIGHTS, lightState.m_nNumLights);
					SET_DYNAMIC_PIXEL_SHADER(pbr_sg_ps30);
				}
				else
				{
					DECLARE_DYNAMIC_PIXEL_SHADER(pbr_mrao_ps30);
					SET_DYNAMIC_PIXEL_SHADER_COMBO(NUM_LIGHTS, lightState.m_nNumLights);
					SET_DYNAMIC_PIXEL_SHADER(pbr_mrao_ps30);
				}
			}
		}

	   Draw();

	   // TODO: DepthToDestAlpha for Alphatested Materials?
	}

	void PBR_Draw_Normal(IShaderShadow* pShaderShadow, IShaderDynamicAPI* pShaderAPI, CBasePerMaterialContextData** pContextDataPtr)
	{
		bool bHasNormalMap = IsTextureLoaded(NormalMap);
		bool bWrinkleMapping = IsTextureLoaded(Compress);
		bool bHasParallax = GetBool(Parallax);

		//==========================================================================//
		// Static Snapshot of the Shader Settings
		//==========================================================================//
		if (IsSnapshotting())
		{
			//==========================================================================//
			// General Rendering Setup
			//==========================================================================//

			// This handles : $IgnoreZ, $Decal, $Nocull, $Znearer, $Wireframe, $AllowAlphaToCoverage
			SetInitialShadowState();

			// We always need this
			pShaderShadow->EnableAlphaWrites(true);

			// Weird name, what it actually means : We output linear values
			pShaderShadow->EnableSRGBWrite(true);

			// Don't want Fog
			DisableFog();

			//==========================================================================//
			// Vertex Shader - Vertex Format
			//==========================================================================//

			// Compressed Verts get Normal + Tangent through vNormal ( NORMAL Stream )
			unsigned int nFlags = VERTEX_POSITION | VERTEX_NORMAL | VERTEX_FORMAT_COMPRESSED;

			// Always just one..
			int nTexCoords = 1;

			// Uncompressed Verts get Tangent + Binormal Sign through vUserData ( TANGENT Stream )
			int nUserDataSize = 4;

			pShaderShadow->VertexShaderVertexFormat(nFlags, nTexCoords, NULL, nUserDataSize);

			//==========================================================================//
			// Sampler Setup
			//==========================================================================//

			EnableSampler(SAMPLER_NORMAL, false);

			// s3, s4, s5, s6
			if (bWrinkleMapping)
			{
				EnableSampler(SAMPLER_BUMPCOMPRESS, false);
				EnableSampler(SAMPLER_BUMPSTRETCH, false);
			}

			//==========================================================================//
			// Set Static Shaders
			//==========================================================================//

			// Setting up static vertex shader
			DECLARE_STATIC_VERTEX_SHADER(pbr_vs30);
			SET_STATIC_VERTEX_SHADER_COMBO(PROJTEX, true); // Don't want to compute Lighting
			SET_STATIC_VERTEX_SHADER_COMBO(WORLD_NORMAL, true); // Want the Normal
			SET_STATIC_VERTEX_SHADER_COMBO(WRINKLEMAPS, bWrinkleMapping);
			SET_STATIC_VERTEX_SHADER(pbr_vs30);

			DECLARE_STATIC_PIXEL_SHADER(pbr_worldnormal_ps30);
			SET_STATIC_PIXEL_SHADER_COMBO(PARALLAXOCCLUSION, bHasParallax);
			SET_STATIC_PIXEL_SHADER_COMBO(WRINKLEMAPS, bWrinkleMapping);
			SET_STATIC_PIXEL_SHADER(pbr_worldnormal_ps30);
		}

		//==========================================================================//
		// Entirely Dynamic Commands
		//==========================================================================//
		if (pShaderAPI)
		{
			//==========================================================================//
			// Bind Textures
			//==========================================================================//

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

			if (bWrinkleMapping)
			{
				BindTexture(SAMPLER_BUMPCOMPRESS, BumpCompress, BumpFrame);
				BindTexture(SAMPLER_BUMPSTRETCH, BumpStretch, BumpFrame);
			}

			//==========================================================================//
			// Setup Constant Registers
			//==========================================================================//

			// VS c223, c224 - $BaseTextureTransform
			SetVertexShaderTextureTransform(LUX_VS_TEXTURETRANSFORM_01, BaseTextureTransform);

			// c1
			if (bHasParallax)
			{
				float4 cControls2;
				cControls2.x = 0.0f;
				cControls2.y = 0.0f;
				cControls2.z = GetFloat(ParallaxDepth);
				cControls2.w = GetFloat(ParallaxCenter);
				pShaderAPI->SetPixelShaderConstant(PBR_PS_FLOAT_CONTROLS2, cControls2);
			}

			// c6
			float4 cNormalMapControls;
			cNormalMapControls.x = GetBool(NormalMap_FlipR) ? -1.0f : 1.0f;
			cNormalMapControls.y = GetBool(NormalMap_FlipG) ? -1.0f : 1.0f;
			cNormalMapControls.z = GetBool(NormalMap_FlipB) ? -1.0f : 1.0f;
			cNormalMapControls.w = fxsaturate(GetFloat(NormalMapFactor));
			pShaderAPI->SetPixelShaderConstant(PBR_PS_FLOAT_NORMALMAPCONTROLS, cNormalMapControls);

			float4 cViewDir = 0.0f;
			pShaderAPI->GetWorldSpaceCameraDirection(cViewDir);

			float flFarZ = pShaderAPI->GetFarZ();
			cViewDir.xyz /= flFarZ;
			pShaderAPI->SetVertexShaderConstant(LUX_VS_FLOAT_SET0_0, cViewDir);

			//==========================================================================//
			// Set Dynamic Shaders
			//==========================================================================//

			// Setting up dynamic vertex shader
			DECLARE_DYNAMIC_VERTEX_SHADER(pbr_vs30);
			SET_DYNAMIC_VERTEX_SHADER_COMBO(SKINNING, HasSkinning());
			SET_DYNAMIC_VERTEX_SHADER_COMBO(COMPRESSION, HasVertexCompression());
			SET_DYNAMIC_VERTEX_SHADER(pbr_vs30);

			DECLARE_DYNAMIC_PIXEL_SHADER(pbr_worldnormal_ps30);
			SET_DYNAMIC_PIXEL_SHADER(pbr_worldnormal_ps30);
		}

		Draw();
	}

	SHADER_DRAW
	{
		if(ShouldDrawNormalsForSSAO())
		{
			// Non-Opaque Materials should not write a SSAO Factor
			if(HasFlag(MATERIAL_VAR_TRANSLUCENT) || HasFlag(MATERIAL_VAR_ADDITIVE) || HasFlag(MATERIAL_VAR_ALPHATEST))
			{
				Draw(false);
				return;
			}

			PBR_Draw_Normal(pShaderShadow, pShaderAPI, pContextDataPtr);
		}
		else
		{
			PBR_Draw_Internal(pShaderShadow, pShaderAPI, pContextDataPtr);
		}
	}
END_SHADER