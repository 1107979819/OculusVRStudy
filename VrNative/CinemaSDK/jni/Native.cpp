/************************************************************************************

Filename    :   Native.cpp
Content     :
Created     :	6/20/2014
Authors     :   Jim Dosé

Copyright   :   Copyright 2014 Oculus VR, LLC. All Rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the Cinema/ directory. An additional grant 
of patent rights can be found in the PATENTS file in the same directory.

*************************************************************************************/

#include "CinemaApp.h"
#include "Native.h"


namespace OculusCinema
{

extern "C" {

long Java_com_oculus_cinemasdk_MainActivity_nativeSetAppInterface(
		JNIEnv *jni, jclass clazz, jobject activity )
{
	LOG( "nativeSetAppInterface");
	return (new CinemaApp())->SetActivity( jni, clazz, activity );
}


void Java_com_oculus_cinemasdk_MainActivity_nativeSetParms( JNIEnv *jni, jclass clazz, jlong interfacePtr, float screenDist ) {

	CinemaApp *cinema = ( CinemaApp * )( ( (App *)interfacePtr )->GetAppInterface() );
	cinema->app->GetMessageQueue().PostPrintf( "parms %f", screenDist );
}

void Java_com_oculus_cinemasdk_MainActivity_nativeSetVideoSize( JNIEnv *jni, jclass clazz, jlong interfacePtr, int width, int height, int rotation ) {
	LOG( "nativeSetVideoSizes: width=%i height=%i rotation=%i", width, height, rotation );

	CinemaApp *cinema = ( CinemaApp * )( ( (App *)interfacePtr )->GetAppInterface() );
	cinema->app->GetMessageQueue().PostPrintf( "video %i %i %i", width, height, rotation );
}

jobject Java_com_oculus_cinemasdk_MainActivity_nativePrepareNewVideo( JNIEnv *jni, jclass clazz, jlong interfacePtr ) {
	CinemaApp *cinema = ( CinemaApp * )( ( (App *)interfacePtr )->GetAppInterface() );

	// set up a message queue to get the return message
	// TODO: make a class that encapsulates this work
	MessageQueue	result(1);
	cinema->app->GetMessageQueue().PostPrintf( "newVideo %p", &result);

	result.SleepUntilMessage();
	const char * msg = result.GetNextMessage();
	jobject	texobj;
	sscanf( msg, "surfaceTexture %p", &texobj );
	free( (void *)msg );

	return texobj;
}

float Java_com_oculus_cinemasdk_MainActivity_nativeGetScreenDist( JNIEnv *jni, jclass clazz, jlong interfacePtr ) {
	// Screen dist can be updated in the loop continuously, so it
	// must be queried back for preference storage.
	CinemaApp *cinema = ( CinemaApp * )( ( (App *)interfacePtr )->GetAppInterface() );
	return cinema->SceneMgr.FreeScreenScale;
}

}	// extern "C"

//==============================================================

// Java method ids
static jmethodID 	togglePlayingMethodId = NULL;
static jmethodID	createVideoThumbnailMethodId = NULL;
static jmethodID	checkForMovieResumeId = NULL;
static jmethodID 	startMovieMethodId = NULL;
static jmethodID 	stopMovieMethodId = NULL;
static jmethodID 	pauseMovieMethodId = NULL;
static jmethodID 	resumeMovieMethodId = NULL;
static jmethodID	setPositionMethodId = NULL;
static jmethodID	getPositionMethodId = NULL;
static jmethodID 	seekDeltaMethodId = NULL;
static jmethodID 	showUIMethodId = NULL;
static jmethodID 	hideUIMethodId = NULL;
static jmethodID 	isPlaybackFinishedMethodId = NULL;
static jmethodID 	hadPlaybackErrorMethodId = NULL;
static jmethodID	gazeCursorMethodId = NULL;
static jmethodID	getPlaybackControlsRectMethodId = NULL;

// Error checks and exits on failure
static jmethodID GetMethodID( App *app, jclass cls, const char * name, const char * signature )
{
	jmethodID mid = app->GetVrJni()->GetMethodID( cls, name, signature );
	if ( !mid )
	{
    	FAIL( "Couldn't find %s methodID", name );
    }

	return mid;
}

void Native::OneTimeInit( App *app, jclass mainActivityClass )
{
	LOG( "Native::OneTimeInit" );

	const double start = TimeInSeconds();

	togglePlayingMethodId 			= GetMethodID( app, mainActivityClass, "togglePlaying", "()V" );
	createVideoThumbnailMethodId 	= GetMethodID( app, mainActivityClass, "createVideoThumbnail", "(Ljava/lang/String;Ljava/lang/String;II)Z" );
	checkForMovieResumeId 			= GetMethodID( app, mainActivityClass, "checkForMovieResume", "(Ljava/lang/String;)Z" );
	startMovieMethodId 				= GetMethodID( app, mainActivityClass, "startMovieFromNative", "(Ljava/lang/String;Ljava/lang/String;ZZ)V" );
	stopMovieMethodId 				= GetMethodID( app, mainActivityClass, "stopMovie", "()V" );
	pauseMovieMethodId 				= GetMethodID( app, mainActivityClass, "pauseMovie", "()V" );
	resumeMovieMethodId 			= GetMethodID( app, mainActivityClass, "resumeMovie", "()V" );
	setPositionMethodId 			= GetMethodID( app, mainActivityClass, "setPosition", "(I)V" );
	getPositionMethodId 			= GetMethodID( app, mainActivityClass, "getPosition", "()I" );
	seekDeltaMethodId				= GetMethodID( app, mainActivityClass, "seekDelta", "(I)V" );
	showUIMethodId 					= GetMethodID( app, mainActivityClass, "showUI", "()V" );
	hideUIMethodId 					= GetMethodID( app, mainActivityClass, "hideUI", "()V" );
	isPlaybackFinishedMethodId		= GetMethodID( app, mainActivityClass, "isPlaybackFinished", "()Z" );
	hadPlaybackErrorMethodId		= GetMethodID( app, mainActivityClass, "hadPlaybackError", "()Z" );
	gazeCursorMethodId				= GetMethodID( app, mainActivityClass, "gazeCursor", "(FFZZI)I" );
	getPlaybackControlsRectMethodId	= GetMethodID( app, mainActivityClass, "getPlaybackControlsRect", "()[I" );

	LOG( "Native::OneTimeInit: %3.1f seconds", TimeInSeconds() - start );
}

void Native::OneTimeShutdown()
{
	LOG( "Native::OneTimeShutdown" );
}

void Native::TogglePlaying( App *app )
{
	LOG( "TogglePlaying()" );
	app->GetVrJni()->CallVoidMethod( app->GetJavaObject(), togglePlayingMethodId );
}

bool Native::CreateVideoThumbnail( App *app, const char *videoFilePath, const char *outputFilePath, const int width, const int height )
{
	LOG( "CreateVideoThumbnail( %s, %s )", videoFilePath, outputFilePath );

	jstring jstrVideoFilePath = app->GetVrJni()->NewStringUTF( videoFilePath );
	jstring jstrOutputFilePath = app->GetVrJni()->NewStringUTF( outputFilePath );

	jboolean result = app->GetVrJni()->CallBooleanMethod( app->GetJavaObject(), createVideoThumbnailMethodId, jstrVideoFilePath, jstrOutputFilePath, width, height );

	app->GetVrJni()->DeleteLocalRef( jstrVideoFilePath );
	app->GetVrJni()->DeleteLocalRef( jstrOutputFilePath );

	LOG( "CreateVideoThumbnail( %s, %s )", videoFilePath, outputFilePath );

	return result;
}

bool Native::CheckForMovieResume( App *app, const char * movieName )
{
	LOG( "CheckForMovieResume( %s )", movieName );

	jstring jstrMovieName = app->GetVrJni()->NewStringUTF( movieName );

	jboolean result = app->GetVrJni()->CallBooleanMethod( app->GetJavaObject(), checkForMovieResumeId, jstrMovieName );

	app->GetVrJni()->DeleteLocalRef( jstrMovieName );

	return result;
}

void Native::StartMovie( App *app, const char * movieName, const char * displayName, bool resumePlayback, bool isEncrypted )
{
	LOG( "StartMovie( %s )", movieName );

	jstring jstrMovieName = app->GetVrJni()->NewStringUTF( movieName );
	jstring jstrDisplayName = app->GetVrJni()->NewStringUTF( displayName );

	app->GetVrJni()->CallVoidMethod( app->GetJavaObject(), startMovieMethodId, jstrMovieName, jstrDisplayName, resumePlayback, isEncrypted );

	app->GetVrJni()->DeleteLocalRef( jstrMovieName );
	app->GetVrJni()->DeleteLocalRef( jstrDisplayName );
}

void Native::StopMovie( App *app )
{
	LOG( "StopMovie()" );
	app->GetVrJni()->CallVoidMethod( app->GetJavaObject(), stopMovieMethodId );
}

void Native::PauseMovie( App *app )
{
	LOG( "PauseMovie()" );
	app->GetVrJni()->CallVoidMethod( app->GetJavaObject(), pauseMovieMethodId );
}

void Native::ResumeMovie( App *app )
{
	LOG( "ResumeMovie()" );
	app->GetVrJni()->CallVoidMethod( app->GetJavaObject(), resumeMovieMethodId );
}

int Native::GetPosition( App *app )
{
	LOG( "GetPosition()" );
	return app->GetVrJni()->CallIntMethod( app->GetJavaObject(), getPositionMethodId );
}

void Native::SetPosition( App *app, int positionMS )
{
	LOG( "SetPosition()" );
	app->GetVrJni()->CallVoidMethod( app->GetJavaObject(), setPositionMethodId, positionMS );
}

void Native::SeekDelta( App *app, int deltaMS )
{
	LOG( "SeekDelta()" );
	app->GetVrJni()->CallVoidMethod( app->GetJavaObject(), seekDeltaMethodId, deltaMS );
}

void Native::ShowUI( App *app )
{
	LOG( "ShowUI()" );
	app->GetVrJni()->CallVoidMethod( app->GetJavaObject(), showUIMethodId );
}

void Native::HideUI( App *app )
{
	LOG( "HideUI()" );
	app->GetVrJni()->CallVoidMethod( app->GetJavaObject(), hideUIMethodId );
}

bool Native::IsPlaybackFinished( App *app )
{
	jboolean result = app->GetVrJni()->CallBooleanMethod( app->GetJavaObject(), isPlaybackFinishedMethodId );
	return ( result != 0 );
}

bool Native::HadPlaybackError( App *app )
{
	jboolean result = app->GetVrJni()->CallBooleanMethod( app->GetJavaObject(), hadPlaybackErrorMethodId );
	return ( result != 0 );
}

int Native::GazeCursor( App *app, const float x, const float y, const bool press, const bool release, const int seekSpeed )
{
	return app->GetVrJni()->CallIntMethod( app->GetJavaObject(), gazeCursorMethodId,
		x, y, press, release, seekSpeed );
}

bool Native::GetPlaybackControlsRect( App *app, int & left, int & bottom, int & right, int & top )
{
	jintArray retArray = ( jintArray )app->GetVrJni()->CallObjectMethod( app->GetJavaObject(), getPlaybackControlsRectMethodId );

	jsize len = app->GetVrJni()->GetArrayLength( retArray );
	if ( len != 4 )
	{
		FAIL( "getPlaybackControlsRect returned %d values", len );
		return false;
	}

	jint *values = app->GetVrJni()->GetIntArrayElements( retArray, NULL );

	left = values[ 0 ];
	bottom = values[ 1 ];
	right = values[ 2 ];
	top = values[ 3 ];

	app->GetVrJni()->ReleaseIntArrayElements( retArray, values, 0 );
	app->GetVrJni()->DeleteLocalRef( retArray );

	// if all negative then it's not fully initialized yet
	if ( ( left == -1 ) && ( bottom == -1 ) && ( right == -1 ) && ( top == -1 ) )
	{
		return false;
	}

	return true;
}

} // namespace OculusCinema
