//////////////////////////////////////////////////////////////////////////////
//
// This file is part of the Corona game engine.
// For overview and more information on licensing please refer to README.md 
// Home page: https://github.com/coronalabs/corona
// Contact: support@coronalabs.com
//
//////////////////////////////////////////////////////////////////////////////

#include "Core/Rtt_Build.h"

#include "Display/Rtt_Display.h"

#include "Core/Rtt_Geometry.h"
#include "Display/Rtt_CPUResourcePool.h"
#include "Display/Rtt_DisplayDefaults.h"
#include "Display/Rtt_BitmapPaint.h"
#include "Display/Rtt_Paint.h"
#include "Display/Rtt_Scene.h"
#include "Display/Rtt_ShaderFactory.h"
#include "Display/Rtt_SpritePlayer.h"
#include "Display/Rtt_TextureFactory.h"
#include "Display/Rtt_TextureResource.h"

#include "Rtt_BufferBitmap.h"
#include "Rtt_Lua.h"
#include "Rtt_LuaContext.h"
#include "Rtt_PlatformSurface.h"
#include "CoronaLua.h"

#include "Renderer/Rtt_GLRenderer.h"
#include "Renderer/Rtt_FrameBufferObject.h"
#include "Renderer/Rtt_Matrix_Renderer.h"
#include "Renderer/Rtt_Program.h"
#include "Renderer/Rtt_Texture.h"

// TODO: Remove when we replace TemporaryHackStream
#include "Rtt_RenderingStream.h"

// TODO: Remove dependency on Runtime's MCachedResourceLibrary interface
#include "Rtt_Runtime.h"

#define ENABLE_DEBUG_PRINT	0

// ----------------------------------------------------------------------------

namespace Rtt
{

// ----------------------------------------------------------------------------

Display::Display( Runtime& owner )
:	fOwner( owner ),
	fDefaults( Rtt_NEW( owner.Allocator(), DisplayDefaults ) ),
	fDeltaTimeInSeconds( 0.0f ),
	fPreviousTime( owner.GetElapsedTime() ),
	fRenderer( NULL ),
	fShaderFactory( NULL ),
	fSpritePlayer( Rtt_NEW( owner.Allocator(), SpritePlayer( owner.Allocator() ) ) ),
	fTextureFactory( Rtt_NEW( owner.Allocator(), TextureFactory( * this ) ) ),
	fScene( Rtt_NEW( & owner.GetAllocator(), Scene( owner.Allocator(), * this ) ) ),
	fStream( Rtt_NEW( owner.GetAllocator(), RenderingStream( owner.GetAllocator() ) ) ),
	fScreenSurface( owner.Platform().CreateScreenSurface() ),
	fIsCollecting( false )
{
}

Display::~Display()
{
	Paint::Finalize();

    //Needs to be done before deletes, because it uses scene etc
    fTextureFactory->ReleaseByType( TextureResource::kTextureResource_Any );
    
	Rtt_DELETE( fScreenSurface );
	Rtt_DELETE( fStream );
	Rtt_DELETE( fScene );
	Rtt_DELETE( fTextureFactory );
	Rtt_DELETE( fSpritePlayer );
	Rtt_DELETE( fShaderFactory );
	Rtt_DELETE( fRenderer );
	Rtt_DELETE( fDefaults );
}

bool
Display::Initialize( lua_State *L, int configIndex )
{
	bool result = false;

	// Only initialize once
	if ( Rtt_VERIFY( ! fRenderer ) )
	{
		Rtt_Allocator *allocator = GetRuntime().GetAllocator();
		fRenderer = Rtt_NEW( allocator, GLRenderer( allocator ) );
		fRenderer->Initialize();
		
		CPUResourcePool *resourcePoolObserver = Rtt_NEW(allocator,CPUResourcePool());
		fRenderer->SetCPUResourceObserver(resourcePoolObserver);

		ProgramHeader programHeader;
		
		if ( configIndex > 0 )
		{
			ReadRenderingConfig( L, configIndex, programHeader );
		}
		fStream->SetOptimalContentSize( fScreenSurface->Width(), fScreenSurface->Height() );

		fShaderFactory = Rtt_NEW( allocator, ShaderFactory( *this, programHeader ) );

		result = true;
	}

	return result;
}
    
void
Display::Teardown()
{
    GetTextureFactory().Teardown();
}

void
Display::ReadRenderingConfig( lua_State *L, int index, ProgramHeader& programHeader )
{
	Rtt_ASSERT( 1 == index );	
	Rtt_ASSERT( lua_istable( L, index ) );

	lua_getfield( L, index, "shaderPrecision" );
	const char *shaderPrecision = lua_tostring( L, -1 );
	if ( shaderPrecision )
	{
		// This is safer to do than strcpy( outShaderPrecision, shaderPrecision, );
		ProgramHeader::Precision p = ProgramHeader::PrecisionForString( shaderPrecision );
		programHeader.SetPrecision( p );
	}
	else if ( lua_type( L, -1 ) == LUA_TTABLE )
	{
		int index = lua_gettop( L );

		lua_pushnil( L );
		while ( lua_next( L, index ) )
		{
			const char *key = lua_tostring( L, -2 );
			ProgramHeader::Type t = ProgramHeader::TypeForString( key );

			const char *value = lua_tostring( L, -1 );
			ProgramHeader::Precision p = ProgramHeader::PrecisionForString( value );

			programHeader.SetPrecision( t, p );

			lua_pop( L, 1 );
		}
	}
	lua_pop( L, 1 );

	lua_getfield( L, index, "minContentWidth" );
	int minContentWidth = (int) lua_tointeger( L, -1 );
	lua_pop( L, 1 );

	lua_getfield( L, index, "maxContentWidth" );
	int maxContentWidth = (int) lua_tointeger( L, -1 );
	lua_pop( L, 1 );

	lua_getfield( L, index, "minContentHeight" );
	int minContentHeight = (int) lua_tointeger( L, -1 );
	lua_pop( L, 1 );
	
	lua_getfield( L, index, "maxContentHeight" );
	int maxContentHeight = (int) lua_tointeger( L, -1 );
	lua_pop( L, 1 );

	fStream->SetContentSizeRestrictions( minContentWidth, maxContentWidth, minContentHeight, maxContentHeight );

	Rtt_ASSERT( 1 == lua_gettop( L ) );	
}

lua_State *
Display::GetL() const
{
	const Runtime& runtime = GetRuntime();
	lua_State *L = ( runtime.IsVMContextValid() ? runtime.VMContext().L() : NULL );
	return L;
}

void
Display::Start()
{
	// The call to SetOptimalContentSize requires a current OpenGL context which we can't assume yet.
	// RuntimeGuard will ensure that an OpenGL context is current and that locks are in place for multithreaded conditions.
	// We must do this inside a scope because the RuntimeGuard destructor is relied on to do unlocking and restoring of OpenGL state as necessary.
	// The overloaded operator()() below uses this same RuntimeGuard technique.
	// TODO: We may not need this scope guard.
	// The runtime guard is re-entrant safe so it should be safe to leave guarded when we hit the operator()().
	RuntimeGuard guard( GetRuntime() );

	RenderingStream& stream = * fStream;
	stream.SetOptimalContentSize( fScreenSurface->Width(), fScreenSurface->Height() );
}
void
Display::Restart()
{
	RenderingStream& stream = * fStream;
	stream.SetOptimalContentSize( fScreenSurface->Width(), fScreenSurface->Height() );
}
	
void
Display::Update()
{
	Runtime& runtime = fOwner;
	lua_State *L = fOwner.VMContext().L();
	fSpritePlayer->Run( L, Rtt_AbsoluteToMilliseconds(runtime.GetElapsedTime()) );

	GetScene().QueueUpdateOfUpdatables();

	const FrameEvent& fe = FrameEvent::Constant();
	fe.Dispatch( L, runtime );
	
	const RenderEvent& re = RenderEvent::Constant();
	re.Dispatch( L, runtime );
}

void
Display::Render()
{
	{
		Rtt_AbsoluteTime elapsedTime = GetRuntime().GetElapsedTime();

		const Rtt::Real kMillisecondsPerSecond = 1000.0f;
		// NOT _USED: Rtt::Real totalTime = Rtt_AbsoluteToMilliseconds( elapsedTime ) / kMillisecondsPerSecond;
		fDeltaTimeInSeconds = Rtt_AbsoluteToMilliseconds( elapsedTime - fPreviousTime ) / kMillisecondsPerSecond;

		fPreviousTime = elapsedTime;

		// Use this when debugging:
		//fDeltaTimeInSeconds = ( 1.0f / 30.0f );
	}

	GetScene().Render( * fRenderer, * fScreenSurface );
}

void
Display::Invalidate()
{
	GetScene().Invalidate();
}

StageObject *
Display::GetStage()
{
	return & GetScene().CurrentStage();
}

StageObject *
Display::GetStageOffscreen()
{
	return & GetScene().OffscreenStage();
}

BitmapPaint *
Display::CaptureScreen()
{
	return Capture( NULL,
					NULL,
					false,
					false,
					NULL,
					NULL );
}

BitmapPaint *
Display::CaptureBounds( Rect *screenBounds )
{
	return Capture( NULL,
					screenBounds,
					false,
					false,
					NULL,
					NULL );
}

BitmapPaint *
Display::CaptureDisplayObject( DisplayObject *object, bool crop_object_to_screen_bounds )
{
	return Capture( object,
					NULL,
					false,
					crop_object_to_screen_bounds,
					NULL,
					NULL );
}

BitmapPaint *
Display::CaptureSave( DisplayObject *object,
						bool crop_object_to_screen_bounds,
						const ColorUnion *optionalBackgroundColor )
{
	return Capture( object,
					NULL,
					true,
					crop_object_to_screen_bounds,
					optionalBackgroundColor,
					NULL );
}

void
Display::ColorSample( float pos_x,
						float pos_y,
						RGBA &output_color )
{
	Rect screenBounds;
	screenBounds.Initialize( pos_x, pos_y, 1.0f, 1.0f );

	BitmapPaint *paint = Capture( NULL,
									&screenBounds,
									true,
									false,
									NULL,
									&output_color );
	if( ! paint )
	{
		Rtt_TRACE( ( "Unable to capture screen. The platform or device might not be supported." ) );
		return;
	}

	Rtt_DELETE( paint );
}

BitmapPaint *
Display::Capture( DisplayObject *object,
					Rect *screenBounds,
					bool will_be_saved_to_file,
					bool crop_object_to_screen_bounds,
					const ColorUnion *optionalBackgroundColor,
					RGBA *optional_output_color )
{
	// Do not continue if given invalid screen bounds.
	if( screenBounds )
	{
		if ((screenBounds->xMin >= screenBounds->xMax) ||
			(screenBounds->yMin >= screenBounds->yMax))
		{
			return NULL;
		}
	}

	fRenderer->BeginFrame( 0.1f, 0.1f, GetScreenToContentScale() );

	////////////////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////////////////

	Runtime& runtime = GetRuntime();
	Rtt_Allocator *allocator = runtime.GetAllocator();

	////////////////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////////////////
	//FBO SETUP

	// Calculate pixel dimensions for texture
	// The source position to capture from.
	Real x_in_content_units = 0.0f;
	Real y_in_content_units = 0.0f;
	Real w_in_content_units = 0.0f;
	Real h_in_content_units = 0.0f;

	Rtt::Real offscreenViewMatrix[16];
	Rtt::CreateViewMatrix( 0.0f, 0.0f, 0.5f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, offscreenViewMatrix );

	Rtt::Real offscreenProjMatrix[16];
	if ( object )
	{
		// Calculate the bounds of the given display object in pixels.
		// Clip the object to the screen's bounds.
		Rect objectBounds = object->StageBounds();
		if (screenBounds)
		{
			objectBounds.Intersect(*screenBounds);
		}
		else
		{
			if( crop_object_to_screen_bounds )
			{
				objectBounds.Intersect(GetScreenContentBounds());
			}
		}

		x_in_content_units = objectBounds.xMin;
		y_in_content_units = objectBounds.yMin;
		w_in_content_units = ( objectBounds.xMax - x_in_content_units );
		h_in_content_units = ( objectBounds.yMax - y_in_content_units );
		Rtt::CreateOrthoMatrix( x_in_content_units, objectBounds.xMax, y_in_content_units, objectBounds.yMax, 0.0f, 1.0f, offscreenProjMatrix );
	}
	else // Full screen or screenBounds.
	{
		Rect *full_screen = const_cast< Rect * >( &GetScreenContentBounds() );

		Rect *bounds_to_use = NULL;

		if( screenBounds )
		{
			// Subregion of the screen.
			screenBounds->Intersect( *full_screen );
			bounds_to_use = screenBounds;
		}
		else
		{
			// Full screen.
			bounds_to_use = full_screen;
		}

		x_in_content_units = bounds_to_use->xMin;
		y_in_content_units = bounds_to_use->yMin;
		w_in_content_units = ( bounds_to_use->xMax - x_in_content_units );
		h_in_content_units = ( bounds_to_use->yMax - y_in_content_units );

		Rtt::CreateOrthoMatrix( x_in_content_units, bounds_to_use->xMax, y_in_content_units, bounds_to_use->yMax, 0.0f, 1.0f, offscreenProjMatrix );
	}

	// The source position to capture from.
	S32 x_in_pixels = 0;
	S32 y_in_pixels = 0;
	S32 w_in_pixels = ceilf(w_in_content_units);
	S32 h_in_pixels = ceilf(h_in_content_units);
	{
		ContentToScreen( x_in_pixels,
							y_in_pixels,
							w_in_pixels,
							h_in_pixels );

		// We ONLY want the w_in_pixels and h_in_pixels.
		// Using anything but 0 here breaks Android.
		x_in_pixels = 0;
		y_in_pixels = 0;
	}

	return NULL;
}

void
Display::UnloadResources()
{
	GetRenderer().ReleaseGPUResources();
}

void
Display::ReloadResources()
{
	GetRenderer().ReleaseGPUResources();
	GetRenderer().Initialize();
}

GroupObject *
Display::Orphanage()
{
	return & GetScene().Orphanage();
}

GroupObject *
Display::HitTestOrphanage()
{
	return & GetScene().SnapshotOrphanage();
}

S32
Display::DeviceWidth() const
{
	return fScreenSurface->Width();
}

S32
Display::DeviceHeight() const
{
	return fScreenSurface->Height();
}

S32
Display::ContentWidth() const
{
	return fStream->ContentWidth();
}

S32
Display::ContentHeight() const
{
	return fStream->ContentHeight();
}

S32
Display::MinContentWidth() const
{
	return fStream->MinContentWidth();
}

S32
Display::MinContentHeight() const
{
	return fStream->MinContentHeight();
}

S32
Display::MaxContentWidth() const
{
	return fStream->MaxContentWidth();
}

S32
Display::MaxContentHeight() const
{
	return fStream->MaxContentHeight();
}


S32
Display::ScaledContentWidth() const
{
    return fStream->ScaledContentWidth();
}

S32
Display::ScaledContentHeight() const
{
    return fStream->ScaledContentHeight();
}


Real
Display::GetScreenToContentScale() const
{
	return fStream->GetScreenToContentScale();
}

S32
Display::GetContentToScreenScale() const
{
	return fStream->GetContentToScreenScale();
}

S32
Display::GetXScreenOffset() const
{
	return fStream->GetXScreenOffset();
}

S32
Display::GetYScreenOffset() const
{
	return fStream->GetYScreenOffset();
}

void
Display::ContentToScreen( S32& x, S32& y ) const
{
	S32 w = 0;
	S32 h = 0;
	ContentToScreen( x, y, w, h );
}

void
Display::ContentToScreen( S32& x, S32& y, S32& w, S32& h ) const
{
	fStream->ContentToScreen( x, y, w, h );
}

void
Display::ContentToPixels( S32& x, S32& y, S32& w, S32& h ) const
{
	fStream->ContentToPixels( x, y, w, h );
}

void
Display::GetContentRect( Rect& outRect ) const
{
	const Vertex2 minCorner = { Rtt_REAL_0, Rtt_REAL_0 };
	const Vertex2 maxCorner = { Rtt_IntToReal( ContentWidth() ), Rtt_IntToReal( ContentHeight() ) };
	const Rect windowRect( minCorner, maxCorner );
	outRect = windowRect;
}

const Rect&
Display::GetScreenContentBounds() const
{
	return ((const RenderingStream *)fStream)->GetScreenContentBounds();
}

void
Display::WindowSizeChanged()
{
	const Runtime& runtime = GetRuntime();
	runtime.Begin();
	{
		RenderingStream *stream = fStream;
		if ( stream ) // Is this check neccesary?
		{
			stream->SetOptimalContentSize( fScreenSurface->Width(), fScreenSurface->Height() );
		}
	}
	runtime.End();
}

bool
Display::HasWindowSizeChanged() const
{
	if (!fStream || !fScreenSurface)
	{
		return false;
	}
	return ((fStream->DeviceWidth() != fScreenSurface->Width()) || (fStream->DeviceHeight() != fScreenSurface->Height()));
}

Rtt_Allocator *
Display::GetAllocator() const
{
	return fOwner.GetAllocator();
}

Rtt_AbsoluteTime
Display::GetElapsedTime() const
{
	return fOwner.GetElapsedTime();
}

U32
Display::GetMaxTextureSize()
{
	U32 result = 1024;
	result = Renderer::GetMaxTextureSize();
	Rtt_ASSERT( result > 0 );
	return result;
}

const char *
Display::GetGlString( const char *s )
{
	return Renderer::GetGlString( s );
}

bool
Display::GetGpuSupportsHighPrecisionFragmentShaders()
{
	return Renderer::GetGpuSupportsHighPrecisionFragmentShaders();
}

size_t
Display::GetMaxVertexTextureUnits()
{
	return Renderer::GetMaxVertexTextureUnits();
}

void
Display::Collect( lua_State *L )
{
	// Protect against re-entrancy (not thread safe)
	if ( ! fIsCollecting )
	{
		fIsCollecting = true;
		GroupObject::CollectUnreachables( L, GetScene(), * Orphanage() );
		fIsCollecting = false;
	}
}


// ----------------------------------------------------------------------------

} // namespace Rtt

// ----------------------------------------------------------------------------

