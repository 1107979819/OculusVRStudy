/************************************************************************************

Filename    :   MoviePlayerView.cpp
Content     :
Created     :	6/17/2014
Authors     :   Jim Dosé

Copyright   :   Copyright 2014 Oculus VR, LLC. All Rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the Cinema/ directory. An additional grant 
of patent rights can be found in the PATENTS file in the same directory.

*************************************************************************************/

#include <android/keycodes.h>
#include "CinemaApp.h"
#include "Native.h"
#include "VRMenu/VRMenuMgr.h"

namespace OculusCinema
{

const VRMenuId_t MoviePlayerView::ID_MOVE_SCREEN( 1000 );

const int MoviePlayerView::MaxSeekSpeed = 5;

MoviePlayerView::MoviePlayerView( CinemaApp &cinema ) :
	View( "MoviePlayerView" ),
	Cinema( cinema ),
	uiActive( false ),
	RepositionScreen( false ),
	SeekSpeed( 0 ),
	PlaybackPos( 0 ),
	NextSeekTime( 0 ),
	Menu( NULL ),
	MenuOpen( false ),
	MoveScreenHandle( 0 ),
	MoveScreenObj( NULL ),
	MoveScreenAlpha()

{
}

MoviePlayerView::~MoviePlayerView()
{
}

//=========================================================================================

void MoviePlayerView::OneTimeInit( const char * launchIntent )
{
	LOG( "MoviePlayerView::OneTimeInit" );

	const double start = TimeInSeconds();

	GazeUserId = Cinema.app->GetGazeCursor().GenerateUserId();

	CreateMenu( Cinema.app, Cinema.app->GetVRMenuMgr(), Cinema.app->GetDefaultFont() );

	LOG( "MoviePlayerView::OneTimeInit: %3.1f seconds", TimeInSeconds() - start );
}

void MoviePlayerView::OneTimeShutdown()
{
	LOG( "MoviePlayerView::OneTimeShutdown" );
}

void MoviePlayerView::CreateMenu( App * app, OvrVRMenuMgr & menuMgr, BitmapFont const & font )
{
	Menu = VRMenu::Create( "MoviePlayerMenu" );

    Array< VRMenuObjectParms const * > parms;

	Posef moveScreenPose( Quatf( Vector3f( 0.0f, 1.0f, 0.0f ), 0.0f ),
			Vector3f(  0.0f, 0.0f,  -1.8f ) );

	VRMenuFontParms moveScreenFontParms( true, true, false, false, false, 0.5f );

	VRMenuSurfaceParms moveScreenSurfParms( "",
			NULL, SURFACE_TEXTURE_MAX,
			NULL, SURFACE_TEXTURE_MAX,
			NULL, SURFACE_TEXTURE_MAX );

	VRMenuObjectParms moveScreenParms( VRMENU_BUTTON, Array< VRMenuComponent* >(), moveScreenSurfParms,
			Strings::MoviePlayer_Reorient, moveScreenPose, Vector3f( 1.0f ), moveScreenFontParms, ID_MOVE_SCREEN,
			VRMenuObjectFlags_t(), VRMenuObjectInitFlags_t( VRMENUOBJECT_INIT_FORCE_POSITION ) );

	parms.PushBack( &moveScreenParms );

	Menu->InitWithItems(menuMgr, font, 0.0f, VRMenuFlags_t( VRMENU_FLAG_TRACK_GAZE ) | VRMENU_FLAG_BACK_KEY_DOESNT_EXIT, parms);
	parms.Clear();

	MoveScreenHandle = Menu->HandleForId( menuMgr, ID_MOVE_SCREEN );
	MoveScreenObj = menuMgr.ToObject( MoveScreenHandle );

    MoveScreenObj->AddFlags( VRMENUOBJECT_DONT_RENDER );
	Vector3f moveScreenTextPosition = Vector3f( 0.0f, -24 * VRMenuObject::DEFAULT_TEXEL_SCALE, 0.0f );
    MoveScreenObj->SetTextLocalPosition( moveScreenTextPosition );

    // ==============================================================================
    //
    // finalize
    //
    Cinema.app->GetGuiSys().AddMenu( Menu );
}

void MoviePlayerView::OnOpen()
{
	LOG( "OnOpen" );
	CurViewState = VIEWSTATE_OPEN;

	Cinema.SceneMgr.ClearMovie();

	SeekSpeed = 0;
	PlaybackPos = 0;
	NextSeekTime = 0;

	RepositionScreen = false;
	MoveScreenAlpha.Set( 0, 0, 0, 0.0f );

	HideUI();
	Cinema.SceneMgr.LightsOff( 1.5f );

	Cinema.StartMoviePlayback();
}

void MoviePlayerView::OnClose()
{
	LOG( "OnClose" );
	CurViewState = VIEWSTATE_CLOSED;
	HideUI();
	Cinema.app->GetGazeCursor().ShowCursor();

	if ( MenuOpen )
	{
		Cinema.app->GetGuiSys().CloseMenu( Cinema.app, Menu, false );
		MenuOpen = false;
	}

	Cinema.app->GetActivityPanel().Visible = false;

	Cinema.SceneMgr.ClearMovie();

	if ( Cinema.SceneMgr.VoidedScene )
	{
		Cinema.SceneMgr.SetSceneModel( Cinema.GetCurrentTheater() );
	}
}

/*
 * Command
 *
 * Actions that need to be performed on the render thread.
 */
bool MoviePlayerView::Command( const char * msg )
{
	if ( CurViewState != VIEWSTATE_OPEN )
	{
		return false;
	}

	if ( MatchesHead( "resume ", msg ) )
	{
		Cinema.MovieSelection( false );
		return false;	// allow VrLib to handle it, too
	}
	else if ( MatchesHead( "pause ", msg ) )
	{
		Native::StopMovie( Cinema.app );
		return false;	// allow VrLib to handle it, too
	}

	return false;
}

void MoviePlayerView::BackPressed()
{
	LOG( "BackPressed" );
	HideUI();
	if ( Cinema.AllowTheaterSelection() )
	{
		LOG( "Opening TheaterSelection" );
		Cinema.TheaterSelection();
	}
	else
	{
		LOG( "Opening MovieSelection" );
		Cinema.MovieSelection( true );
	}
}

bool MoviePlayerView::OnKeyEvent( const int keyCode, const KeyState::eKeyEventType eventType )
{
	switch ( keyCode )
	{
		case AKEYCODE_BACK:
		{
			switch ( eventType )
			{
				case KeyState::KEY_EVENT_SHORT_PRESS:
				LOG( "KEY_EVENT_SHORT_PRESS" );
				BackPressed();
				return true;
				break;
				default:
				//LOG( "unexpected back key state %i", eventType );
				break;
			}
		}
		break;
		case AKEYCODE_MEDIA_NEXT:
			if ( eventType == KeyState::KEY_EVENT_UP )
			{
				Cinema.SetMovie( Cinema.GetNextMovie() );
				Cinema.ResumeOrRestartMovie();
			}
			break;
		case AKEYCODE_MEDIA_PREVIOUS:
			if ( eventType == KeyState::KEY_EVENT_UP )
			{
				Cinema.SetMovie( Cinema.GetPreviousMovie() );
				Cinema.ResumeOrRestartMovie();
			}
			break;
		break;
	}
	return false;
}

//=========================================================================================

static bool InsideUnit( const Vector2f v )
{
	return v.x > -1.0f && v.x < 1.0f && v.y > -1.0f && v.y < 1.0f;
}

void MoviePlayerView::ShowUI()
{
	Native::ShowUI( Cinema.app );
	Cinema.SceneMgr.ForceMono = true;
	Cinema.app->GetGazeCursor().ShowCursor();
	uiActive = true;
}

void MoviePlayerView::HideUI()
{
	Cinema.app->GetGazeCursor().HideCursor();
	Native::HideUI( Cinema.app );
	Cinema.SceneMgr.ForceMono = false;
	uiActive = false;

	SeekSpeed = 0;
	PlaybackPos = 0;
	NextSeekTime = 0;
}

void MoviePlayerView::CheckDebugControls( const VrFrame & vrFrame )
{
	if ( !Cinema.AllowDebugControls )
	{
		return;
	}

#if 0
	if ( !( vrFrame.Input.buttonState & BUTTON_Y ) )
	{
		Cinema.SceneMgr.ChangeSeats( vrFrame );
	}
#endif

	if ( vrFrame.Input.buttonPressed & BUTTON_X )
	{
		Cinema.SceneMgr.ToggleLights( 0.5f );
	}

	if ( vrFrame.Input.buttonPressed & BUTTON_SELECT )
	{
		Cinema.SceneMgr.UseOverlay = !Cinema.SceneMgr.UseOverlay;
		Cinema.app->CreateToast( "Overlay: %i", Cinema.SceneMgr.UseOverlay );
	}

	// Press Y to toggle FreeScreen mode, while holding the scale and distance can be adjusted
	if ( vrFrame.Input.buttonPressed & BUTTON_Y )
	{
		Cinema.SceneMgr.FreeScreenActive = !Cinema.SceneMgr.FreeScreenActive || Cinema.SceneMgr.SceneInfo.UseFreeScreen;
		Cinema.SceneMgr.PutScreenInFront();
	}

	if ( Cinema.SceneMgr.FreeScreenActive && ( vrFrame.Input.buttonState & BUTTON_Y ) )
	{
		Cinema.SceneMgr.FreeScreenDistance -= vrFrame.Input.sticks[0][1] * vrFrame.DeltaSeconds * 3;
		Cinema.SceneMgr.FreeScreenDistance = OVR::Alg::Clamp( Cinema.SceneMgr.FreeScreenDistance, 1.0f, 4.0f );
		Cinema.SceneMgr.FreeScreenScale += vrFrame.Input.sticks[0][0] * vrFrame.DeltaSeconds * 3;
		Cinema.SceneMgr.FreeScreenScale = OVR::Alg::Clamp( Cinema.SceneMgr.FreeScreenScale, 1.0f, 4.0f );

		if ( vrFrame.Input.buttonReleased & (BUTTON_LSTICK_UP|BUTTON_LSTICK_DOWN|BUTTON_LSTICK_LEFT|BUTTON_LSTICK_RIGHT) )
		{
			Cinema.app->CreateToast( "FreeScreenDistance:%3.1f  FreeScreenScale:%3.1f", Cinema.SceneMgr.FreeScreenDistance, Cinema.SceneMgr.FreeScreenScale );
		}
	}
}

bool MoviePlayerView::CursorInsidePlaybackControls() const
{
	if ( !uiActive )
	{
		return false;
	}

	int left, bottom, right, top;

	// get the playback controls position
	if ( !Native::GetPlaybackControlsRect( Cinema.app, left, bottom, right, top ) )
	{
		// it takes a few frames to initilize the bounds of the controls
		LOG( "PlaybackControlsRect Not initialized");
		return false;
	}

	const Vector2f panelCursor = GazeCoordinatesOnPanel( Cinema.SceneMgr.Scene.CenterViewMatrix(), Cinema.app->GetActivityPanel().Matrix, Cinema.app->GetActivityPanel().AlternateGazeCheck  );
	Vector2f cursorPos( (panelCursor.x * 0.5 + 0.5) * Cinema.app->GetActivityPanel().Width, (0.5 - panelCursor.y * 0.5) * Cinema.app->GetActivityPanel().Height );
	if ( ( cursorPos.x >= left ) && ( cursorPos.x < right ) && ( cursorPos.y >= top ) && ( cursorPos.y < bottom ) )
	{
		return true;
	}

	return false;
}

void MoviePlayerView::CheckInput( const VrFrame & vrFrame )
{
	if ( !uiActive && !RepositionScreen )
	{
		if ( ( vrFrame.Input.buttonPressed & BUTTON_A ) || ( ( vrFrame.Input.buttonReleased & BUTTON_TOUCH ) && !( vrFrame.Input.buttonState & BUTTON_TOUCH_WAS_SWIPE ) ) )
		{
			// open ui if it's not visible
			Cinema.app->PlaySound( "touch_up" );
			ShowUI();
		}
	}

	if ( vrFrame.Input.buttonPressed & ( BUTTON_DPAD_LEFT | BUTTON_SWIPE_BACK ) )
	{
		if ( ( vrFrame.Input.buttonPressed & BUTTON_DPAD_LEFT ) || !CursorInsidePlaybackControls() )
		{
			ShowUI();
			if ( SeekSpeed == 0 )
			{
				Native::PauseMovie( Cinema.app );
				PlaybackPos = Native::GetPosition( Cinema.app );
			}

			SeekSpeed--;
			if ( ( SeekSpeed == 0 ) || ( SeekSpeed < -MaxSeekSpeed ) )
			{
				SeekSpeed = 0;
				Native::ResumeMovie( Cinema.app );
			}

			Cinema.app->PlaySound( "touch_up" );
		}
	}

	if ( vrFrame.Input.buttonPressed & ( BUTTON_DPAD_RIGHT | BUTTON_SWIPE_FORWARD ) )
	{
		if ( ( vrFrame.Input.buttonPressed & BUTTON_DPAD_RIGHT ) || !CursorInsidePlaybackControls() )
		{
			ShowUI();
			if ( SeekSpeed == 0 )
			{
				Native::PauseMovie( Cinema.app );
				PlaybackPos = Native::GetPosition( Cinema.app );
			}

			SeekSpeed++;
			if ( ( SeekSpeed == 0 ) || ( SeekSpeed > MaxSeekSpeed ) )
			{
				SeekSpeed = 0;
				Native::ResumeMovie( Cinema.app );
			}

			Cinema.app->PlaySound( "touch_up" );
		}
	}

	if ( Cinema.SceneMgr.FreeScreenActive )
	{
		const Vector2f screenCursor = GazeCoordinatesOnPanel( Cinema.SceneMgr.Scene.CenterViewMatrix(), Cinema.SceneMgr.FreeScreenMatrix(), Cinema.app->GetActivityPanel().AlternateGazeCheck  );
		if ( !InsideUnit( screenCursor ) && !CursorInsidePlaybackControls() )
		{
			// outside of screen, so show reposition message
			const double now = TimeInSeconds();
			float alpha = MoveScreenAlpha.Value( now );
			if ( alpha > 0.0f )
			{
				MoveScreenObj->RemoveFlags( VRMENUOBJECT_DONT_RENDER );
				MoveScreenObj->SetTextColor( Vector4f( alpha ) );
			}

			if ( vrFrame.Input.buttonPressed & ( BUTTON_A | BUTTON_TOUCH ) )
			{
				RepositionScreen = true;
			}
		}
		else
		{
			// onscreen, so hide message
			const double now = TimeInSeconds();
			MoveScreenAlpha.Set( now, -1.0f, now + 1.0f, 1.0f );
			MoveScreenObj->AddFlags( VRMENUOBJECT_DONT_RENDER );
		}
	}

	// while we're holding down the button or touchpad, reposition screen
	if ( RepositionScreen )
	{
		if ( vrFrame.Input.buttonState & ( BUTTON_A | BUTTON_TOUCH ) )
		{
			Cinema.SceneMgr.PutScreenInFront();
		}
		else
		{
			RepositionScreen = false;
		}
	}

	if ( vrFrame.Input.buttonPressed & BUTTON_START )
	{
		Native::TogglePlaying( Cinema.app );
		SeekSpeed = 0;
	}

	if ( vrFrame.Input.buttonPressed & BUTTON_SELECT )
	{
		// movie select
		Cinema.app->PlaySound( "touch_up" );
		Cinema.MovieSelection( false );
	}

	if ( vrFrame.Input.buttonPressed & BUTTON_B )
	{
		if ( !uiActive )
		{
			BackPressed();
		}
		else
		{
			Cinema.app->PlaySound( "touch_up" );
			HideUI();
			Native::ResumeMovie( Cinema.app );
		}
	}
}

void MoviePlayerView::UpdateUI( const VrFrame & vrFrame )
{
	// only show cursor when uiActive
	if ( uiActive )
	{
		const Matrix4f screenModel = Cinema.app->GetActivityPanel().Matrix;

		// -1 to 1 range on panelMatrix, returns -2,-2 if looking away from the panel
		const Vector2f cursor = GazeCoordinatesOnPanel( Cinema.SceneMgr.Scene.CenterViewMatrix(), screenModel, Cinema.app->GetActivityPanel().AlternateGazeCheck );

		const bool press = ( vrFrame.Input.buttonPressed & ( BUTTON_A | BUTTON_TOUCH ) ) != 0;
		const bool release = ( ( vrFrame.Input.buttonReleased & ( BUTTON_A | BUTTON_TOUCH ) ) != 0 );

		ePlaybackControlsEvent uiButtonPressed = ( ePlaybackControlsEvent )Native::GazeCursor( Cinema.app, (cursor.x * 0.5 + 0.5) * Cinema.app->GetActivityPanel().Width,
			(0.5 - cursor.y * 0.5) * Cinema.app->GetActivityPanel().Height, press, release, SeekSpeed );

		// don't register a button press if we just finished a swipe
		if ( !( vrFrame.Input.buttonState & BUTTON_TOUCH_WAS_SWIPE ) )
		{
			switch( uiButtonPressed )
			{
			case UI_RW_PRESSED :
				// rewind
				Cinema.app->PlaySound( "touch_up" );
				if ( SeekSpeed == 0 )
				{
					Native::PauseMovie( Cinema.app );
					PlaybackPos = Native::GetPosition( Cinema.app );
				}

				SeekSpeed--;
				if ( ( SeekSpeed == 0 ) || ( SeekSpeed < -MaxSeekSpeed ) )
				{
					SeekSpeed = 0;
					Native::ResumeMovie( Cinema.app );
				}
				break;

			case UI_PLAY_PRESSED :
				// play/pause
				Cinema.app->PlaySound( "touch_up" );
				Native::TogglePlaying( Cinema.app );
				SeekSpeed = 0;
				break;

			case UI_FF_PRESSED :
				// fast forward
				Cinema.app->PlaySound( "touch_up" );
				if ( SeekSpeed == 0 )
				{
					Native::PauseMovie( Cinema.app );
					PlaybackPos = Native::GetPosition( Cinema.app );
				}

				SeekSpeed++;
				if ( ( SeekSpeed == 0 ) || ( SeekSpeed > MaxSeekSpeed ) )
				{
					SeekSpeed = 0;
					Native::ResumeMovie( Cinema.app );
				}
				break;

			case UI_CAROUSEL_PRESSED :
				// movie select
				Cinema.app->PlaySound( "touch_up" );
				Cinema.MovieSelection( false );
				break;

			case UI_CLOSE_UI_PRESSED :
				Cinema.app->PlaySound( "touch_up" );
				HideUI();
				Native::ResumeMovie( Cinema.app );
				break;

			case UI_USER_TIMEOUT :
				HideUI();
				Native::ResumeMovie( Cinema.app );
				break;

			case UI_SEEK_PRESSED :
				PlaybackPos = Native::GetPosition( Cinema.app );
				NextSeekTime = TimeInSeconds() + 0.25;
				break;

			default:
			case UI_NO_EVENT :
				break;
			}
		}

		float cursorDistance = FLT_MAX;
		eGazeCursorStateType cursorState = CURSOR_STATE_NORMAL;

		if ( InsideUnit( cursor ) )
		{
			// FIXME: Project onto Free Screen
			const Vector3f pos = screenModel.Transform( Vector3f( cursor.x, cursor.y, 0.0f ) );
			cursorDistance = ( pos - Cinema.SceneMgr.Scene.CenterViewMatrix().GetTranslation() ).Length();
		}
		Cinema.app->GetGazeCursor().UpdateForUser( GazeUserId, cursorDistance, cursorState );
	}
}

/*
 * DrawEyeView
 */
Matrix4f MoviePlayerView::DrawEyeView( const int eye, const float fovDegrees )
{
	return Cinema.SceneMgr.DrawEyeView( eye, fovDegrees );
}

/*
 * Frame()
 *
 * App override
 */
Matrix4f MoviePlayerView::Frame( const VrFrame & vrFrame )
{
	// Drop to 2x MSAA during playback, people should be focused
	// on the high quality screen.
	EyeParms eyeParms = Cinema.app->GetEyeParms();
	eyeParms.multisamples = 2;
	Cinema.app->SetEyeParms( eyeParms );

	if ( Native::HadPlaybackError( Cinema.app ) )
	{
		LOG( "Playback failed" );
		Cinema.UnableToPlayMovie();
	}
	else if ( Native::IsPlaybackFinished( Cinema.app ) )
	{
		LOG( "Playback finished" );
		Cinema.MovieFinished();
	}

	CheckInput( vrFrame );
	CheckDebugControls( vrFrame );
	UpdateUI( vrFrame );

	if ( Cinema.SceneMgr.FreeScreenActive && !MenuOpen )
	{
		Cinema.app->GetGuiSys().OpenMenu( Cinema.app, Cinema.app->GetGazeCursor(), "MoviePlayerMenu" );
		MenuOpen = true;
	}
	else if ( !Cinema.SceneMgr.FreeScreenActive && MenuOpen )
	{
		Cinema.app->GetGuiSys().CloseMenu( Cinema.app, Menu, false );
		MenuOpen = false;
	}

	if ( SeekSpeed != 0 )
	{
		const double now = TimeInSeconds();
		if ( now > NextSeekTime )
		{
			int PlaybackSpeed = ( SeekSpeed < 0 ) ? -( 1 << -SeekSpeed ) : ( 1 << SeekSpeed );
			PlaybackPos += 250 * PlaybackSpeed;
			Native::SetPosition( Cinema.app, PlaybackPos );
			NextSeekTime = now + 0.25;
		}
	}

	return Cinema.SceneMgr.Frame( vrFrame );
}

} // namespace OculusCinema
