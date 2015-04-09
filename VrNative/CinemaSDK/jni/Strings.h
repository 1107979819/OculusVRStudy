/************************************************************************************

Filename    :   Strings.h
Content     :	Text strings used by app.  Located in one place to make translation easier.
Created     :	9/30/2014
Authors     :   Jim Dosé

Copyright   :   Copyright 2014 Oculus VR, LLC. All Rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the Cinema/ directory. An additional grant 
of patent rights can be found in the PATENTS file in the same directory.

*************************************************************************************/

#if !defined( Strings_h )
#define Strings_h

namespace OculusCinema {

class Strings {
public:
	static const char * LoadingMenu_Title;

	static const char * Category_Trailers;
	static const char * Category_MyVideos;

	static const char * MovieSelection_Resume;
	static const char * MovieSelection_Next;

	static const char * ResumeMenu_Title;
	static const char * ResumeMenu_Resume;
	static const char * ResumeMenu_Restart;

	static const char * TheaterSelection_Title;

	static const char * Error_NoVideosOnPhone;
	static const char * Error_NoVideosInMyVideos;
	static const char * Error_UnableToPlayMovie;

	static const char * MoviePlayer_Reorient;
};

} // namespace OculusCinema

#endif // Strings_h
