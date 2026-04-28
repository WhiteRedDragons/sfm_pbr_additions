// ALL SKIP STATEMENTS THAT AFFECT THIS SHADER!!!
// ( $WRINKLEMAP != 0 && $PARALLAXOCCLUSION != 0 )
// ( $SUBSURFACESCATTERING != 0 && $LIGHTWARPTEXTURE != 0 )
// ( $SUBSURFACESCATTERING != 0 &&  $PARALLAXOCCLUSION != 0 )
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
class pbr_sg_projtex_ps30_Static_Index
{
	unsigned int m_nFLASHLIGHTDEPTHFILTERMODE : 2;
	unsigned int m_nPARALLAXOCCLUSION : 2;
	unsigned int m_nWORLD_NORMAL : 2;
	unsigned int m_nWRINKLEMAP : 2;
	unsigned int m_nSUBSURFACESCATTERING : 2;
	unsigned int m_nDUALLOBE : 2;
#ifdef _DEBUG
	bool m_bFLASHLIGHTDEPTHFILTERMODE : 1;
	bool m_bPARALLAXOCCLUSION : 1;
	bool m_bWORLD_NORMAL : 1;
	bool m_bWRINKLEMAP : 1;
	bool m_bSUBSURFACESCATTERING : 1;
	bool m_bDUALLOBE : 1;
#endif	// _DEBUG
public:
	void SetFLASHLIGHTDEPTHFILTERMODE( int i )
	{
		Assert( i >= 0 && i <= 2 );
		m_nFLASHLIGHTDEPTHFILTERMODE = i;
#ifdef _DEBUG
		m_bFLASHLIGHTDEPTHFILTERMODE = true;
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

	pbr_sg_projtex_ps30_Static_Index(  )
	{
		m_nFLASHLIGHTDEPTHFILTERMODE = 0;
		m_nPARALLAXOCCLUSION = 0;
		m_nWORLD_NORMAL = 0;
		m_nWRINKLEMAP = 0;
		m_nSUBSURFACESCATTERING = 0;
		m_nDUALLOBE = 0;
#ifdef _DEBUG
		m_bFLASHLIGHTDEPTHFILTERMODE = false;
		m_bPARALLAXOCCLUSION = false;
		m_bWORLD_NORMAL = false;
		m_bWRINKLEMAP = false;
		m_bSUBSURFACESCATTERING = false;
		m_bDUALLOBE = false;
#endif	// _DEBUG
	}

	int GetIndex() const
	{
		Assert( m_bFLASHLIGHTDEPTHFILTERMODE && m_bPARALLAXOCCLUSION && m_bWORLD_NORMAL && m_bWRINKLEMAP && m_bSUBSURFACESCATTERING && m_bDUALLOBE );
		AssertMsg( !( ( m_nWRINKLEMAP != 0 ) && ( m_nPARALLAXOCCLUSION != 0 ) ), "Invalid combo combination ( ( WRINKLEMAP != 0 ) && ( PARALLAXOCCLUSION != 0 ) )" );
		AssertMsg( !( ( m_nSUBSURFACESCATTERING != 0 ) && ( m_nPARALLAXOCCLUSION != 0 ) ), "Invalid combo combination ( ( SUBSURFACESCATTERING != 0 ) && ( PARALLAXOCCLUSION != 0 ) )" );
		return ( 12 * m_nFLASHLIGHTDEPTHFILTERMODE ) + ( 36 * m_nPARALLAXOCCLUSION ) + ( 72 * m_nWORLD_NORMAL ) + ( 144 * m_nWRINKLEMAP ) + ( 288 * m_nSUBSURFACESCATTERING ) + ( 576 * m_nDUALLOBE ) + 0;
	}
};

#define shaderStaticTest_pbr_sg_projtex_ps30 psh_forgot_to_set_static_FLASHLIGHTDEPTHFILTERMODE + psh_forgot_to_set_static_PARALLAXOCCLUSION + psh_forgot_to_set_static_WORLD_NORMAL + psh_forgot_to_set_static_WRINKLEMAP + psh_forgot_to_set_static_SUBSURFACESCATTERING + psh_forgot_to_set_static_DUALLOBE


class pbr_sg_projtex_ps30_Dynamic_Index
{
	unsigned int m_nPIXELFOGTYPE : 2;
	unsigned int m_nFLASHLIGHTSHADOWS : 2;
	unsigned int m_nUBERLIGHT : 2;
#ifdef _DEBUG
	bool m_bPIXELFOGTYPE : 1;
	bool m_bFLASHLIGHTSHADOWS : 1;
	bool m_bUBERLIGHT : 1;
#endif	// _DEBUG
public:
	void SetPIXELFOGTYPE( int i )
	{
		Assert( i >= 0 && i <= 2 );
		m_nPIXELFOGTYPE = i;
#ifdef _DEBUG
		m_bPIXELFOGTYPE = true;
#endif	// _DEBUG
	}

	void SetFLASHLIGHTSHADOWS( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nFLASHLIGHTSHADOWS = i;
#ifdef _DEBUG
		m_bFLASHLIGHTSHADOWS = true;
#endif	// _DEBUG
	}

	void SetUBERLIGHT( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nUBERLIGHT = i;
#ifdef _DEBUG
		m_bUBERLIGHT = true;
#endif	// _DEBUG
	}

	pbr_sg_projtex_ps30_Dynamic_Index(  )
	{
		m_nPIXELFOGTYPE = 0;
		m_nFLASHLIGHTSHADOWS = 0;
		m_nUBERLIGHT = 0;
#ifdef _DEBUG
		m_bPIXELFOGTYPE = false;
		m_bFLASHLIGHTSHADOWS = false;
		m_bUBERLIGHT = false;
#endif	// _DEBUG
	}

	int GetIndex() const
	{
		Assert( m_bPIXELFOGTYPE && m_bFLASHLIGHTSHADOWS && m_bUBERLIGHT );
		return ( 1 * m_nPIXELFOGTYPE ) + ( 3 * m_nFLASHLIGHTSHADOWS ) + ( 6 * m_nUBERLIGHT ) + 0;
	}
};

#define shaderDynamicTest_pbr_sg_projtex_ps30 psh_forgot_to_set_dynamic_PIXELFOGTYPE + psh_forgot_to_set_dynamic_FLASHLIGHTSHADOWS + psh_forgot_to_set_dynamic_UBERLIGHT

