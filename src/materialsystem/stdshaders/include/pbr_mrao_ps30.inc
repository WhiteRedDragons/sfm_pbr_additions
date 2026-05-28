// ALL SKIP STATEMENTS THAT AFFECT THIS SHADER!!!
// ( $PARALLAXOCCLUSION != 0 && $WRINKLEMAP != 0 )
// ( $PARALLAXOCCLUSION != 0 && $SUBSURFACESCATTERING != 0 )
// ( $DUALLOBE != 0 && $PREMULTIPLIEDALPHA != 0 )

#pragma once
#include "shaderlib/cshader.h"
class pbr_mrao_ps30_Static_Index
{
	unsigned int m_nPARALLAXOCCLUSION : 2;
	unsigned int m_nWRINKLEMAPS : 2;
	unsigned int m_nSUBSURFACESCATTERING : 2;
	unsigned int m_nDUALLOBE : 2;
	unsigned int m_nPREMULTIPLIEDALPHA : 2;
#ifdef _DEBUG
	bool m_bPARALLAXOCCLUSION : 1;
	bool m_bWRINKLEMAPS : 1;
	bool m_bSUBSURFACESCATTERING : 1;
	bool m_bDUALLOBE : 1;
	bool m_bPREMULTIPLIEDALPHA : 1;
#endif	// _DEBUG
public:
	void SetPARALLAXOCCLUSION( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nPARALLAXOCCLUSION = i;
#ifdef _DEBUG
		m_bPARALLAXOCCLUSION = true;
#endif	// _DEBUG
	}

	void SetWRINKLEMAPS( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nWRINKLEMAPS = i;
#ifdef _DEBUG
		m_bWRINKLEMAPS = true;
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

	void SetPREMULTIPLIEDALPHA( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nPREMULTIPLIEDALPHA = i;
#ifdef _DEBUG
		m_bPREMULTIPLIEDALPHA = true;
#endif	// _DEBUG
	}

	pbr_mrao_ps30_Static_Index(  )
	{
		m_nPARALLAXOCCLUSION = 0;
		m_nWRINKLEMAPS = 0;
		m_nSUBSURFACESCATTERING = 0;
		m_nDUALLOBE = 0;
		m_nPREMULTIPLIEDALPHA = 0;
#ifdef _DEBUG
		m_bPARALLAXOCCLUSION = false;
		m_bWRINKLEMAPS = false;
		m_bSUBSURFACESCATTERING = false;
		m_bDUALLOBE = false;
		m_bPREMULTIPLIEDALPHA = false;
#endif	// _DEBUG
	}

	int GetIndex() const
	{
		Assert( m_bPARALLAXOCCLUSION && m_bWRINKLEMAPS && m_bSUBSURFACESCATTERING && m_bDUALLOBE && m_bPREMULTIPLIEDALPHA );
		AssertMsg( !( ( m_nPARALLAXOCCLUSION != 0 ) && ( m_nSUBSURFACESCATTERING != 0 ) ), "Invalid combo combination ( ( PARALLAXOCCLUSION != 0 ) && ( SUBSURFACESCATTERING != 0 ) )" );
		AssertMsg( !( ( m_nDUALLOBE != 0 ) && ( m_nPREMULTIPLIEDALPHA != 0 ) ), "Invalid combo combination ( ( DUALLOBE != 0 ) && ( PREMULTIPLIEDALPHA != 0 ) )" );
		return ( 5 * m_nPARALLAXOCCLUSION ) + ( 10 * m_nWRINKLEMAPS ) + ( 20 * m_nSUBSURFACESCATTERING ) + ( 40 * m_nDUALLOBE ) + ( 80 * m_nPREMULTIPLIEDALPHA ) + 0;
	}
};

#define shaderStaticTest_pbr_mrao_ps30 psh_forgot_to_set_static_PARALLAXOCCLUSION + psh_forgot_to_set_static_WRINKLEMAPS + psh_forgot_to_set_static_SUBSURFACESCATTERING + psh_forgot_to_set_static_DUALLOBE + psh_forgot_to_set_static_PREMULTIPLIEDALPHA


class pbr_mrao_ps30_Dynamic_Index
{
	unsigned int m_nNUM_LIGHTS : 3;
#ifdef _DEBUG
	bool m_bNUM_LIGHTS : 1;
#endif	// _DEBUG
public:
	void SetNUM_LIGHTS( int i )
	{
		Assert( i >= 0 && i <= 4 );
		m_nNUM_LIGHTS = i;
#ifdef _DEBUG
		m_bNUM_LIGHTS = true;
#endif	// _DEBUG
	}

	pbr_mrao_ps30_Dynamic_Index(  )
	{
		m_nNUM_LIGHTS = 0;
#ifdef _DEBUG
		m_bNUM_LIGHTS = false;
#endif	// _DEBUG
	}

	int GetIndex() const
	{
		Assert( m_bNUM_LIGHTS );
		return ( 1 * m_nNUM_LIGHTS ) + 0;
	}
};

#define shaderDynamicTest_pbr_mrao_ps30 psh_forgot_to_set_dynamic_NUM_LIGHTS

