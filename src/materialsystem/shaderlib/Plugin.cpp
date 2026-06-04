// Interface for Plugin Loader in SFM
// by ficool2

#include "engine/iserverplugin.h"

#include "materialsystem/ishadersystem.h"

#include "ProceduralTextures.h"

#include <Windows.h>

const char *ccDescription = "ShiroDkxtro2's Version of the PBR Shader";

class CPlugin_ShaderPBR : public IServerPluginCallbacks
{
	bool Load( CreateInterfaceFn interfaceFactory, CreateInterfaceFn gameServerFactory ) override;
	void Unload( void ) override {}
	void Pause( void ) override {}
	void UnPause( void ) override {}
	const char* GetPluginDescription( void ) override { return ccDescription; }
	void LevelInit( char const* pMapName ) override;
	void ServerActivate( edict_t* pEdictList, int edictCount, int clientMax ) override {}
	void GameFrame( bool simulating ) override {}
	void LevelShutdown( void ) override;
	void ClientActive( edict_t* pEntity ) override {}
	void ClientFullyConnect( edict_t* pEntity ) override {}
	void ClientDisconnect( edict_t* pEntity ) override {}
	void ClientPutInServer( edict_t* pEntity, char const* playername ) override {}
	void SetCommandClient( int index ) override {}
	void ClientSettingsChanged( edict_t* pEdict ) override {}
	PLUGIN_RESULT ClientConnect( bool* bAllowConnect, edict_t* pEntity, const char* pszName, const char* pszAddress, char* reject, int maxrejectlen ) override { return PLUGIN_CONTINUE; }
	PLUGIN_RESULT ClientCommand( edict_t* pEntity, const CCommand& args ) override { return PLUGIN_CONTINUE; }
	PLUGIN_RESULT NetworkIDValidated( const char* pszUserName, const char* pszNetworkID ) override { return PLUGIN_CONTINUE; }
	void OnQueryCvarValueFinished( QueryCvarCookie_t iCookie, edict_t* pPlayerEntity, EQueryCvarValueStatus eStatus, const char* pCvarName, const char* pCvarValue ) override {}
	void OnEdictAllocated( edict_t* edict ) override {}
	void OnEdictFreed( const edict_t* edict )  override {}

	bool LoadShaders();
};

bool CPlugin_ShaderPBR::Load( CreateInterfaceFn interfaceFactory, CreateInterfaceFn gameServerFactory )
{
	ConColorMsg( Color( 255, 255, 255, 255 ), "[PBR Shader - ShiroDkxtro2] Loading...\n" ); 

	materials = ( IMaterialSystem* )interfaceFactory( MATERIAL_SYSTEM_INTERFACE_VERSION, NULL );

	if ( !LoadShaders() )
		return false;

	return true;
}

void CPlugin_ShaderPBR::LevelInit(char const* pMapName)
{
	ConColorMsg(Color(255, 255, 255, 255), "[PBR Shader - ShiroDkxtro2] Creating Procedural Textures\n");
	InitProceduralTextures(materials);
}

void CPlugin_ShaderPBR::LevelShutdown(void)
{
	ConColorMsg(Color(255, 255, 255, 255), "[PBR Shader - ShiroDkxtro2] Shutting down Procedural Textures\n");
	ShutdownProceduralTextures();
}

HMODULE g_hModule = NULL;

class CShaderSystem : public IShaderSystemInternal
{
public:
	virtual bool LoadShaderDLL( const char* pFullPath, const char* pPathID, bool bModShaderDLL ) = 0;
};

bool CPlugin_ShaderPBR::LoadShaders()
{
	CShaderSystem* pShaderSystem = dynamic_cast< CShaderSystem* >(
		static_cast< IShaderSystem* >( materials->QueryInterface( SHADERSYSTEM_INTERFACE_VERSION ) ) );

	char szFileName[MAX_PATH];
	GetModuleFileName( g_hModule, szFileName, sizeof( szFileName ) );

	pShaderSystem->LoadShaderDLL( szFileName, "GAME", true );

	return true;
}

EXPOSE_INTERFACE( CPlugin_ShaderPBR, IServerPluginCallbacks, INTERFACEVERSION_ISERVERPLUGINCALLBACKS );

BOOL WINAPI DllMain( HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved )
{
	switch ( fdwReason )
	{
		case DLL_PROCESS_ATTACH:
			g_hModule = hinstDLL;
			break;
	}

	return TRUE;
}