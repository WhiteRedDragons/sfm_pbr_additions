//==========================================================================//
//
//	Purpose of this File : Turns Magic Numbers into Macros
//
//==========================================================================//

#ifndef PBR_REGISTERMAP_H_
#define PBR_REGISTERMAP_H_

// Fullbright, Microshadow Factor
#define PBR_PS_FLOAT_CONTROLS1		REGISTER_FLOAT_001

// Dual Lobe Roughness Bias, Lerp Factor, Parallax Depth, Parallax Center
#define PBR_PS_FLOAT_CONTROLS2		REGISTER_FLOAT_002

// Color, Intensity, Power
#define PBR_PS_FLOAT_SSSCONTROLS1	REGISTER_FLOAT_004
#define PBR_PS_FLOAT_SSSCONTROLS2	REGISTER_FLOAT_005

// Normal Map Flips, Normal Map Factor
#define PBR_PS_FLOAT_NORMALMAPCONTROLS		REGISTER_FLOAT_006

// .w's Free
#define PBR_PS_FLOAT_MRAO_SCALE				REGISTER_FLOAT_007
#define PBR_PS_FLOAT_MRAO_BIAS				REGISTER_FLOAT_008
#define PBR_PS_FLOAT_MRAO_EXPONENT			REGISTER_FLOAT_009

// c10 to c31 used by LUX Constants ( including SSAO Controls ), Lighting and Ambient Cubes

#endif // PBR_REGISTERMAP_H_