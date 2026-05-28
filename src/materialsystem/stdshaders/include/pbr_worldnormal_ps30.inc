// ALL SKIP STATEMENTS THAT AFFECT THIS SHADER!!!
// ( $PARALLAXOCCLUSION != 0 && $WRINKLEMAP != 0 )

#pragma once
#include "shaderlib/cshader.h"
class pbr_worldnormal_ps30_Static_Index
{
	unsigned int m_nPARALLAXOCCLUSION : 2;
	unsigned int m_nWRINKLEMAPS : 2;
#ifdef _DEBUG
	bool m_bPARALLAXOCCLUSION : 1;
	bool m_bWRINKLEMAPS : 1;
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

	pbr_worldnormal_ps30_Static_Index(  )
	{
		m_nPARALLAXOCCLUSION = 0;
		m_nWRINKLEMAPS = 0;
#ifdef _DEBUG
		m_bPARALLAXOCCLUSION = false;
		m_bWRINKLEMAPS = false;
#endif	// _DEBUG
	}

	int GetIndex() const
	{
		Assert( m_bPARALLAXOCCLUSION && m_bWRINKLEMAPS );
		return ( 1 * m_nPARALLAXOCCLUSION ) + ( 2 * m_nWRINKLEMAPS ) + 0;
	}
};

#define shaderStaticTest_pbr_worldnormal_ps30 psh_forgot_to_set_static_PARALLAXOCCLUSION + psh_forgot_to_set_static_WRINKLEMAPS


class pbr_worldnormal_ps30_Dynamic_Index
{
public:
	pbr_worldnormal_ps30_Dynamic_Index(  )
	{
	}

	int GetIndex() const
	{
		return 0;
	}
};

#define shaderDynamicTest_pbr_worldnormal_ps30 1

