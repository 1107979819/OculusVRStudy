/************************************************************************************

Filename    :   Native.h
Content     :
Created     :	8/8/2014
Authors     :   Jim Dosé

Copyright   :   Copyright 2014 Oculus VR, LLC. All Rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the Cinema/ directory. An additional grant 
of patent rights can be found in the PATENTS file in the same directory.

*************************************************************************************/

#if !defined( Native_h )
#define Native_h

namespace OculusCinema {

class Native {
public:
	static void			OneTimeInit( App *app, jclass mainActivityClass );
	static void			OneTimeShutdown();
	static void 		ShowUI( App *app );
	static void 		HideUI( App *app );
	static void			TogglePlaying( App *app );
	static bool 		CreateVideoThumbnail( App *app, const char *videoFilePath, const char *outputFilePath, const int width, const int height );
	static bool			CheckForMovieResume( App *app, const char * movieName );
	static void 		StartMovie( App *app, const char * movieName, const char *displayName, bool resumePlayback, bool isEncrypted );
	static void 		StopMovie( App *app );
	static void 		PauseMovie( App *app );
	static void 		ResumeMovie( App *app );
	static int 			GetPosition( App *app );
	static void 		SetPosition( App *app, int positionMS );
	static void 		SeekDelta( App *app, int deltaMS );
	static bool 		IsPlaybackFinished( App *app );
	static bool 		HadPlaybackError( App *app );
	static int	 		GazeCursor( App *app, const float x, const float y, const bool press, const bool release, const int seekSpeed );
	static bool 		GetPlaybackControlsRect( App *app, int & left, int & bottom, int & right, int & top );
};

} // namespace OculusCinema

#endif // Native_h
