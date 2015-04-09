/************************************************************************************

Filename    :   Strings.cpp
Content     :	Text strings used by app.  Located in one place to make translation easier.
Created     :	9/30/2014
Authors     :   Jim Dosé

Copyright   :   Copyright 2014 Oculus VR, LLC. All Rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the Cinema/ directory. An additional grant 
of patent rights can be found in the PATENTS file in the same directory.

*************************************************************************************/

#include "Strings.h"


namespace OculusCinema
{
const char * Strings::LoadingMenu_Title				= "Loading movies.\nPlease wait.";

const char * Strings::Category_Trailers 			= "Trailers";
const char * Strings::Category_MyVideos 			= "My Videos";

const char * Strings::MovieSelection_Resume			= "Resume";
const char * Strings::MovieSelection_Next			= "Next";

const char * Strings::ResumeMenu_Title				= "What would you like to do?";
const char * Strings::ResumeMenu_Resume				= "Resume";
const char * Strings::ResumeMenu_Restart			= "Restart";

const char * Strings::TheaterSelection_Title		= "SELECT THEATER";

const char * Strings::Error_NoVideosOnPhone 		= "There are no videos on your phone. \nPlease insert the SD card included with your Gear VR.";
const char * Strings::Error_NoVideosInMyVideos 		= "To watch your videos in Cinema, record\nwith your camera or copy videos to your\nphone's internal memory or SD card.";
const char * Strings::Error_UnableToPlayMovie 		= "We couldn't play this video.";

const char * Strings::MoviePlayer_Reorient			= "Tap the touchpad to reorient screen";

} // namespace OculusCinema
