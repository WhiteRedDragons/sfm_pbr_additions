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
#include "pbr_ps30.inc"

// FIXME: Sampler Layout
// Defining samplers
const Sampler_t SAMPLER_BASETEXTURE = SHADER_SAMPLER0;
const Sampler_t SAMPLER_NORMAL = SHADER_SAMPLER1;
const Sampler_t SAMPLER_ENVMAP = SHADER_SAMPLER2;
const Sampler_t SAMPLER_LIGHTWARP = SHADER_SAMPLER3;
const Sampler_t SAMPLER_THICKNESS = SHADER_SAMPLER3;

const Sampler_t SAMPLER_SHADOWDEPTH = SHADER_SAMPLER4;
const Sampler_t SAMPLER_RANDOMROTATION = SHADER_SAMPLER5;
const Sampler_t SAMPLER_FLASHLIGHT = SHADER_SAMPLER6;

const Sampler_t SAMPLER_LIGHTMAP = SHADER_SAMPLER7;

const Sampler_t SAMPLER_COMPRESS = SHADER_SAMPLER8;
const Sampler_t SAMPLER_STRETCH = SHADER_SAMPLER9;
const Sampler_t SAMPLER_MRAO = SHADER_SAMPLER10;

const Sampler_t SAMPLER_EMISSIVE = SHADER_SAMPLER11;

const Sampler_t SAMPLER_SPECULAR = SHADER_SAMPLER12;

const Sampler_t SAMPLER_SSAO = SHADER_SAMPLER13;

const Sampler_t SAMPLER_BUMPCOMPRESS = SHADER_SAMPLER14;
const Sampler_t SAMPLER_BUMPSTRETCH = SHADER_SAMPLER12;

// Convars
static ConVar mat_fullbright("mat_fullbright", "0", FCVAR_CHEAT);
static ConVar mat_specular("mat_specular", "1", FCVAR_NONE);
static ConVar mat_pbr_parallaxmap("mat_pbr_parallaxmap", "1");

// Beginning the shader
BEGIN_VS_SHADER(PBR, "PBR shader")

    // Setting up vmt parameters
    // FIXME: Capslocked Parameter Names are hard to read and this is only a thing because everyone copies the Code from Valve
    BEGIN_SHADER_PARAMS;
        SHADER_PARAM(AlphaTestReference, SHADER_PARAM_TYPE_FLOAT, "0", "");
        SHADER_PARAM(EnvMap, SHADER_PARAM_TYPE_ENVMAP, "", "Set the cubemap for this material.");
        SHADER_PARAM(MRAOTexture, SHADER_PARAM_TYPE_TEXTURE, "", "Texture with metalness in R, roughness in G, ambient occlusion in B.");
        SHADER_PARAM(EmissionTexture, SHADER_PARAM_TYPE_TEXTURE, "", "Emission texture");
        SHADER_PARAM(NormalTexture, SHADER_PARAM_TYPE_TEXTURE, "", "Normal texture (deprecated, use $bumpmap)");
        SHADER_PARAM(BumpMap, SHADER_PARAM_TYPE_TEXTURE, "", "Normal texture");
        SHADER_PARAM(BumpFrame, SHADER_PARAM_TYPE_INTEGER, "0", "Frame number for $bumpmap")
        SHADER_PARAM(UseEnvAmbient, SHADER_PARAM_TYPE_BOOL, "0", "Use the cubemaps to compute ambient light.");
        SHADER_PARAM(SpecularTexture, SHADER_PARAM_TYPE_TEXTURE, "", "Specular F0 RGB map");
        SHADER_PARAM(LightWarpTexture, SHADER_PARAM_TYPE_TEXTURE, "", "Lightwarp Texture" );
        SHADER_PARAM(ThicknessTexture, SHADER_PARAM_TYPE_TEXTURE, "", "Thickness map for SSS" );
        SHADER_PARAM(Parallax, SHADER_PARAM_TYPE_BOOL, "0", "Use Parallax Occlusion Mapping.");
        SHADER_PARAM(ParallaxDepth, SHADER_PARAM_TYPE_FLOAT, "0.0030", "Depth of the Parallax Map");
        SHADER_PARAM(ParallaxCenter, SHADER_PARAM_TYPE_FLOAT, "0.5", "Center depth of the Parallax Map");
        SHADER_PARAM(MetalnessFactor, SHADER_PARAM_TYPE_FLOAT, "1.0", "Metalness factor");
        SHADER_PARAM(RoughnessFactor, SHADER_PARAM_TYPE_FLOAT, "1.0", "Roughness factor");
        SHADER_PARAM(EmissiveFactor, SHADER_PARAM_TYPE_FLOAT, "1.0", "Emissive factor" );
        SHADER_PARAM(SpecularFactor, SHADER_PARAM_TYPE_FLOAT, "1.0", "Specular factor" );
        SHADER_PARAM(AOFactor, SHADER_PARAM_TYPE_FLOAT, "1.0", "Ambient occlusion factor");
        SHADER_PARAM(SSAOFactor, SHADER_PARAM_TYPE_FLOAT, "1.0", "Screen space ambient occlusion factor");
        SHADER_PARAM(SSSCOLOR, SHADER_PARAM_TYPE_COLOR, "[1 1 1]", "Subsurface scattering color");
        SHADER_PARAM(SSSIntensity, SHADER_PARAM_TYPE_FLOAT, "1.0", "SSS intensity");
        SHADER_PARAM(SSSPowerScale, SHADER_PARAM_TYPE_FLOAT, "1.0", "SSS power scale");
        SHADER_PARAM(Compress, SHADER_PARAM_TYPE_TEXTURE, "", "Compression wrinklemap");
        SHADER_PARAM(BumpCompress, SHADER_PARAM_TYPE_TEXTURE, "", "Stretch bumpmap" );
        SHADER_PARAM(Stretch, SHADER_PARAM_TYPE_TEXTURE, "", "Stretch wrinklemap");
        SHADER_PARAM(BumpStretch, SHADER_PARAM_TYPE_TEXTURE, "", "Compression bumpmap" );
    END_SHADER_PARAMS;

    // Initializing parameters
    SHADER_INIT_PARAMS()
    {
        // Fallback for changed parameter
        // NUKE: $NormalTexture, replace with $NormalMap
        if (params[NormalTexture]->IsDefined())
            params[BumpMap]->SetStringValue(params[NormalTexture]->GetStringValue());

        // Dynamic lights need a bumpmap
        // FIXME: Use Standard Flat Normal Map in Dynamic State, set this to "..." for Models
        // Setting $BumpMap to something is required or WorldLight based Lighting doesn't work on Models correctly
        if (!params[BumpMap]->IsDefined())
            params[BumpMap]->SetStringValue("dev/flat_normal");

        // Set a good default mrao texture
        // FIXME: Use Standard White Texture with proper Bias Values
        if (!params[MRAOTexture]->IsDefined())
            params[MRAOTexture]->SetStringValue("dev/pbr_mraotexture");

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
        InitIntParam( BumpFrame, params, 0 );

        // FIXME: Bracket Spacing
        InitFloatParam( MetalnessFactor, params, 1.0f );
        InitFloatParam( RoughnessFactor, params, 1.0f );
        InitFloatParam( EmissiveFactor, params, 1.0f );
        InitFloatParam( SpecularFactor, params, 1.0f );
        InitFloatParam( AOFactor, params, 1.0f );
        InitFloatParam( SSAOFactor, params, 1.0f );
        InitFloatParam( SSSIntensity, params, 1.0f );
        InitFloatParam( SSSPowerScale, params, 1.0f );

        // NUKE: This is a Color Param, it's default Value is 1 1 1 if you set it here or not
        InitVecParam( SSSCOLOR, params, 1, 1, 1 );
    };

    // Define shader fallback
    SHADER_FALLBACK
    {
        return 0;
    };

    SHADER_INIT
    {
        // FIXME: Make dedicated $BaseColor and $NormalMap Parameters
        LoadTexture(BaseTexture, TEXTUREFLAGS_SRGB);
        LoadBumpMap(BumpMap);

        // FIXME2: Make dedicated $Diffuse Texture for "$SpecularTexture" which should be renamed to $Specular honestly
        LoadTexture(MRAOTexture);
        LoadTexture(SpecularTexture, TEXTUREFLAGS_SRGB);

        int envMapFlags = g_pHardwareConfig->GetHDRType() == HDR_TYPE_NONE ? TEXTUREFLAGS_SRGB : 0;
        envMapFlags |= TEXTUREFLAGS_ALL_MIPS;
        LoadCubeMap(EnvMap, envMapFlags);

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

        // Setting up booleans
        bool bHasBaseTexture = params[BaseTexture]->IsTexture();
        bool bHasNormalTexture = params[BumpMap]->IsTexture();
        bool bHasMraoTexture = params[MRAOTexture]->IsTexture();
        bool bHasSpecularTexture = !bHasMraoTexture && params[SpecularTexture]->IsTexture();
        bool bHasEnvMap = params[EnvMap]->IsTexture();
        bool bHasEmissionTexture = params[EmissionTexture]->IsTexture();

        // IsDefined() is not real on Shader Draw; This doesn't make any sense
        bool bHasColor = true; // params[Color1]->IsDefined();

        // FIXME: Non-sensical Variable Name, this is bIsModel. Did you know Models can also be lightmapped?
        // FIXME: just nuke Brush Support, who is going to use this on SFM
        bool bLightMapped = !IS_FLAG_SET(MATERIAL_VAR_MODEL);

        // NUKE: Non-sense or force. Why is there two of different ways of deriving Ambient
        // Physically based workarounds
        bool bUseEnvAmbient = params[UseEnvAmbient]->GetIntValue();

        bool bThicknessTexture = !bLightMapped && params[ThicknessTexture]->IsTexture();

        // Can't have lightwarp and SSS together
        // ShiroDkxtro2: Why? Modern LUT-based implementations of SSS use a "LightWarpTexture" for SSS
        bool bLightwarpTexture = !bThicknessTexture && params[LightWarpTexture]->IsTexture();

        // Only supported on models
        bool bWrinkleMapping = !bLightMapped && params[Compress]->IsDefined();

        // Determining whether we're dealing with a fully opaque material
        // FIXME: Transluceny on PBR is more than just simple Alphablending
        BlendType_t nBlendType = EvaluateBlendRequirements(BaseTexture, true);
        bool bFullyOpaque = (nBlendType != BT_BLENDADD) && (nBlendType != BT_BLEND) && !bIsAlphaTested;

        if (IsSnapshotting())
        {
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
                pShaderShadow->EnableBlending(true);
                pShaderShadow->BlendFunc(SHADER_BLEND_ONE, SHADER_BLEND_ONE); // Additive blending
            }
            else
            {
                SetDefaultBlendingShadowState(BaseTexture, true);
            }

            // FIXME: Why is this here and not down there
            int nShadowFilterMode = bHasFlashlight ? g_pHardwareConfig->GetShadowFilterMode() : 0;

            // FIXME: All of this
            // Setting up samplers
            pShaderShadow->EnableTexture(SAMPLER_BASETEXTURE, true);    // Basecolor texture
            pShaderShadow->EnableSRGBRead(SAMPLER_BASETEXTURE, true);   // Basecolor is sRGB

            // FIXME: This is enabled without ever checking if there is a Emissive Texture
            pShaderShadow->EnableTexture(SAMPLER_EMISSIVE, true);       // Emission texture
            pShaderShadow->EnableSRGBRead(SAMPLER_EMISSIVE, true);      // Emission is sRGB

            // FIXME: This is enabled despite only being used on Brushes
            pShaderShadow->EnableTexture(SAMPLER_LIGHTMAP, true);       // Lightmap texture
            pShaderShadow->EnableSRGBRead(SAMPLER_LIGHTMAP, false);     // Lightmaps aren't sRGB

            pShaderShadow->EnableTexture(SAMPLER_MRAO, true);           // MRAO texture
            pShaderShadow->EnableSRGBRead(SAMPLER_MRAO, false);         // MRAO isn't sRGB
            pShaderShadow->EnableTexture(SAMPLER_NORMAL, true);         // Normal texture
            pShaderShadow->EnableSRGBRead(SAMPLER_NORMAL, false);       // Normals aren't sRGB
            pShaderShadow->EnableTexture(SAMPLER_SPECULAR, true);       // Specular F0 texture
            pShaderShadow->EnableSRGBRead(SAMPLER_SPECULAR, true);      // Specular F0 is sRGB
            pShaderShadow->EnableTexture(SAMPLER_SSAO, true);           // SSAO texture
            pShaderShadow->EnableSRGBRead(SAMPLER_SSAO, true);         // SSAO is sRGB

            // If the flashlight is on, set up its textures
            if (bHasFlashlight)
            {
                pShaderShadow->EnableTexture(SAMPLER_SHADOWDEPTH, true);        // Shadow depth map
                pShaderShadow->SetShadowDepthFiltering(SAMPLER_SHADOWDEPTH);
                pShaderShadow->EnableSRGBRead(SAMPLER_SHADOWDEPTH, false);
                pShaderShadow->EnableTexture(SAMPLER_RANDOMROTATION, true);     // Noise map
                pShaderShadow->EnableTexture(SAMPLER_FLASHLIGHT, true);         // Flashlight cookie - ok why is it not called cookie then
                pShaderShadow->EnableSRGBRead(SAMPLER_FLASHLIGHT, true);
            }

            // Setting up envmap
            if (bHasEnvMap)
            {
                pShaderShadow->EnableTexture(SAMPLER_ENVMAP, true); // Envmap
                if (g_pHardwareConfig->GetHDRType() == HDR_TYPE_NONE)
                {
                    pShaderShadow->EnableSRGBRead(SAMPLER_ENVMAP, true); // Envmap is only sRGB with HDR disabled?
                }
            }

            // Thickness (SSS)
            if (bThicknessTexture)
            {
                pShaderShadow->EnableTexture(SAMPLER_THICKNESS, true); 
                pShaderShadow->EnableSRGBRead(SAMPLER_THICKNESS, false);
            }     
            // Lightwarp
            else if (bLightwarpTexture)
            {
                pShaderShadow->EnableTexture(SAMPLER_LIGHTWARP, true); 
                pShaderShadow->EnableSRGBRead(SAMPLER_LIGHTWARP, false);
            }

            // Wrinkle mapping
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

            // Enabling sRGB writing
            // See common_ps_fxc.h line 349
            // PS2b shaders and up write sRGB
            pShaderShadow->EnableSRGBWrite(true);

            // FIXME: This Vertex Format Setup doesn't follow Convention and is likely broken with Facial Flexes because of missing Uncompressed Vert Data
            if (IS_FLAG_SET(MATERIAL_VAR_MODEL))
            {
                // We only need the position and surface normal
                unsigned int flags = VERTEX_POSITION | VERTEX_NORMAL | VERTEX_FORMAT_COMPRESSED;
                // We need three texcoords, all in the default float2 size
                pShaderShadow->VertexShaderVertexFormat(flags, 1, 0, 0);
            }
            else
            {
                // We need the position, surface normal, and vertex compression format
                unsigned int flags = VERTEX_POSITION | VERTEX_NORMAL;
                // We only need one texcoord, in the default float2 size
                pShaderShadow->VertexShaderVertexFormat(flags, 3, 0, 0);
            }
        
            // FIXME: Move to Param Init why does it need to be here
            int useParallax = params[Parallax]->GetIntValue();
            // Parallax and wrinkle are incompatible
            if (!mat_pbr_parallaxmap.GetBool() || bWrinkleMapping)
            {
                useParallax = 0;
            }

            // FIXME: Always pass on TBN from the Vertex Shader
            // SSAO path
            bool bWorldNormal = ( ENABLE_FIXED_LIGHTING_OUTPUTNORMAL_AND_DEPTH ==
                              ( IS_FLAG2_SET( MATERIAL_VAR2_USE_GBUFFER0 ) + 2 * IS_FLAG2_SET( MATERIAL_VAR2_USE_GBUFFER1 ) ) );

            // FIXME: Split Brushes and Models into two separate Shaders
            // FIXME: Split Projected Texture into separate Shader

            // Setting up static vertex shader
            DECLARE_STATIC_VERTEX_SHADER(pbr_vs30);
            SET_STATIC_VERTEX_SHADER_COMBO(WORLD_NORMAL, bWorldNormal);
//          SET_STATIC_VERTEX_SHADER_COMBO(LIGHTMAPPED, bLightMapped);
            SET_STATIC_VERTEX_SHADER(pbr_vs30);

            // Setting up static pixel shader
            DECLARE_STATIC_PIXEL_SHADER(pbr_ps30);
            SET_STATIC_PIXEL_SHADER_COMBO(FLASHLIGHT, bHasFlashlight);
            SET_STATIC_PIXEL_SHADER_COMBO(FLASHLIGHTDEPTHFILTERMODE, nShadowFilterMode); // TODO: Check if the ATI Shadow Format Issue was fixed on SFM
//            SET_STATIC_PIXEL_SHADER_COMBO(LIGHTMAPPED, 0); // bLightMapped
            SET_STATIC_PIXEL_SHADER_COMBO(USEENVAMBIENT, bUseEnvAmbient);
            SET_STATIC_PIXEL_SHADER_COMBO(EMISSIVE, bHasEmissionTexture); // FIXME: Make additively rendered pass to save on Samplers
//            SET_STATIC_PIXEL_SHADER_COMBO(SPECULAR, 0); // bHasSpecularTexture
            SET_STATIC_PIXEL_SHADER_COMBO(PARALLAXOCCLUSION, useParallax);
            SET_STATIC_PIXEL_SHADER_COMBO(WORLD_NORMAL, bWorldNormal);
//            SET_STATIC_PIXEL_SHADER_COMBO(LightWarpTexture, 0); // bLightwarpTexture
            SET_STATIC_PIXEL_SHADER_COMBO(WRINKLEMAP, bWrinkleMapping);
            SET_STATIC_PIXEL_SHADER_COMBO(SUBSURFACESCATTERING, bThicknessTexture);
            SET_STATIC_PIXEL_SHADER(pbr_ps30);

            // FIXME: Move up to the rest of snapshot setup ( LUX style )
            // Setting up fog
            if (bHasFlashlight)
                FogToBlack();
            else
                DefaultFog(); // I think this is correct

            // HACK HACK HACK - enable alpha writes all the time so that we have them for underwater stuff
            pShaderShadow->EnableAlphaWrites(bFullyOpaque);

            // FIXME: All of the below except Color, not on Brushes
            float flLScale = pShaderShadow->GetLightMapScaleFactor();

            PI_BeginCommandBuffer();

            // Send ambient cube to the pixel sh
            PI_SetPixelShaderAmbientLightCube( PSREG_AMBIENT_CUBE );

            // Send lighting array to the pixel shader
            PI_SetPixelShaderLocalLighting( PSREG_LIGHT_INFO_ARRAY );

            // Set up shader modulation color
            PI_SetModulationPixelShaderDynamicState_LinearScale_ScaleInW( PSREG_DIFFUSE_MODULATION, flLScale );

            PI_EndCommandBuffer();
        }
        // FIXME: Make separate if statement and add some spacing
        else // Not snapshotting -- begin dynamic state
        {
            bool bLightingOnly = mat_fullbright.GetInt() == 2 && !IS_FLAG_SET(MATERIAL_VAR_NO_DEBUG_OVERRIDE);

            // Setting up albedo texture
            if (bHasBaseTexture)
            {
                BindTexture(SAMPLER_BASETEXTURE, BaseTexture, Frame);
            }
            else
            {
                pShaderAPI->BindStandardTexture(SAMPLER_BASETEXTURE, TEXTURE_GREY);
            }

            // Setting up vmt color
            // FIXME: Standardise
            Vector4D color( 0, 0, 0, 0 );
            if (bHasColor)
            {
                params[Color1]->GetVecValue(color.Base(), 3);
            }
            else
            {
                color.Init( 1, 1, 1 );
            }
            pShaderAPI->SetPixelShaderConstant(PSREG_SELFILLUMTINT, color.Base());

            // Setting up environment map
            if (bHasEnvMap)
            {  
                // FIXME: EnvMapFrame
                BindTexture(SAMPLER_ENVMAP, EnvMap, 0); // FIXME: Missing Frame Parameter
            }
            else
            {
                // This is also the mat_specular 0 Case that for some Reason is handled way below
                pShaderAPI->BindStandardTexture(SAMPLER_ENVMAP, TEXTURE_BLACK);
            }

            // Setting up emissive texture
            if (bHasEmissionTexture)
            {
                BindTexture(SAMPLER_EMISSIVE, EmissionTexture, 0); // FIXME: Missing Frame Parameter
            }
            else
            {
                // NOTE: The Sampler is always enabled, either this is some magical SFM Requirement or this is non-sense
                pShaderAPI->BindStandardTexture(SAMPLER_EMISSIVE, TEXTURE_BLACK);
            }

            // Setting up normal map
            // NOTE: A default Normal Map is defined in Param Init, but there is still a Fallback here
            if (bHasNormalTexture)
            {
                BindTexture(SAMPLER_NORMAL, BumpMap, BumpFrame);
            }
            else
            {
                pShaderAPI->BindStandardTexture(SAMPLER_NORMAL, TEXTURE_NORMALMAP_FLAT);
            }

            // Setting up mrao map
            if (bHasMraoTexture)
            {
                BindTexture(SAMPLER_MRAO, MRAOTexture, 0); // FIXME: Missing Frame Parameter
            }
            else
            {
                pShaderAPI->BindStandardTexture(SAMPLER_MRAO, TEXTURE_WHITE);
            }

            if (bHasSpecularTexture)
            {
                BindTexture(SAMPLER_SPECULAR, SpecularTexture, 0); // FIXME: Missing Frame Parameter
            }
            else
            {
                pShaderAPI->BindStandardTexture(SAMPLER_SPECULAR, TEXTURE_BLACK);
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

            // Getting the light state
            // FIXME: Model only State
            LightState_t lightState;
            pShaderAPI->GetDX9LightState(&lightState);

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
                ITexture *pFlashlightDepthTexture;
                flashlightState = pShaderAPI->GetFlashlightStateEx(flashlightWorldToTexture, &pFlashlightDepthTexture);
                bFlashlightShadows = flashlightState.m_bEnableShadows && (pFlashlightDepthTexture != NULL);

                SetFlashLightColorFromState(flashlightState, pShaderAPI, false, PSREG_FLASHLIGHT_COLOR);

                if (pFlashlightDepthTexture && g_pConfig->ShadowDepthTexture() && flashlightState.m_bEnableShadows)
                {
                    BindTexture(SAMPLER_SHADOWDEPTH, pFlashlightDepthTexture, 0);
                    pShaderAPI->BindStandardTexture(SAMPLER_RANDOMROTATION, TEXTURE_SHADOW_NOISE_2D);
                }
            }

            // FIXME: All of this
            // ---
            // Getting fog info
            MaterialFogMode_t fogType = pShaderAPI->GetSceneFogMode();
            int fogIndex = (fogType == MATERIAL_FOG_LINEAR_BELOW_FOG_Z) ? 1 : 0;

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

            float vEyePos_SpecExponent[4];
            pShaderAPI->GetWorldSpaceCameraPosition(vEyePos_SpecExponent);

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
            pShaderAPI->SetPixelShaderConstant(PSREG_EYEPOS_SPEC_EXPONENT, vEyePos_SpecExponent, 1);

            // Setting lightmap texture
            if (bLightMapped)
                s_pShaderAPI->BindStandardTexture(SAMPLER_LIGHTMAP, TEXTURE_LIGHTMAP);

            // Setting up dynamic vertex shader
            DECLARE_DYNAMIC_VERTEX_SHADER(pbr_vs30);
            SET_DYNAMIC_VERTEX_SHADER_COMBO(DOWATERFOG, fogIndex);
            SET_DYNAMIC_VERTEX_SHADER_COMBO(SKINNING, numBones > 0);
            SET_DYNAMIC_VERTEX_SHADER_COMBO(COMPRESSED_VERTS, (int)vertexCompression);
            SET_DYNAMIC_VERTEX_SHADER_COMBO(NUM_LIGHTS, lightState.m_nNumLights);
            SET_DYNAMIC_VERTEX_SHADER(pbr_vs30);

            // Setting up dynamic pixel shader
            // FIXME: Optimize Dynamic Combos. This is long compiletimes for no Reason
            DECLARE_DYNAMIC_PIXEL_SHADER(pbr_ps30);
            SET_DYNAMIC_PIXEL_SHADER_COMBO(NUM_LIGHTS, lightState.m_nNumLights);
            SET_DYNAMIC_PIXEL_SHADER_COMBO(WRITEWATERFOGTODESTALPHA, bWriteWaterFogToAlpha);
            SET_DYNAMIC_PIXEL_SHADER_COMBO(WRITE_DEPTH_TO_DESTALPHA, bWriteDepthToAlpha);
            SET_DYNAMIC_PIXEL_SHADER_COMBO(PIXELFOGTYPE, pShaderAPI->GetPixelFogCombo());
            SET_DYNAMIC_PIXEL_SHADER_COMBO(FLASHLIGHTSHADOWS, bFlashlightShadows);
            SET_DYNAMIC_PIXEL_SHADER_COMBO(UBERLIGHT, flashlightState.m_bUberlight);
            SET_DYNAMIC_PIXEL_SHADER(pbr_ps30);

            // Setting up base texture transform
            // FIXME: Use a Macro Map for this because I don't trust these random Enums that love to vary across branches
            SetVertexShaderTextureTransform(VERTEX_SHADER_SHADER_SPECIFIC_CONST_0, BaseTextureTransform);

            // FIXME: This is non-sense because we derive metal color from the BaseColor
            // FIXME: Duplicate Texture Bind, happens above for regular BaseTexture
            // Handle mat_fullbright 2 (diffuse lighting only)
            if (bLightingOnly)
            {
                pShaderAPI->BindStandardTexture(SAMPLER_BASETEXTURE, TEXTURE_GREY); // Basecolor
            }

            // FIXME: Duplicate Texture Bind, move this to where $EnvMap is handled
            // Handle mat_specular 0 (no envmap reflections)
            if (!mat_specular.GetBool())
            {
                pShaderAPI->BindStandardTexture(SAMPLER_ENVMAP, TEXTURE_BLACK); // Envmap
            }

            // Sending fog info to the pixel shader
            pShaderAPI->SetPixelShaderFogParams(PSREG_FOG_PARAMS);

            // Ambient occlusion
            ITexture* pAOTexture = pShaderAPI->GetTextureRenderingParameter( TEXTURE_RENDERPARM_AMBIENT_OCCLUSION );

            if (pAOTexture)
                BindTexture( SAMPLER_SSAO, pAOTexture );
            else
                pShaderAPI->BindStandardTexture( SAMPLER_SSAO, TEXTURE_WHITE );

            // Metalness, roughtness, ambient occlusion, SSAO Factors
            float flSSAOStrength = 1.0f;
            if (bHasFlashlight)
                flSSAOStrength *= flashlightState.m_flAmbientOcclusion;

            // FIXME: Use Bias Values because multiplication does not f'n work with 0.0 and makes anything hard to pinpoint to an exact Value
            // FIXME: Single Parameter not 4
            float vMRAOFactors[4] =
            {
                GetFloatParam( MetalnessFactor, params, 1.0f ),
                GetFloatParam( RoughnessFactor, params, 1.0f ),
                GetFloatParam( AOFactor, params, 1.0f ),
                GetFloatParam( SSAOFactor, params, 1.0f ) * flSSAOStrength
            };
            pShaderAPI->SetPixelShaderConstant(PSREG_PBR_MRAO_FACTORS, vMRAOFactors, 1);

            // Emissive, specular factors, SSS intensity and power scale 
            float vExtraFactors[4] =
            {
                GetFloatParam( EmissiveFactor, params, 1.0f ),
                GetFloatParam( SpecularFactor, params, 1.0f ), // Wat? What is the point of $MetalnessFactor if we end up having two Parameters to handle the same Thing
                GetFloatParam( SSSIntensity, params, 1.0f ),
                GetFloatParam( SSSPowerScale, params, 1.0f )
            };
            pShaderAPI->SetPixelShaderConstant(PSREG_PBR_EXTRA_FACTORS, vExtraFactors, 1);

            float vSSSColor[4] = { 0, 0, 0, 0 };
            params[SSSCOLOR]->GetVecValue( vSSSColor, 3 );
            pShaderAPI->SetPixelShaderConstant( PSREG_PBR_SSS_COLOR, vSSSColor, 1 );

            // Need this for sampling SSAO
            pShaderAPI->SetScreenSizeForVPOS();

            // Pass FarZ for SSAO
            int nLightingPreviewMode = pShaderAPI->GetIntRenderingParameter( INT_RENDERPARM_ENABLE_FIXED_LIGHTING );
            if ( nLightingPreviewMode == ENABLE_FIXED_LIGHTING_OUTPUTNORMAL_AND_DEPTH )
            {
                float vEyeDir[4];
                pShaderAPI->GetWorldSpaceCameraDirection( vEyeDir );

                float flFarZ = pShaderAPI->GetFarZ();
                vEyeDir[0] /= flFarZ;	// Divide by farZ for SSAO algorithm
                vEyeDir[1] /= flFarZ;
                vEyeDir[2] /= flFarZ;
                pShaderAPI->SetVertexShaderConstant( VERTEX_SHADER_SHADER_SPECIFIC_CONST_8, vEyeDir );
            }

            // More flashlight related stuff
            // FIXME: Same as above, standardise this
            if (bHasFlashlight)
            {
                float atten[4], pos[4], tweaks[4];
                SetFlashLightColorFromState(flashlightState, pShaderAPI, false, PSREG_FLASHLIGHT_COLOR);

                BindTexture(SAMPLER_FLASHLIGHT, flashlightState.m_pSpotlightTexture, flashlightState.m_nSpotlightTextureFrame);

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

            // HAHAHHAHA The same slop-code as the original Shader
            // FIXME: Unbracketed Code

            float flParams[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
            // Parallax Depth (the strength of the effect)
            flParams[0] = GetFloatParam(ParallaxDepth, params, 3.0f);
            // Parallax Center (the height at which it's not moved)
            flParams[1] = GetFloatParam(ParallaxCenter, params, 3.0f);
            pShaderAPI->SetPixelShaderConstant(PSREG_SHADER_CONTROLS, flParams, 1);

        }

        // Actually draw the shader
       Draw();

       // TODO: DepthToDestAlpha for Alphatested Materials?
    };

// Closing it off
END_SHADER;