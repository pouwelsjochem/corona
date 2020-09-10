//////////////////////////////////////////////////////////////////////////////
//
// This file is part of the Corona game engine.
// For overview and more information on licensing please refer to README.md 
// Home page: https://github.com/coronalabs/corona
// Contact: support@coronalabs.com
//
//////////////////////////////////////////////////////////////////////////////

#include "Core/Rtt_Build.h"

#include "Core/Rtt_String.h"

#include "Rtt_IPhonePlatformCore.h"
#include "Rtt_IPhoneTimer.h"

#include "Rtt_IPhoneFont.h"
#include "Rtt_IPhoneScreenSurface.h"

#include "Rtt_LuaLibSystem.h"
//#include "Rtt_LuaResource.h"
#include "Rtt_AppleFont.h"

#include "Rtt_TouchInhibitor.h"

#import "CoronaCards/CoronaView.h"

// ----------------------------------------------------------------------------

namespace Rtt
{

// ----------------------------------------------------------------------------

IPhonePlatformCore::IPhonePlatformCore( CoronaView *view )
:	Super( view ),
	fDevice( GetAllocator(), view )
{
}

IPhonePlatformCore::~IPhonePlatformCore()
{
}

MPlatformDevice&
IPhonePlatformCore::GetDevice() const
{
	return const_cast< IPhoneDevice& >( fDevice );
}

static Rtt_INLINE
double DegreesToRadians( double degrees )
{
	return degrees * M_PI/180;
}

bool
IPhonePlatformCore::SaveBitmap( PlatformBitmap* bitmap, NSString* filePath ) const
{
	Rtt_ASSERT( bitmap );
	PlatformBitmap::Orientation orientation = bitmap->GetOrientation();
	bool isSideways = PlatformBitmap::kLeft == orientation || PlatformBitmap::kRight == orientation;

	const void* buffer = bitmap->GetBits( & GetAllocator() );
	size_t w = bitmap->Width();
	size_t h = bitmap->Height();
	size_t wDst = w;
	size_t hDst = h;
	if ( isSideways )
	{
		Swap( wDst, hDst );
	}

	size_t bytesPerPixel = PlatformBitmap::BytesPerPixel( bitmap->GetFormat() );
	size_t bytesPerRow = w*bytesPerPixel;
	NSInteger numBytes = h*bytesPerRow;

	CGBitmapInfo srcBitmapInfo = CGBitmapInfo(kCGBitmapByteOrderDefault | kCGImageAlphaLast)
	CGBitmapInfo dstBitmapInfo = kCGImageAlphaPremultipliedLast;

	CGDataProviderRef dataProvider = CGDataProviderCreateWithData( NULL, buffer, numBytes, NULL );
	CGColorSpaceRef colorspace = CGColorSpaceCreateDeviceRGB();
	CGImageRef imageRef = CGImageCreate(w, h, 8, 32, w*bytesPerPixel,
                                        colorspace, srcBitmapInfo, dataProvider,
                                        NULL, true, kCGRenderingIntentDefault);
    

	Rtt_ASSERT( w == CGImageGetWidth( imageRef ) );
	Rtt_ASSERT( h == CGImageGetHeight( imageRef ) );
    
    
	//void* pixels = calloc( bytesPerRow, h );
	CGContextRef context = CGBitmapContextCreate(NULL, wDst, hDst, 8, wDst*bytesPerPixel, colorspace, dstBitmapInfo);

	// On iPhone, when the image is sideways, we have to rotate the bits b/c when 
	// we read them in using glReadPixels, the window buffer is physically oriented 
	// as upright, so glReadPixels returns them assuming the buffer is physically
	// oriented upright, rather than sideways.
	if ( isSideways )
	{
		S32 angle = - ( bitmap->DegreesToUprightBits() );
		CGFloat dx = (CGFloat)wDst;
		CGFloat dy = (CGFloat)hDst;
		if ( 90 == angle )
		{
			dy = 0.f;
		}
		if ( -90 == angle )
		{
			dx = 0.f;
		}

		CGContextTranslateCTM( context, dx, dy );
		CGContextRotateCTM( context, DegreesToRadians( angle ) );
	}
	else if ( PlatformBitmap::kDown == orientation )
	{
		CGContextTranslateCTM( context, wDst, hDst );
		CGContextRotateCTM( context, DegreesToRadians( 180 ) );
	}

	CGContextDrawImage( context, CGRectMake( 0.0, 0.0, w, h ), imageRef );
	CGImageRef flippedImageRef = CGBitmapContextCreateImage(context);
	UIImage* image = [[UIImage alloc] initWithCGImage:flippedImageRef];

	if ( ! filePath )
	{
		UIImageWriteToSavedPhotosAlbum( image, nil, nil, nil );
	}
	else
	{
		NSData *imageData = UIImagePNGRepresentation( image );
        [imageData writeToFile:filePath atomically:YES];
	}

	[image release];
	CGImageRelease( flippedImageRef );
	CGColorSpaceRelease( colorspace );
	CGContextRelease( context );
	//free( pixels );


//	UIImage* image = [[UIImage alloc] initWithCGImage:imageRef];
//	UIImageWriteToSavedPhotosAlbum( image, nil, nil, nil );
//	[image release];

	CGImageRelease( imageRef );
	CGDataProviderRelease( dataProvider );
	bitmap->FreeBits();

	return true;
}

bool
IPhonePlatformCore::SaveBitmap( PlatformBitmap* bitmap, const char* filePath ) const
{
	return SaveBitmap( bitmap, [NSString stringWithUTF8String:filePath] );
}

Real
IPhonePlatformCore::GetStandardFontSize() const
{
	// The system's default label font size matches UITextField's font size, which is a point size of 17.
	return [AppleFont labelFontSize] * GetView().contentScaleFactor;
}

// This is a place where we can add system.getInfo() categories that return arbitrary types
// (it also allows us to add categories on a per platform basis)
int
IPhonePlatformCore::PushSystemInfo( lua_State *L, const char *key ) const
{
	// Validate.
	if (!L)
	{
		return 0;
	}

	// Push the requested system information to Lua.
	int pushedValues = 0;
	pushedValues = Super::PushSystemInfo(L, key);

	// Return the number of values pushed into Lua.
	return pushedValues;
}

void
IPhonePlatformCore::RegisterUserNotificationSettings(int type) const
{
#ifdef Rtt_DEBUG
	// Make sure our constants are the same as whats provided by apple.
	Rtt_ASSERT( Rtt_UIUserNotificationTypeNone == UIUserNotificationTypeNone );
	Rtt_ASSERT( Rtt_UIUserNotificationTypeBadge == UIUserNotificationTypeBadge );
	Rtt_ASSERT( Rtt_UIUserNotificationTypeSound == UIUserNotificationTypeSound );
	Rtt_ASSERT( Rtt_UIUserNotificationTypeAlert == UIUserNotificationTypeAlert );
#endif
	
	// These functions were added in iOS 8 and have to be called or notifications won't work correctly.  Even though this part if just for setting notifications, we're asking for the other permissions because
	// once you call this function once it won't ask again when you call it again with new permissions.  In the alert dialog thats shown, it doesn't specify which permissions are asked for
	// anyways.
	Class cls = NSClassFromString(@"UIUserNotificationSettings");
	if ( cls )
	{
		id settings = [cls settingsForTypes:type categories:nil];
		[[UIApplication sharedApplication] registerUserNotificationSettings:settings];
	}
}
	
void
IPhonePlatformCore::RegisterUserNotificationSettings() const
{
	RegisterUserNotificationSettings(Rtt_UIUserNotificationTypeAlert|Rtt_UIUserNotificationTypeBadge|Rtt_UIUserNotificationTypeSound);
}

#if 0
static bool
IsNativeKeySupported( NSString *key )
{
	NSNumber *value = [NSNumber numberWithBool:YES];
	NSDictionary *keys = [NSDictionary dictionaryWithObjectsAndKeys:
		value, @"applicationIconBadgeNumber", 
		value, @"applicationSupportsShakeToEdit",
		value, @"networkActivityIndicatorVisible",
		nil];

	bool result = ( nil != [keys valueForKey:key] );
	return result;
}
#endif

void
IPhonePlatformCore::SetNativeProperty( lua_State *L, const char *key, int valueIndex ) const
{
	UIApplication *app = [UIApplication sharedApplication];
	NSString *k = [NSString stringWithUTF8String:key];

	if ( [k isEqualToString:@"applicationIconBadgeNumber"] )
	{
		RegisterUserNotificationSettings();
		app.applicationIconBadgeNumber = lua_tointeger( L, valueIndex );
	}
	else if ( [k isEqualToString:@"applicationSupportsShakeToEdit"] )
	{
		app.applicationSupportsShakeToEdit = lua_toboolean( L, valueIndex );
	}
	else if ( [k isEqualToString:@"networkActivityIndicatorVisible"] )
	{
		app.networkActivityIndicatorVisible = lua_toboolean( L, valueIndex );
	}
}

int
IPhonePlatformCore::PushNativeProperty( lua_State *L, const char *key ) const
{
	int result = 1;

	UIApplication *app = [UIApplication sharedApplication];
	NSString *k = [NSString stringWithUTF8String:key];

	// NOTE: attempted to use KVC (valueForKey: but this failed b/c boolean properties
	// were being returned as ints in the returned NSNumber object
	if ( [k isEqualToString:@"applicationIconBadgeNumber"] )
	{
		lua_pushinteger( L, app.applicationIconBadgeNumber );
	}
	else if ( [k isEqualToString:@"applicationSupportsShakeToEdit"] )
	{
		lua_pushboolean( L, app.applicationSupportsShakeToEdit );
	}
	else if ( [k isEqualToString:@"networkActivityIndicatorVisible"] )
	{
		lua_pushboolean( L, app.networkActivityIndicatorVisible );
	}
	else
	{
		Rtt_ASSERT_NOT_IMPLEMENTED();
		result = 0;
	}

	return result;
}

// ----------------------------------------------------------------------------

} // namespace Rtt

// ----------------------------------------------------------------------------

