/************************************************************************************

Filename    :   MoviePlayerView.h
Content     :
Created     :	6/17/2014
Authors     :   Jim Dosé

Copyright   :   Copyright 2014 Oculus VR, LLC. All Rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the Cinema/ directory. An additional grant 
of patent rights can be found in the PATENTS file in the same directory.

*************************************************************************************/

#if !defined( MoviePlayerView_h )
#define MoviePlayerView_h

#include "GazeCursor.h"
#include "VRMenu/GuiSys.h"
#include "Lerp.h"

using namespace OVR;

namespace OculusCinema {

enum ePlaybackControlsEvent
{
	UI_NO_EVENT = 0,
	UI_RW_PRESSED = 1,
	UI_PLAY_PRESSED = 2,
	UI_FF_PRESSED = 3,
	UI_CAROUSEL_PRESSED = 4,
	UI_CLOSE_UI_PRESSED = 5,
	UI_USER_TIMEOUT = 6,
	UI_SEEK_PRESSED = 7
};

class CinemaApp;

class MoviePlayerView : public View
{
public:
							MoviePlayerView( CinemaApp &app_ );
	virtual 				~MoviePlayerView();

	virtual void 			OneTimeInit( const char * launchIntent );
	virtual void			OneTimeShutdown();

	virtual void 			OnOpen();
	virtual void 			OnClose();

	virtual bool 			Command( const char * msg );
	virtual bool 			OnKeyEvent( const int keyCode, const KeyState::eKeyEventType eventType );

	virtual Matrix4f 		DrawEyeView( const int eye, const float fovDegrees );
	virtual Matrix4f 		Frame( const VrFrame & vrFrame );

private:
	CinemaApp &				Cinema;

	gazeCursorUserId_t		GazeUserId;	// id unique to this swipe view for interacting with gaze cursor

	bool					uiActive;

	bool					RepositionScreen;

	static const int		MaxSeekSpeed;
	int						SeekSpeed;
	int						PlaybackPos;
	double					NextSeekTime;

    static const VRMenuId_t ID_MOVE_SCREEN;

	VRMenu *				Menu;
	bool					MenuOpen;

	menuHandle_t			MoveScreenHandle;
	VRMenuObject *			MoveScreenObj;
	Lerp					MoveScreenAlpha;

private:
	void 					CreateMenu( App * app, OvrVRMenuMgr & menuMgr, BitmapFont const & font );

	void					BackPressed();

	void 					UpdateUI( const VrFrame & vrFrame );
	void 					CheckInput( const VrFrame & vrFrame );
	void 					CheckDebugControls( const VrFrame & vrFrame );

	bool 					CursorInsidePlaybackControls() const;

	void 					ShowUI();
	void 					HideUI();
};

} // namespace OculusCinema

#endif // MoviePlayerView_h
