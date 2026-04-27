// ALL SKIP STATEMENTS THAT AFFECT THIS SHADER!!!
// ($PIXELFOGTYPE == 0) && ($WRITEWATERFOGTODESTALPHA != 0)
// ( $WORLD_NORMAL == 1 ) && ( $NUM_LIGHTS != 0 ) && ( $WRITEWATERFOGTODESTALPHA == 1 )
// ( $WRINKLEMAP != 0 && $PARALLAXOCCLUSION != 0 )
// ( $WRINKLEMAP != 0 && $LIGHTMAPPED != 0 )
// ( $SUBSURFACESCATTERING != 0 ) && ( $LIGHTWARPTEXTURE != 0 )
// ( $SUBSURFACESCATTERING != 0 && $PARALLAXOCCLUSION != 0 )
// defined $PIXELFOGTYPE && defined $WRITEWATERFOGTODESTALPHA && ( $PIXELFOGTYPE != 1 ) && $WRITEWATERFOGTODESTALPHA
// defined $LIGHTING_PREVIEW && defined $FASTPATHENVMAPTINT && $LIGHTING_PREVIEW && $FASTPATHENVMAPTINT
// defined $LIGHTING_PREVIEW && defined $FASTPATHENVMAPCONTRAST && $LIGHTING_PREVIEW && $FASTPATHENVMAPCONTRAST
// defined $LIGHTING_PREVIEW && defined $FASTPATH && $LIGHTING_PREVIEW && $FASTPATH
// ($FLASHLIGHT || $FLASHLIGHTSHADOWS) && $LIGHTING_PREVIEW
// defined $PIXELFOGTYPE && defined $WRITEWATERFOGTODESTALPHA && ( $PIXELFOGTYPE != 1 ) && $WRITEWATERFOGTODESTALPHA
// defined $LIGHTING_PREVIEW && defined $FASTPATHENVMAPTINT && $LIGHTING_PREVIEW && $FASTPATHENVMAPTINT
// defined $LIGHTING_PREVIEW && defined $FASTPATHENVMAPCONTRAST && $LIGHTING_PREVIEW && $FASTPATHENVMAPCONTRAST
// defined $LIGHTING_PREVIEW && defined $FASTPATH && $LIGHTING_PREVIEW && $FASTPATH
// ($FLASHLIGHT || $FLASHLIGHTSHADOWS) && $LIGHTING_PREVIEW
// defined $PIXELFOGTYPE && defined $WRITEWATERFOGTODESTALPHA && ( $PIXELFOGTYPE != 1 ) && $WRITEWATERFOGTODESTALPHA
// defined $LIGHTING_PREVIEW && defined $FASTPATHENVMAPTINT && $LIGHTING_PREVIEW && $FASTPATHENVMAPTINT
// defined $LIGHTING_PREVIEW && defined $FASTPATHENVMAPCONTRAST && $LIGHTING_PREVIEW && $FASTPATHENVMAPCONTRAST
// defined $LIGHTING_PREVIEW && defined $FASTPATH && $LIGHTING_PREVIEW && $FASTPATH
// ($FLASHLIGHT || $FLASHLIGHTSHADOWS) && $LIGHTING_PREVIEW

#pragma once
#include "shaderlib/cshader.h"
class pbr_mrao_ps30_Static_Index
{
	unsigned int m_nEMISSIVE : 2;
	unsigned int m_nPARALLAXOCCLUSION : 2;
	unsigned int m_nWORLD_NORMAL : 2;
	unsigned int m_nWRINKLEMAP : 2;
	unsigned int m_nSUBSURFACESCATTERING : 2;
	unsigned int m_nDUALLOBE : 2;
#ifdef _DEBUG
	bool m_bEMISSIVE : 1;
	bool m_bPARALLAXOCCLUSION : 1;
	bool m_bWORLD_NORMAL : 1;
	bool m_bWRINKLEMAP : 1;
	bool m_bSUBSURFACESCATTERING : 1;
	bool m_bDUALLOBE : 1;
#endif	// _DEBUG
public:
	void SetEMISSIVE( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nEMISSIVE = i;
#ifdef _DEBUG
		m_bEMISSIVE = true;
#endif	// _DEBUG
	}

	void SetPARALLAXOCCLUSION( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nPARALLAXOCCLUSION = i;
#ifdef _DEBUG
		m_bPARALLAXOCCLUSION = true;
#endif	// _DEBUG
	}

	void SetWORLD_NORMAL( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nWORLD_NORMAL = i;
#ifdef _DEBUG
		m_bWORLD_NORMAL = true;
#endif	// _DEBUG
	}

	void SetWRINKLEMAP( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nWRINKLEMAP = i;
#ifdef _DEBUG
		m_bWRINKLEMAP = true;
#endif	// _DEBUG
	}

	void SetSUBSURFACESCATTERING( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nSUBSURFACESCATTERING = i;
#ifdef _DEBUG
		m_bSUBSURFACESCATTERING = true;
#endif	// _DEBUG
	}

	void SetDUALLOBE( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nDUALLOBE = i;
#ifdef _DEBUG
		m_bDUALLOBE = true;
#endif	// _DEBUG
	}

	pbr_mrao_ps30_Static_Index(  )
	{
		m_nEMISSIVE = 0;
		m_nPARALLAXOCCLUSION = 0;
		m_nWORLD_NORMAL = 0;
		m_nWRINKLEMAP = 0;
		m_nSUBSURFACESCATTERING = 0;
		m_nDUALLOBE = 0;
#ifdef _DEBUG
		m_bEMISSIVE = false;
		m_bPARALLAXOCCLUSION = false;
		m_bWORLD_NORMAL = false;
		m_bWRINKLEMAP = false;
		m_bSUBSURFACESCATTERING = false;
		m_bDUALLOBE = false;
#endif	// _DEBUG
	}

	int GetIndex() const
	{
		Assert( m_bEMISSIVE && m_bPARALLAXOCCLUSION && m_bWORLD_NORMAL && m_bWRINKLEMAP && m_bSUBSURFACESCATTERING && m_bDUALLOBE );
		AssertMsg( !( ( m_nWRINKLEMAP != 0 ) && ( m_nPARALLAXOCCLUSION != 0 ) ), "Invalid combo combination ( ( WRINKLEMAP != 0 ) && ( PARALLAXOCCLUSION != 0 ) )" );
		AssertMsg( !( ( m_nSUBSURFACESCATTERING != 0 ) && ( m_nPARALLAXOCCLUSION != 0 ) ), "Invalid combo combination ( ( SUBSURFACESCATTERING != 0 ) && ( PARALLAXOCCLUSION != 0 ) )" );
		return ( 60 * m_nEMISSIVE ) + ( 120 * m_nPARALLAXOCCLUSION ) + ( 240 * m_nWORLD_NORMAL ) + ( 480 * m_nWRINKLEMAP ) + ( 960 * m_nSUBSURFACESCATTERING ) + ( 1920 * m_nDUALLOBE ) + 0;
	}
};

#define shaderStaticTest_pbr_mrao_ps30 psh_forgot_to_set_static_EMISSIVE + psh_forgot_to_set_static_PARALLAXOCCLUSION + psh_forgot_to_set_static_WORLD_NORMAL + psh_forgot_to_set_static_WRINKLEMAP + psh_forgot_to_set_static_SUBSURFACESCATTERING + psh_forgot_to_set_static_DUALLOBE


class pbr_mrao_ps30_Dynamic_Index
{
	unsigned int m_nWRITEWATERFOGTODESTALPHA : 2;
	unsigned int m_nPIXELFOGTYPE : 2;
	unsigned int m_nNUM_LIGHTS : 3;
	unsigned int m_nWRITE_DEPTH_TO_DESTALPHA : 2;
#ifdef _DEBUG
	bool m_bWRITEWATERFOGTODESTALPHA : 1;
	bool m_bPIXELFOGTYPE : 1;
	bool m_bNUM_LIGHTS : 1;
	bool m_bWRITE_DEPTH_TO_DESTALPHA : 1;
#endif	// _DEBUG
public:
	void SetWRITEWATERFOGTODESTALPHA( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nWRITEWATERFOGTODESTALPHA = i;
#ifdef _DEBUG
		m_bWRITEWATERFOGTODESTALPHA = true;
#endif	// _DEBUG
	}

	void SetPIXELFOGTYPE( int i )
	{
		Assert( i >= 0 && i <= 2 );
		m_nPIXELFOGTYPE = i;
#ifdef _DEBUG
		m_bPIXELFOGTYPE = true;
#endif	// _DEBUG
	}

	void SetNUM_LIGHTS( int i )
	{
		Assert( i >= 0 && i <= 4 );
		m_nNUM_LIGHTS = i;
#ifdef _DEBUG
		m_bNUM_LIGHTS = true;
#endif	// _DEBUG
	}

	void SetWRITE_DEPTH_TO_DESTALPHA( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nWRITE_DEPTH_TO_DESTALPHA = i;
#ifdef _DEBUG
		m_bWRITE_DEPTH_TO_DESTALPHA = true;
#endif	// _DEBUG
	}

	pbr_mrao_ps30_Dynamic_Index(  )
	{
		m_nWRITEWATERFOGTODESTALPHA = 0;
		m_nPIXELFOGTYPE = 0;
		m_nNUM_LIGHTS = 0;
		m_nWRITE_DEPTH_TO_DESTALPHA = 0;
#ifdef _DEBUG
		m_bWRITEWATERFOGTODESTALPHA = false;
		m_bPIXELFOGTYPE = false;
		m_bNUM_LIGHTS = false;
		m_bWRITE_DEPTH_TO_DESTALPHA = false;
#endif	// _DEBUG
	}

	int GetIndex() const
	{
		Assert( m_bWRITEWATERFOGTODESTALPHA && m_bPIXELFOGTYPE && m_bNUM_LIGHTS && m_bWRITE_DEPTH_TO_DESTALPHA );
		AssertMsg( !( ( m_nPIXELFOGTYPE == 0 ) && ( m_nWRITEWATERFOGTODESTALPHA != 0 ) ), "Invalid combo combination ( ( PIXELFOGTYPE == 0 ) && ( WRITEWATERFOGTODESTALPHA != 0 ) )" );
		AssertMsg( !( 1 && ( 1 && ( ( m_nPIXELFOGTYPE != 1 ) && m_nWRITEWATERFOGTODESTALPHA ) ) ), "Invalid combo combination ( 1 && ( 1 && ( ( PIXELFOGTYPE != 1 ) && WRITEWATERFOGTODESTALPHA ) ) )" );
		AssertMsg( !( 1 && ( 1 && ( ( m_nPIXELFOGTYPE != 1 ) && m_nWRITEWATERFOGTODESTALPHA ) ) ), "Invalid combo combination ( 1 && ( 1 && ( ( PIXELFOGTYPE != 1 ) && WRITEWATERFOGTODESTALPHA ) ) )" );
		AssertMsg( !( 1 && ( 1 && ( ( m_nPIXELFOGTYPE != 1 ) && m_nWRITEWATERFOGTODESTALPHA ) ) ), "Invalid combo combination ( 1 && ( 1 && ( ( PIXELFOGTYPE != 1 ) && WRITEWATERFOGTODESTALPHA ) ) )" );
		return ( 1 * m_nWRITEWATERFOGTODESTALPHA ) + ( 2 * m_nPIXELFOGTYPE ) + ( 6 * m_nNUM_LIGHTS ) + ( 30 * m_nWRITE_DEPTH_TO_DESTALPHA ) + 0;
	}
};

#define shaderDynamicTest_pbr_mrao_ps30 psh_forgot_to_set_dynamic_WRITEWATERFOGTODESTALPHA + psh_forgot_to_set_dynamic_PIXELFOGTYPE + psh_forgot_to_set_dynamic_NUM_LIGHTS + psh_forgot_to_set_dynamic_WRITE_DEPTH_TO_DESTALPHA

