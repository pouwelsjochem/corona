//////////////////////////////////////////////////////////////////////////////
//
// This file is part of the Corona game engine.
// For overview and more information on licensing please refer to README.md 
// Home page: https://github.com/coronalabs/corona
// Contact: support@coronalabs.com
//
//////////////////////////////////////////////////////////////////////////////

#ifndef _Rtt_Display_H__
#define _Rtt_Display_H__

////
//
#define GLM_FORCE_RADIANS
#define GLM_FORCE_CXX98
#include "glm/glm.hpp"
#include "glm/ext.hpp"
//
////

#include "Core/Rtt_Types.h"

#include "Core/Rtt_Array.h"
#include "Core/Rtt_Time.h"

#include "Rtt_MPlatform.h"

// ----------------------------------------------------------------------------

struct lua_State;

namespace Rtt
{

struct Rect;

class BitmapPaint;
class DisplayDefaults;
class DisplayObject;
class GroupObject;
class MDisplayDelegate;
class ProgramHeader;
class Renderer;
class Runtime;
class Scene;
class ShaderFactory;
class SpritePlayer;
class StageObject;
class String;
class TextureFactory;
class PlatformSurface;
class RenderingStream;

// ----------------------------------------------------------------------------

class Display
{
	public:
		// List of all premium features (Pro and Enterprise)
		typedef enum _Feature
		{
			kDisplayNewSnapshot,
			kGraphicsDefineEffectGraph,
			kGraphicsDefineEffectFragment,
			kGraphicsDefineEffectVertex,
			kObjectPath,
			kObjectFill,
			kObjectStroke,
			kObjectBlendMode,
			kLineStroke,
			kLineBlendMode,
			kDisplayColorSample,

			kNumFeatures
		}
		Feature;
		
		static const char *StringForFeature( Feature value );
		static bool IsProFeature( Feature value );
		static bool IsEnterpriseFeature( Feature value );
		static const char *GetTierString( Feature value );

	public:
		Display( Runtime& owner );
		virtual ~Display();

	public:
		//! \Return true for success. False for failure.
		virtual bool Initialize( lua_State *L, int configIndex );
		virtual void Teardown();

	protected:
		void ReadRenderingConfig( lua_State *L, int index, ProgramHeader& programHeader );

	public:
		virtual void Start();
		virtual void Restart();

	public:
		// Call on a timer tick
		void Update();
        lua_State *GetL() const;

	public:
		// Note: NOTHING in this class requires being virtual.
		// We need to consider removing "virtual" everywhere
		// from this class.
		virtual void Render();
		virtual void Invalidate();
		virtual StageObject *GetStage();
		virtual StageObject *GetStageOffscreen();

		BitmapPaint *CaptureScreen( bool save_to_file,
									bool output_file_will_be_png_format );

		BitmapPaint *CaptureBounds( Rect *screenBounds,
									bool will_be_saved_to_file,
									bool output_file_will_be_png_format );

		BitmapPaint *CaptureDisplayObject( DisplayObject *object,
											bool will_be_saved_to_file,
											bool output_file_will_be_png_format,
											bool crop_object_to_screen_bounds );

		BitmapPaint *CaptureSave( DisplayObject *object,
									bool crop_object_to_screen_bounds,
									bool output_file_will_be_png_format,
									const ColorUnion *optionalBackgroundColor );

		void ColorSample( float pos_x,
							float pos_y,
							RGBA &output_color );

	private:
		virtual BitmapPaint *Capture( DisplayObject *object,
										Rect *screenBounds,
										bool will_be_saved_to_file,
										bool output_file_will_be_png_format,
										bool crop_object_to_screen_bounds,
										const ColorUnion *optionalBackgroundColor,
										RGBA *optional_output_color );

	public:
		virtual void UnloadResources();
		virtual void ReloadResources();

	public:
		virtual GroupObject *Orphanage();
		virtual GroupObject *HitTestOrphanage();

	public:
		// Size in physical pixels
		virtual S32 DeviceWidth() const;
		virtual S32 DeviceHeight() const;
		
	public:
		// Dynamic Content Scaling
		virtual S32 ContentWidth() const;
		virtual S32 ContentHeight() const;
		virtual S32 ScaledContentWidth() const;
		virtual S32 ScaledContentHeight() const;
	
		virtual Real GetScreenToContentScale() const;
		virtual S32 GetContentToScreenScale() const;

		virtual S32 GetXScreenOffset() const;
		virtual S32 GetYScreenOffset() const;

		virtual void ContentToScreen( S32& x, S32& y ) const;
		virtual void ContentToScreen( S32& x, S32& y, S32& w, S32& h ) const;
		virtual void ContentToPixels( S32& x, S32& y, S32& w, S32& h ) const;

	public:
		virtual void GetContentRect( Rect& outRect ) const;
		virtual const Rect& GetScreenContentBounds() const;

		// Call when window size changes so viewport of RenderingStream can be updated.
		virtual void WindowSizeChanged();

		// Detects if the device width/height of the surface has changed compared to the stream's device width/height.
		// Returns true if they defer, meaning that the caller should then call WindowSizeChanged() to update content scales.
		virtual bool HasWindowSizeChanged() const;

	public:
		Rtt_Allocator *GetAllocator() const;

		Runtime& GetRuntime() { return fOwner; }
		const Runtime& GetRuntime() const { return fOwner; }

		MDisplayDelegate *GetDelegate() const { return fDelegate; }
		void SetDelegate( MDisplayDelegate *delegate ) { fDelegate = delegate; }

		DisplayDefaults& GetDefaults() { return * fDefaults; }
		const DisplayDefaults& GetDefaults() const { return * fDefaults; }

		Rtt_AbsoluteTime GetElapsedTime() const;
		float GetDeltaTimeInSeconds() const { return fDeltaTimeInSeconds; }
		Rtt_AbsoluteTime GetPreviousTime() const { return fPreviousTime; }

		Renderer& GetRenderer() { return *fRenderer; }
		const Renderer& GetRenderer() const { return *fRenderer; }

		ShaderFactory& GetShaderFactory() const { return * fShaderFactory; }

		SpritePlayer& GetSpritePlayer() const { return * fSpritePlayer; }

		TextureFactory& GetTextureFactory() const { return * fTextureFactory; }
				
		static U32 GetMaxTextureSize();
		static const char *GetGlString( const char *s );
		static bool GetGpuSupportsHighPrecisionFragmentShaders();
		static size_t GetMaxVertexTextureUnits();

	public:
		Scene& GetScene() { return *fScene; }
		const Scene& GetScene() const { return *fScene; }

#if defined( Rtt_ANDROID_ENV ) && TEMPORARY_HACK
    // TODO: Remove this once TEMPORARY_HACK is removed in JavaToNativeBridge.cpp
		RenderingStream& GetStream() { return * fStream; }
#endif

		void Collect( lua_State *L );

	private:
		Runtime& fOwner;
		MDisplayDelegate *fDelegate;
		DisplayDefaults *fDefaults;
		float fDeltaTimeInSeconds;
		Rtt_AbsoluteTime fPreviousTime;
		Renderer *fRenderer;
		ShaderFactory *fShaderFactory;
		SpritePlayer *fSpritePlayer;
		TextureFactory *fTextureFactory;
		Scene *fScene;

		// TODO: Refactor data structure portions out
		// We temporarily use RenderingStream b/c it contains key data
		// about window size
		RenderingStream *fStream;
		PlatformSurface *fScreenSurface;

		bool fIsCollecting; // guards against nested calls to Collect()
};

// ----------------------------------------------------------------------------

} // namespace Rtt

// ----------------------------------------------------------------------------

#endif // _Rtt_Display_H__
