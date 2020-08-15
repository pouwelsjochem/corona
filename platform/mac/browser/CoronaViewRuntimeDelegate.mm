//////////////////////////////////////////////////////////////////////////////
//
// This file is part of the Corona game engine.
// For overview and more information on licensing please refer to README.md 
// Home page: https://github.com/coronalabs/corona
// Contact: support@coronalabs.com
//
//////////////////////////////////////////////////////////////////////////////

#include "Core/Rtt_Build.h"

#import "CoronaViewPrivate.h"

#import "CoronaViewRuntimeDelegate.h"

#include "Display/Rtt_Display.h"
#include "Rtt_LuaContext.h"
#include "Rtt_Runtime.h"
#include "Rtt_ProjectSettings.h"

// ----------------------------------------------------------------------------

namespace Rtt
{

// ----------------------------------------------------------------------------

CoronaViewRuntimeDelegate::CoronaViewRuntimeDelegate( CoronaView *owner )
:	fOwner( owner )
{
}

CoronaViewRuntimeDelegate::~CoronaViewRuntimeDelegate()
{
}

void
CoronaViewRuntimeDelegate::DidInitLuaLibraries( const Runtime& sender ) const
{
    PreloadLibraries( sender );
}

void
CoronaViewRuntimeDelegate::InitializeConfig( const Runtime& sender, lua_State *L ) const
{
	static const char kApplication[] = "application";
	static const char kMetadata[] = "metadata";
	static const char kAppId[] = "appId";

	Rtt_LUA_STACK_GUARD( L );

	lua_getglobal( L, kApplication ); // application
	{
		// [Lua] if not application then
		// [Lua]     application = {}
		// [Lua] end
		if ( ! lua_istable( L, -1 ) )
		{
			lua_pop( L, 1 ); // pop non-nil value
			lua_createtable( L, 0, 1 );

			lua_setglobal( L, kApplication ); // application
		}
		
		int index = lua_gettop( L );

		lua_getfield( L, index, kMetadata );
		{
			// [Lua] if not application.metadata then
			// [Lua]     application.metadata = {}
			// [Lua] end
			if ( ! lua_istable( L, -1 ) )
			{
				lua_pop( L, 1 ); // pop non-nil value
				lua_createtable( L, 0, 1 );

				lua_pushvalue( L, -1 ); // leave metadata on stack, so push extra
				lua_setfield( L, index, kMetadata );
			}

			// [Lua] if type(application.metadata ~= "string" then
			// [Lua]     application.metadata = bundleId
			// [Lua] end
			lua_getfield( L, -1, kAppId );
			bool isValidAppId = ( lua_type( L, -1 ) == LUA_TSTRING );
			lua_pop( L, 1 );
			if ( ! isValidAppId )
			{
				NSString *bundleId = [[NSBundle mainBundle] bundleIdentifier];
				lua_pushstring( L, [bundleId UTF8String] );
				lua_setfield( L, -2, kAppId );
			}
		}
		lua_pop( L, 1 );

		int w = -1, h = -1;

		lua_getfield( L, index, "content" );

		if ( lua_istable( L, -1 ) )
		{
			lua_getfield( L, -1, "width" );
			w = (int) lua_tointeger( L, -1 );
			lua_pop( L, 1 );

			lua_getfield( L, -1, "height" );
			h = (int) lua_tointeger( L, -1 );
			lua_pop( L, 1 );

		}
		lua_pop( L, 1 );

		// TODO: take default orientation into account
		// Make sure we don't end up with a zero size view
		if (w < 1)
		{
			w = 320;
		}
		if (h < 1 )
		{
			h = 480;
		}

		Rtt_TRACE(("====== CoronaViewPrivate: %d x %d ======", w, h));
	}
	lua_pop( L, 1 );
}

void
CoronaViewRuntimeDelegate::DidLoadConfig( const Runtime& sender, lua_State *L ) const
{

}

id< CoronaRuntime >
CoronaViewRuntimeDelegate::GetPluginContext() const
{
	return nil; // fOwner.pluginContext;
}

// ----------------------------------------------------------------------------

} // namespace Rtt

// ----------------------------------------------------------------------------
