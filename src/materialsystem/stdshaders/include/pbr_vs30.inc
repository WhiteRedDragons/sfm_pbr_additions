#pragma once
#include "shaderlib/cshader.h"
class pbr_vs30_Static_Index
{
	unsigned int m_nPROJTEX : 2;
	unsigned int m_nWRINKLEMAPS : 2;
	unsigned int m_nWORLD_NORMAL : 2;
#ifdef _DEBUG
	bool m_bPROJTEX : 1;
	bool m_bWRINKLEMAPS : 1;
	bool m_bWORLD_NORMAL : 1;
#endif	// _DEBUG
public:
	void SetPROJTEX( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nPROJTEX = i;
#ifdef _DEBUG
		m_bPROJTEX = true;
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

	void SetWORLD_NORMAL( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nWORLD_NORMAL = i;
#ifdef _DEBUG
		m_bWORLD_NORMAL = true;
#endif	// _DEBUG
	}

	pbr_vs30_Static_Index(  )
	{
		m_nPROJTEX = 0;
		m_nWRINKLEMAPS = 0;
		m_nWORLD_NORMAL = 0;
#ifdef _DEBUG
		m_bPROJTEX = false;
		m_bWRINKLEMAPS = false;
		m_bWORLD_NORMAL = false;
#endif	// _DEBUG
	}

	int GetIndex() const
	{
		Assert( m_bPROJTEX && m_bWRINKLEMAPS && m_bWORLD_NORMAL );
		return ( 4 * m_nPROJTEX ) + ( 8 * m_nWRINKLEMAPS ) + ( 16 * m_nWORLD_NORMAL ) + 0;
	}
};

#define shaderStaticTest_pbr_vs30 vsh_forgot_to_set_static_PROJTEX + vsh_forgot_to_set_static_WRINKLEMAPS + vsh_forgot_to_set_static_WORLD_NORMAL


class pbr_vs30_Dynamic_Index
{
	unsigned int m_nCOMPRESSION : 2;
	unsigned int m_nSKINNING : 2;
#ifdef _DEBUG
	bool m_bCOMPRESSION : 1;
	bool m_bSKINNING : 1;
#endif	// _DEBUG
public:
	void SetCOMPRESSION( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nCOMPRESSION = i;
#ifdef _DEBUG
		m_bCOMPRESSION = true;
#endif	// _DEBUG
	}

	void SetSKINNING( int i )
	{
		Assert( i >= 0 && i <= 1 );
		m_nSKINNING = i;
#ifdef _DEBUG
		m_bSKINNING = true;
#endif	// _DEBUG
	}

	pbr_vs30_Dynamic_Index(  )
	{
		m_nCOMPRESSION = 0;
		m_nSKINNING = 0;
#ifdef _DEBUG
		m_bCOMPRESSION = false;
		m_bSKINNING = false;
#endif	// _DEBUG
	}

	int GetIndex() const
	{
		Assert( m_bCOMPRESSION && m_bSKINNING );
		return ( 1 * m_nCOMPRESSION ) + ( 2 * m_nSKINNING ) + 0;
	}
};

#define shaderDynamicTest_pbr_vs30 vsh_forgot_to_set_dynamic_COMPRESSION + vsh_forgot_to_set_dynamic_SKINNING

