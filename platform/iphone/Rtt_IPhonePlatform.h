//////////////////////////////////////////////////////////////////////////////
//
// This file is part of the Corona game engine.
// For overview and more information on licensing please refer to README.md 
// Home page: https://github.com/coronalabs/corona
// Contact: support@coronalabs.com
//
//////////////////////////////////////////////////////////////////////////////

#ifndef _Rtt_IPhonePlatform_H__
#define _Rtt_IPhonePlatform_H__

#include "Rtt_IPhonePlatformCore.h"

#include "Rtt_IPhoneDevice.h"

// ----------------------------------------------------------------------------

namespace Rtt
{

class AppleStoreProvider;

// ----------------------------------------------------------------------------

// This is the implementation of MPlatform for use by Corona/Native.
// (CoronaCards uses the parent class IPhonePlatformCore)
class IPhonePlatform : public IPhonePlatformCore
{
	Rtt_CLASS_NO_COPIES( IPhonePlatform )

	public:
		typedef IPhonePlatformCore Super;

	public:
		IPhonePlatform( CoronaView *view );
		virtual ~IPhonePlatform();

	public:
		virtual PlatformStoreProvider* GetStoreProvider( const ResourceHandle<lua_State>& handle ) const;

		virtual bool CanShowPopup( const char *name ) const;
		virtual bool ShowPopup( lua_State *L, const char *name, int optionsIndex ) const;
		virtual bool HidePopup( const char *name ) const;

	public:
		virtual void* CreateAndScheduleNotification( lua_State *L, int index ) const;
		virtual void ReleaseNotification( void *notificationId ) const;
		virtual void CancelNotification( void *notificationId ) const;

		void SetNativeProperty( lua_State *L, const char *key, int valueIndex ) const;
		int PushNativeProperty( lua_State *L, const char *key ) const;

	public:
		virtual void RuntimeErrorNotification( const char *errorType, const char *message, const char *stacktrace ) const;

	private:
		mutable AppleStoreProvider *fInAppStoreProvider;
		UIView *fActivityView;
		id fPopupControllerDelegate;
};

// ----------------------------------------------------------------------------

} // namespace Rtt

// ----------------------------------------------------------------------------

#endif // _Rtt_IPhonePlatform_H__
