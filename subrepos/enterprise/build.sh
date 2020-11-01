#!/bin/bash

# If stdout is the terminal, save a copy of the output to a log file
if [ -t 1 ]
then
	# Redirect stdout ( > ) into a named pipe ( >() ) running "tee"
	exec > >(tee "build-$(date +'%F-%T').log")
	# Copy stderr to stdout
	exec 2>&1
fi

set -x 

path=$(dirname "$0")

USER=$1
PASSWORD=$2
TARGET=$3

## OUTPUT_DIR

PRODUCT=CoronaEnterprise
BUILD_DIR=$path/build
OUTPUT_DIR=$path/build/$PRODUCT/

#
# Checks exit value for error
# 
checkError() {
    if [ $? -ne 0 ]
    then
        echo "Exiting due to errors (above)"
        exit -1
    fi
}

# 
# Extract tarball to create base dir structure
# 

if [ -e "$BUILD_DIR" ]
then
    rm -rf "$BUILD_DIR"
fi

mkdir -pv "$BUILD_DIR"
checkError

# 
# Canonicalize relative paths to absolute paths
# 
pushd "$path" > /dev/null
dir=$(pwd)
path=$dir
popd > /dev/null

pushd "$OUTPUT_DIR" > /dev/null
dir=$(pwd)
OUTPUT_DIR=$dir
popd > /dev/null


#
# Directories
# 

ROOT_DIR=$path/../..
BIN_MAC_DIR=$ROOT_DIR/bin/mac
BIN_WIN_DIR=$ROOT_DIR/bin/win
BIN_XCODE_DIR=$ROOT_DIR/bin/xcode
PLATFORM_DIR=$ROOT_DIR/platform
EXTERNAL_DIR=$ROOT_DIR/external
LUA_DIR=$EXTERNAL_DIR/lua-5.1.3/src
RTT_DIR=$ROOT_DIR/librtt

## Misc Dirs

CONTENTS_DIR=$path/contents
CORONA_DIR=$OUTPUT_DIR/Corona
CORONA_SHARED_DIR=$CORONA_DIR/shared
CORONA_SHARED_BIN_DIR=$CORONA_SHARED_DIR/bin
CORONA_SHARED_INCLUDE_CORONA_DIR=$CORONA_SHARED_DIR/include/Corona
CORONA_SHARED_INCLUDE_LUA_DIR=$CORONA_SHARED_DIR/include/lua
CORONA_SHARED_RESOURCE_DIR=$CORONA_SHARED_DIR/resource

mkdir -p $CORONA_DIR/android/lib/gradle
mkdir -p $CORONA_DIR/android/resource
mkdir -p $CORONA_DIR/ios/include/Corona
mkdir -p $CORONA_DIR/ios/lib
mkdir -p $CORONA_DIR/ios/resource
mkdir -p $CORONA_DIR/mac/bin
mkdir -p $CORONA_DIR/shared/bin
mkdir -p $CORONA_DIR/shared/include/Corona
mkdir -p $CORONA_DIR/shared/resource
mkdir -p $CORONA_DIR/win/bin
mkdir -p $CORONA_DIR/xcode

# 
# Shared
#

CONFIG=Release

## Corona/xcode
DST_XCODE_DIR=$CORONA_DIR/xcode

cp -v "$BIN_XCODE_DIR/universal-framework.sh" "$DST_XCODE_DIR"
checkError


cp -v "$BIN_XCODE_DIR/codesign-framework.sh" "$DST_XCODE_DIR"
checkError


## Corona/mac/bin
DST_BIN_MAC_DIR=$CORONA_DIR/mac/bin

BUILD_CORONABUILDER=YES
# If coronabuilder already exists in the build dir and we're on jenkins, then assume it was prebuilt by a different job.
# (this is important because Xcode will try to build it for the wrong OS or CPU arch unless things are lined up just right)
if [[ -d "$PLATFORM_DIR/mac/build/Release/CoronaBuilder.app" && -n "${WORKSPACE}" ]]
then
	BUILD_CORONABUILDER=NO
fi

# Build lua and luac first because Xcode seem to disregard dependency for code generation
xcodebuild -project "$PLATFORM_DIR/mac/ratatouille.xcodeproj" -scheme lua -configuration "$CONFIG"
checkError

xcodebuild -project "$PLATFORM_DIR/mac/ratatouille.xcodeproj" -scheme luac -configuration "$CONFIG"
checkError

if [[ "$BUILD_CORONABUILDER" == "YES" ]]
then
    xcodebuild -project "$PLATFORM_DIR/mac/CoronaBuilder.xcodeproj" -target CoronaBuilder -configuration "$CONFIG" clean
    checkError

	# Make the external CoronaCards dependency
    xcodebuild -project "$PLATFORM_DIR/mac/ratatouille.xcodeproj" -target CoronaCards -configuration "$CONFIG"
    checkError

    xcodebuild -project "$PLATFORM_DIR/mac/CoronaBuilder.xcodeproj" -target CoronaBuilder -configuration "$CONFIG"
    checkError
fi

mv -v "$PLATFORM_DIR/mac/build/Release/CoronaBuilder.app" "$DST_BIN_MAC_DIR"
checkError


# Record the version of CoronaSDK build this corresponds to in an easily accessible place
"$DST_BIN_MAC_DIR/CoronaBuilder.app/Contents/MacOS/CoronaBuilder" version > "$CORONA_DIR/BUILD" && chmod -w "$CORONA_DIR/BUILD"
checkError

cp -v "$ROOT_DIR/bin/mac/lfs.so" "$DST_BIN_MAC_DIR"
checkError

cp -v "$PLATFORM_DIR/mac/build/$CONFIG/lstrip" "$DST_BIN_MAC_DIR"
checkError

cp -v "$PLATFORM_DIR/mac/build/$CONFIG/lua" "$DST_BIN_MAC_DIR"
checkError

cp -v "$PLATFORM_DIR/mac/build/$CONFIG/luac" "$DST_BIN_MAC_DIR"
checkError

cp -v "$BIN_MAC_DIR/CopyResources.sh" "$DST_BIN_MAC_DIR"
checkError

cp -v "$BIN_MAC_DIR/CreateInfoPlist.sh" "$DST_BIN_MAC_DIR"
checkError

cp -v "$BIN_MAC_DIR/CertifyBuild.sh" "$DST_BIN_MAC_DIR"
checkError

cp -v "$BIN_MAC_DIR/buildSettingsToPlist.lua" "$DST_BIN_MAC_DIR"
checkError

cp -v "$BIN_MAC_DIR/relativePath.sh" "$DST_BIN_MAC_DIR"
checkError

cp -v "$BIN_MAC_DIR/lua2c.sh" "$DST_BIN_MAC_DIR"
checkError

## Corona/win/bin
DST_BIN_WIN_DIR=$CORONA_DIR/win/bin

cp -v "$BIN_WIN_DIR/lua2c.bat" "$DST_BIN_WIN_DIR"
checkError

cp -v "$BIN_WIN_DIR/lua.exe" "$DST_BIN_WIN_DIR"
checkError


## Corona/shared/include/lua
cp -v "$LUA_DIR/lua.h" "$CORONA_SHARED_INCLUDE_LUA_DIR"
checkError

cp -v "$LUA_DIR/luaconf.h" "$CORONA_SHARED_INCLUDE_LUA_DIR"
checkError

cp -v "$LUA_DIR/lualib.h" "$CORONA_SHARED_INCLUDE_LUA_DIR"
checkError

cp -v "$LUA_DIR/lauxlib.h" "$CORONA_SHARED_INCLUDE_LUA_DIR"
checkError

cp -v "$RTT_DIR/Corona/"*.h "$CORONA_SHARED_INCLUDE_CORONA_DIR"
checkError

## Corona/shared/bin
cp -v "$ROOT_DIR/bin/"*.lua "$CORONA_SHARED_BIN_DIR"
checkError

cp -v "$ROOT_DIR/bin/shared/"*.lua "$CORONA_SHARED_BIN_DIR"
checkError

cp -rv "$ROOT_DIR/bin/shared/"pl "$CORONA_SHARED_BIN_DIR"
checkError

## Corona/shared/resource
cp -v "$ROOT_DIR/sdk/dmg/Corona3rdPartyLicenses.txt" "$CORONA_SHARED_RESOURCE_DIR"
ln -s "Corona3rdPartyLicenses.txt" "$CORONA_SHARED_RESOURCE_DIR/CoronaSDK3rdPartyLicenses.txt"
checkError
cp -v "$ROOT_DIR/platform/resources/json.lua" "$CORONA_SHARED_RESOURCE_DIR"
checkError
cp -v "$ROOT_DIR/platform/resources/dkjson.lua" "$CORONA_SHARED_RESOURCE_DIR"
checkError
cp -v "$ROOT_DIR/platform/resources/CoronaPListSupport.lua" "$CORONA_SHARED_RESOURCE_DIR"
checkError

# 
# TVOS
#

if [ -z "${JOB_NAME}" ]
then
    export JOB_NAME=Enterprise
fi

"$PLATFORM_DIR/tvos/build_templates.sh"
checkError


# 
# iOS
#

"$PLATFORM_DIR/iphone/enterprise/build.sh" "$USER" "$PASSWORD" "$OUTPUT_DIR" "$TARGET"
checkError


# 
# Android
# 
(
    set -e
    if [ ! -f "$PLATFORM_DIR/android/gradlew" ]
    then
        exit 0
    fi
    echo Building Gradle artifacts!

    cd "$PLATFORM_DIR/android"
    ./gradlew clean
    ./gradlew installAppTemplateAndAARToSim -PcoronaNativeOutputDir="$CORONA_DIR"
)
checkError

# 
# docs
# 
#cp -v docs/style.css $OUTPUT_DIR/.
#cp -v docs/README.html $OUTPUT_DIR/.

#
# zip it up
# 
pushd "$BUILD_DIR"
# the COPYFILE_DISABLE=1 stops tar saving HFS file attributes
# and we exclude cruft autogenerated by Xcode
COPYFILE_DISABLE=1 tar --exclude '*/project.xcworkspace*' --exclude '*/xcuserdata*' -czvf $PRODUCT.tgz $PRODUCT
popd > /dev/null

