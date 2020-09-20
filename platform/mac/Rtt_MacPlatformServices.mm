//////////////////////////////////////////////////////////////////////////////
//
// This file is part of the Corona game engine.
// For overview and more information on licensing please refer to README.md 
// Home page: https://github.com/coronalabs/corona
// Contact: support@coronalabs.com
//
//////////////////////////////////////////////////////////////////////////////

#include "Core/Rtt_Build.h"

#include "Rtt_MacPlatformServices.h"

#import <CoreFoundation/CoreFoundation.h>
#import <Foundation/Foundation.h>
#import <SystemConfiguration/SystemConfiguration.h>
#import <netinet/in.h>

// ----------------------------------------------------------------------------

namespace Rtt
{

// ----------------------------------------------------------------------------

MacPlatformServices::MacPlatformServices( const MPlatform& platform )
:	fPlatform( platform )
{
}

const MPlatform&
MacPlatformServices::Platform() const
{
	return fPlatform;
}

#define Rtt_CORONA_DOMAIN "com.coronalabs.Corona_Simulator" // "com.anscamobile.ratatouille"
static const char kCoronaDomainUTF8[] = Rtt_CORONA_DOMAIN;
static CFStringRef kCoronaDomain = CFSTR( Rtt_CORONA_DOMAIN );
#undef Rtt_CORONA_DOMAIN

void
MacPlatformServices::GetPreference( const char *key, Rtt::String * value ) const
{
	const char *result = NULL;
	NSString *k = [[NSString alloc] initWithUTF8String:key];

	// A little strange, but here's the backstory:
	// Corona started with using CFPreferences and an explict custom app domain that differed 
	// than what you would normally get with NSUserDefaults.
	// Eventually NSUserDefaults started being used which obviously mapped to a different preference file.
	// As kind of a nice side-effect-quirk though, some of the nasty preferences like username/email/authentication
	// checks go to the CFPreference while mostly UI stuff goes to NSUserDefaults. This is convenient for people who
	// like to blow-away non-important settings (delete NSUserDefaults) and start over without breaking their app
	// which actually conforms to desired Apple-UI guidelines.
	// But to deal with Windows, more settings are being pushed though the Get/SetPreferences API.
	// But the API is not aware of this quirk we currently have in the code.
	// For now, I am going to case the settings so that legacy-non-throw-away settings go through CFPrefernences,
	// and everything else goes through NSUserDefaults.


	// Try NSUserDefaults first
	NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];
	NSString *v = [defaults stringForKey:k]; Rtt_ASSERT( !v || [v isKindOfClass:[NSString class]] );
	if( nil != v)
	{
		result = [v UTF8String];
	}
	else // fallback to CFPreferences
	{
		// TODO: Figure out how to do this on a suite domain
		//	CFPropertyListRef v = CFPreferencesCopyValue(
		//		(CFStringRef)k, kCoronaDomain, kCFPreferencesCurrentUser, kCFPreferencesCurrentHost );
		CFPropertyListRef v = CFPreferencesCopyAppValue( (CFStringRef)k, kCoronaDomain );
		if ( v )
		{
			if ( CFStringGetTypeID() == CFGetTypeID( v ) )
			{
				result = [(NSString*)v UTF8String];
			}
			
			CFRelease( v );
		}
	}

	[k release];

	value->Set( result );
}

void
MacPlatformServices::SetPreference( const char *key, const char *value ) const
{
	// A little strange, but here's the backstory:
	// Corona started with using CFPreferences and an explict custom app domain that differed 
	// than what you would normally get with NSUserDefaults.
	// Eventually NSUserDefaults started being used which obviously mapped to a different preference file.
	// As kind of a nice side-effect-quirk though, some of the nasty preferences like username/email/authentication
	// checks go to the CFPreference while mostly UI stuff goes to NSUserDefaults. This is convenient for people who
	// like to blow-away non-important settings (delete NSUserDefaults) and start over without breaking their app
	// which actually conforms to desired Apple-UI guidelines.
	// But to deal with Windows, more settings are being pushed though the Get/SetPreferences API.
	// But the API is not aware of this quirk we currently have in the code.
	// For now, I am going to case the settings so that legacy-non-throw-away settings go through CFPrefernences,
	// and everything else goes through NSUserDefaults.
	
	if ( Rtt_VERIFY( key ) )
	{

		NSString *k = [[NSString alloc] initWithUTF8String:key];
		// TODO: Figure out how to do this on a suite domain
		NSString *v = ( value ? [[NSString alloc] initWithUTF8String:value] : nil );

		CFPreferencesSetAppValue( (CFStringRef)k, (CFPropertyListRef)v, kCoronaDomain );
		(void)Rtt_VERIFY( CFPreferencesAppSynchronize( kCoronaDomain ) );
		
		[v release];
		[k release];
	}
}

void
MacPlatformServices::GetSecurePreference( const char *key, Rtt::String * value ) const
{
	UInt32 pwdLen = 0;
	void *pwd = NULL;

	NSString *tmp = nil;
	if ( noErr == SecKeychainFindGenericPassword(
							NULL, (UInt32) strlen( kCoronaDomainUTF8 ), kCoronaDomainUTF8,
							(UInt32) strlen( key ), key, & pwdLen, & pwd, NULL ) )
	{
		tmp = [[NSString alloc] initWithBytes:pwd length:pwdLen encoding:NSUTF8StringEncoding];
		(void)Rtt_VERIFY( noErr == SecKeychainItemFreeContent( NULL, pwd ) );
	}

	value->Set( [tmp UTF8String] );
	[tmp release];
}

bool
MacPlatformServices::SetSecurePreference( const char *key, const char *value ) const
{
	bool result = false;

	SecKeychainItemRef item = NULL;

	UInt32 keyLen = (UInt32) strlen( key );

	if ( errSecItemNotFound == SecKeychainFindGenericPassword(
							NULL, (UInt32) strlen( kCoronaDomainUTF8 ), kCoronaDomainUTF8,
							keyLen, key, NULL, NULL, & item ) )
	{
		// No item found, so add the item (provided value is not NULL)
		if ( value )
		{
			// Add password
			result = ( noErr == SecKeychainAddGenericPassword(
									NULL, (UInt32) strlen( kCoronaDomainUTF8 ), kCoronaDomainUTF8,
									keyLen, key, (UInt32) strlen( value ), value, NULL ) );
		}
		else
		{
			result = true;
		}
	}
	else
	{
		Rtt_ASSERT( item );
		if ( ! value )
		{
			// Remove password
			result = ( noErr == SecKeychainItemDelete( item ) );
		}
		else
		{
			Rtt_ASSERT( item );

			// Modify password
			result = ( noErr == SecKeychainItemModifyAttributesAndData( item, NULL, (UInt32) strlen( value ), value ) );
		}

		CFRelease( item );
	}

	return result;
}

static SCNetworkReachabilityRef
CreateWithAddress( const struct sockaddr_in* hostAddress )
{
	SCNetworkReachabilityRef result =
		SCNetworkReachabilityCreateWithAddress( NULL, (const struct sockaddr*)hostAddress );

	return result;
}

static bool
IsHostReachable( SCNetworkReachabilityRef reachability )
{
	Rtt_ASSERT( reachability );

	bool result = false;

	SCNetworkConnectionFlags flags;
	if ( SCNetworkReachabilityGetFlags( reachability, & flags ) )
	{
		if ( (flags & kSCNetworkFlagsReachable ) == 0 )
		{
			// if target host is not reachable
			// return NotReachable;
			goto exit_gracefully;
		}
		
		if ( (flags & kSCNetworkFlagsConnectionRequired) == 0 )
		{
			// if target host is reachable and no connection is required
			//  then we'll assume (for now) that your on Wi-Fi
			// retVal = ReachableViaWiFi;
			result = true;
			goto exit_gracefully;
		}
		
		
		if ( (flags & kSCNetworkFlagsConnectionAutomatic) != 0 )
		{
			// ... and the connection is on-demand (or on-traffic) if the
			//     calling application is using the CFSocketStream or higher APIs
			if ((flags & kSCNetworkFlagsInterventionRequired) == 0)
			{
				// ... and no [user] intervention is needed
				// retVal = ReachableViaWiFi;
				result = true;
				goto exit_gracefully;
			}
		}
		/*
		if ((flags & kSCNetworkReachabilityFlagsIsWWAN) == kSCNetworkReachabilityFlagsIsWWAN)
		{
			// ... but WWAN connections are OK if the calling application
			//     is using the CFNetwork (CFSocketStream?) APIs.
			// retVal = ReachableViaWWAN;
			result = true;
			goto exit_gracefully;
		}
		*/
	}

exit_gracefully:
	return result;
}

bool
MacPlatformServices::IsInternetAvailable() const
{
	bool result = false;

	SCNetworkReachabilityRef reachability = SCNetworkReachabilityCreateWithName(NULL, "coronalabs.com");

	if ( reachability )
	{
		result = IsHostReachable( reachability );
		CFRelease( reachability );
	}


	return result;
}

bool
MacPlatformServices::IsLocalWifiAvailable() const
{
	bool result = false;

	struct sockaddr_in localWifiAddress;
	bzero(&localWifiAddress, sizeof(localWifiAddress));
	localWifiAddress.sin_len = sizeof(localWifiAddress);
	localWifiAddress.sin_family = AF_INET;
	// IN_LINKLOCALNETNUM is defined in <netinet/in.h> as 169.254.0.0
	localWifiAddress.sin_addr.s_addr = htonl(IN_LINKLOCALNETNUM);

	SCNetworkReachabilityRef reachability = CreateWithAddress( & localWifiAddress );
	if ( reachability )
	{
		SCNetworkConnectionFlags flags;
		if ( SCNetworkReachabilityGetFlags( reachability, & flags ) )
		{
			result = (flags & kSCNetworkFlagsReachable) && (flags & kSCNetworkFlagsIsDirect);
		}

		CFRelease( reachability );
	}

	return result;
}

void
MacPlatformServices::Terminate() const
{
#ifdef Rtt_STDLIB_EXIT
	exit( 0 );
#else
	NSApplication *application = [NSApplication sharedApplication];
	[application terminate:application];
#endif
}

void
MacPlatformServices::Sleep( int milliseconds ) const
{
	usleep( milliseconds*1000 );
}

// ----------------------------------------------------------------------------

} // namespace Rtt

// ----------------------------------------------------------------------------

