/************************************************************************************

Filename    :   MovieSelectionView.cpp
Content     :
Created     :	6/19/2014
Authors     :   Jim Dosé

Copyright   :   Copyright 2014 Oculus VR, LLC. All Rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the Cinema/ directory. An additional grant 
of patent rights can be found in the PATENTS file in the same directory.

*************************************************************************************/

#include <android/keycodes.h>
#include "LibOVR/Src/Kernel/OVR_String_Utils.h"
#include "App.h"
#include "MovieSelectionView.h"
#include "CinemaApp.h"
#include "VRMenu/VRMenuMgr.h"
#include "MovieCategoryComponent.h"
#include "MoviePosterComponent.h"
#include "MovieSelectionComponent.h"
#include "SwipeHintComponent.h"
#include "PackageFiles.h"

namespace OculusCinema {

const VRMenuId_t MovieSelectionView::ID_CENTER_ROOT( 1000 );
const VRMenuId_t MovieSelectionView::ID_ERROR_MESSAGE( 1001 );
const VRMenuId_t MovieSelectionView::ID_SDCARD_MESSAGE( 1002 );
const VRMenuId_t MovieSelectionView::ID_PLAIN_ERROR_MESSAGE( 1003 );
const VRMenuId_t MovieSelectionView::ID_MOVIE_SELECTION( 2000 );
const VRMenuId_t MovieSelectionView::ID_MOVIE_ROOT( 2001 );
const VRMenuId_t MovieSelectionView::ID_MOVIE_POSTER_ROOTS( 2002 );
const VRMenuId_t MovieSelectionView::ID_MOVIE_POSTERS( 8000 );
const VRMenuId_t MovieSelectionView::ID_CATEGORY_ROOT( 3000 );
const VRMenuId_t MovieSelectionView::ID_CATEGORY_ITEMS( 3001 );
const VRMenuId_t MovieSelectionView::ID_MOVIE_TITLE_ROOT( 4000 );
const VRMenuId_t MovieSelectionView::ID_MOVIE_TITLE( 4001 );
const VRMenuId_t MovieSelectionView::ID_SWIPE_ICON_LEFT( 9002 );
const VRMenuId_t MovieSelectionView::ID_SWIPE_ICON_RIGHT( 9505 );
const VRMenuId_t MovieSelectionView::ID_RESUME_ICON( 4008 );
const VRMenuId_t MovieSelectionView::ID_TIMER_ICON( 4009 );
const VRMenuId_t MovieSelectionView::ID_TIMER_TEXT( 4010 );
const VRMenuId_t MovieSelectionView::ID_3D_ICONS( 5000 );
const VRMenuId_t MovieSelectionView::ID_SHADOWS( 6000 );
const VRMenuId_t MovieSelectionView::ID_MOVE_SCREEN( 7000 );

static const int PosterWidth = 228;
static const int PosterHeight = 344;

static const Vector3f PosterScale( 4.4859375f * 0.98f );

static const double TimerTotalTime = 10;

static const int NumSwipeTrails = 3;


//=======================================================================================

MovieSelectionView::MovieSelectionView( CinemaApp &cinema ) :
	View( "MovieSelectionView" ),
	Cinema( cinema ),
	Menu( NULL ),
	CenterRootHandle( 0 ),
	CenterRoot( NULL ),
	ErrorMessageHandle( 0 ),
	ErrorMessage( NULL ),
	ErrorMessagePose(),
	ErrorMessageScale( 0.0f ),
	ErrorMessageTextPosition(),
	ErrorMessageClicked( false ),
	SDCardMessageHandle( 0 ),
	SDCardMessage( NULL ),
	SDCardMessagePose(),
	SDCardMessageScale( 0.0f ),
	SDCardMessageTextPosition(),
	PlainErrorMessageHandle( 0 ),
	PlainErrorMessage( NULL ),
	PlainErrorMessagePose(),
	PlainErrorMessageScale( 0.0f ),
	PlainErrorMessageTextPosition(),
	MovieRootHandle( 0 ),
    MovieRoot( NULL ),
    CategoryRootHandle( 0 ),
    CategoryRoot( NULL ),
	CategoryBoundsExpandMin(),
	CategoryBoundsExpandMax(),
	TitleRootHandle( 0 ),
	TitleRoot( NULL ),
	MovieTitleHandle( 0 ),
	MovieTitle( NULL ),
	MovieTitlePose(),
	SelectionHandle( 0 ),
	SelectionObject( NULL ),
	SelectionComponent( NULL ),
	SelectionOffset(),
	SelectionBoundsExpandMin(),
	SelectionBoundsExpandMax(),
	CenterPoster( NULL ),
	CenterIndex( 0 ),
	CenterPosition(),
	Is3DIconPose(),
	ShadowPose(),
	SwipeIconPose(),
	SwipeIconLeftWidth( 0 ),
	SwipeIconLeftHeight( 0 ),
	SwipeIconRightWidth( 0 ),
	SwipeIconRightHeight( 0 ),
	ResumeIconHandle( 0 ),
	ResumeIcon( NULL ),
	ResumeIconPose(),
	ResumeIconScale( 0.0f ),
	ResumeTextPosition(),
	TimerIconHandle( 0 ),
	TimerIcon( NULL ),
	TimerIconOffset(),
	TimerIconScale( 0.0f ),
	TimerStartTime( 0 ),
	TimerValue( 0 ),
	ShowTimer( false ),
	TimerTextHandle( 0 ),
	TimerText( NULL ),
	TimerTextPose(),
	MoveScreenHandle( 0 ),
	MoveScreenObj( NULL ),
	MoveScreenAlpha(),
	SelectionFader(),
	MovieBrowser( NULL ),
	MoviePanelPositions(),
    MoviePosterComponents(),
    Categories(),
    CurrentCategory( CATEGORY_TRAILERS ),
	MovieList(),
	MoviesIndex( 0 ),
	LastMovieDisplayed( NULL ),
	HasOpened( false ),
	RepositionScreen( false )

{
	// This is called at library load time, so the system is not initialized
	// properly yet.
}

MovieSelectionView::~MovieSelectionView()
{
	DeletePointerArray( MovieBrowserItems );
}

void MovieSelectionView::OneTimeInit( const char * launchIntent )
{
	LOG( "MovieSelectionView::OneTimeInit" );

	const double start = TimeInSeconds();

    CreateMenu( Cinema.app, Cinema.app->GetVRMenuMgr(), Cinema.app->GetDefaultFont() );

	SetCategory( CATEGORY_TRAILERS );

	LOG( "MovieSelectionView::OneTimeInit %3.1f seconds", TimeInSeconds() - start );
}

void MovieSelectionView::OneTimeShutdown()
{
	LOG( "MovieSelectionView::OneTimeShutdown" );
}

void MovieSelectionView::OnOpen()
{
	LOG( "OnOpen" );
	CurViewState = VIEWSTATE_OPEN;

	LastMovieDisplayed = NULL;

	if ( Cinema.InLobby )
	{
		Cinema.SceneMgr.SetSceneModel( *Cinema.ModelMgr.BoxOffice );
	}

	Cinema.SceneMgr.LightsOn( 1.5f );

	const double now = TimeInSeconds();
	SelectionFader.Set( now, 0, now + 0.1, 1.0f );

	if ( Cinema.InLobby )
	{
		CategoryRoot->RemoveFlags( VRMENUOBJECT_DONT_RENDER );
		Menu->SetFlags( VRMENU_FLAG_BACK_KEY_EXITS_APP );
	}
	else
	{
		CategoryRoot->AddFlags( VRMENUOBJECT_DONT_RENDER );
		Menu->SetFlags( VRMenuFlags_t() );
	}

	ResumeIcon->AddFlags( VRMENUOBJECT_DONT_RENDER );
	TimerIcon->AddFlags( VRMENUOBJECT_DONT_RENDER );
	CenterRoot->RemoveFlags( VRMENUOBJECT_DONT_RENDER );
	
	MovieBrowser->SetSelectionIndex( MoviesIndex );

	RepositionScreen = false;

	UpdateMenuPosition();
	Cinema.SceneMgr.ClearGazeCursorGhosts();
	Cinema.app->GetGuiSys().OpenMenu( Cinema.app, Cinema.app->GetGazeCursor(), "MovieBrowser" );

	MoviePosterComponent::ShowShadows = Cinema.InLobby;

	SwipeHintComponent::ShowSwipeHints = true;

	Cinema.app->GetSwapParms().WarpProgram = WP_CHROMATIC;
	HasOpened = false;
}

void MovieSelectionView::OnClose()
{
	LOG( "OnClose" );
	ShowTimer = false;
	CurViewState = VIEWSTATE_CLOSED;
	CenterRoot->AddFlags( VRMENUOBJECT_DONT_RENDER );
	Cinema.app->GetGuiSys().CloseMenu( Cinema.app, Menu, false );
}

bool MovieSelectionView::Command( const char * msg )
{
	return false;
}

bool MovieSelectionView::OnKeyEvent( const int keyCode, const KeyState::eKeyEventType eventType )
{
	switch ( keyCode )
	{
		case AKEYCODE_BACK:
		{
			switch ( eventType )
			{
				case KeyState::KEY_EVENT_DOUBLE_TAP:
					LOG( "KEY_EVENT_DOUBLE_TAP" );
					return true;
				default:
					return false;
			}
		}
	}
	return false;
}

//=======================================================================================

void MovieSelectionView::CreateMenu( App * app, OvrVRMenuMgr & menuMgr, BitmapFont const & font )
{
	Menu = VRMenu::Create( "MovieBrowser" );

    Vector3f fwd( 0.0f, 0.0f, 1.0f );
	Vector3f up( 0.0f, 1.0f, 0.0f );
	Vector3f defaultScale( 1.0f );

    Array< VRMenuObjectParms const * > parms;

	VRMenuFontParms fontParms( true, true, false, false, false, 1.0f );

    // ==============================================================================
    //
    // menu roots
    //

	//
	// center
	//
	const Posef centerPose( Quatf( Vector3f( 0.0f, 1.0f, 0.0f ), 0.0f ), Vector3f( 0.0f, 0.0f, 0.0f ) );

	VRMenuObjectParms centerRootParms( VRMENU_CONTAINER, Array< VRMenuComponent* >(), VRMenuSurfaceParms(),
			"CenterRoot", centerPose, defaultScale, fontParms, ID_CENTER_ROOT,
			VRMenuObjectFlags_t(), VRMenuObjectInitFlags_t( VRMENUOBJECT_INIT_FORCE_POSITION ) );

	parms.PushBack( &centerRootParms );

	Menu->InitWithItems(menuMgr, font, 0.0f, VRMenuFlags_t(VRMENU_FLAG_BACK_KEY_EXITS_APP), parms);
	parms.Clear();

	CenterRootHandle = Menu->HandleForId(menuMgr, ID_CENTER_ROOT);
	CenterRoot = menuMgr.ToObject( CenterRootHandle );


	//
	// movies
	//
	const Posef moviePose( Quatf( Vector3f( 0.0f, 1.0f, 0.0f ), 0.0f ), Vector3f( 0.0f, 0.0f, 0.0f ) );

	VRMenuObjectParms movieRootParms( VRMENU_CONTAINER, Array< VRMenuComponent* >(), VRMenuSurfaceParms(),
			"MovieRoot", moviePose, defaultScale, fontParms, ID_MOVIE_ROOT,
			VRMenuObjectFlags_t(), VRMenuObjectInitFlags_t( VRMENUOBJECT_INIT_FORCE_POSITION ) );

	parms.PushBack( &movieRootParms );

	//
	// category
	//
	const Posef categoryPose( Quatf( Vector3f( 0.0f, 1.0f, 0.0f ), 0.0f ), Vector3f( 0.0f, 0.0f, 0.0f ) );

	VRMenuObjectParms categoryRootParms( VRMENU_CONTAINER, Array< VRMenuComponent* >(), VRMenuSurfaceParms(),
			"CategoryRoot", categoryPose, defaultScale, fontParms, ID_CATEGORY_ROOT,
			VRMenuObjectFlags_t(), VRMenuObjectInitFlags_t( VRMENUOBJECT_INIT_FORCE_POSITION ) );

	parms.PushBack( &categoryRootParms );

	//
	// title
	//
	const Posef titlePose( Quatf( Vector3f( 0.0f, 1.0f, 0.0f ), 0.0f ), Vector3f( 0.0f, 0.0f, 0.0f ) );

	VRMenuObjectParms titleRootParms( VRMENU_CONTAINER, Array< VRMenuComponent* >(), VRMenuSurfaceParms(),
			"TitleRoot", titlePose, defaultScale, fontParms, ID_MOVIE_TITLE_ROOT,
			VRMenuObjectFlags_t(), VRMenuObjectInitFlags_t( VRMENUOBJECT_INIT_FORCE_POSITION ) );

	parms.PushBack( &titleRootParms );

    Menu->AddItems( menuMgr, font, parms, CenterRootHandle, false );
    parms.Clear();

    // ==============================================================================
    //
	// error message
	//
	ErrorMessagePose = Posef( Quatf( Vector3f( 0.0f, 1.0f, 0.0f ), 0.0f ),
			Vector3f(  0.00f, 1.76f + 330.0f * VRMenuObject::DEFAULT_TEXEL_SCALE,  -7.39f + 0.5f ) );

	VRMenuFontParms errorMessageFontParms( true, true, false, false, false, 0.5f );

	VRMenuSurfaceParms errorMessageSurfParms( "",
			"assets/error.png", SURFACE_TEXTURE_DIFFUSE,
			NULL, SURFACE_TEXTURE_MAX,
			NULL, SURFACE_TEXTURE_MAX );

	ErrorMessageScale = 5.0f;

	VRMenuObjectParms errorMessageParms( VRMENU_BUTTON, Array< VRMenuComponent* >(), errorMessageSurfParms,
			"", ErrorMessagePose, Vector3f( ErrorMessageScale ), errorMessageFontParms, ID_ERROR_MESSAGE,
			VRMenuObjectFlags_t(), VRMenuObjectInitFlags_t( VRMENUOBJECT_INIT_FORCE_POSITION ) );

	parms.PushBack( &errorMessageParms );

    Menu->AddItems( menuMgr, font, parms, CenterRootHandle, false );
    parms.Clear();

    ErrorMessageHandle = CenterRoot->ChildHandleForId(menuMgr, ID_ERROR_MESSAGE);
    ErrorMessage = menuMgr.ToObject( ErrorMessageHandle );

    ErrorMessage->AddFlags( VRMENUOBJECT_DONT_RENDER );
	ErrorMessageTextPosition = Vector3f( 0.0f, -48 * VRMenuObject::DEFAULT_TEXEL_SCALE * ErrorMessageScale, 0.0f );
    ErrorMessage->SetTextLocalPosition( ErrorMessageTextPosition );

    // ==============================================================================
    //
	// sdcard icon
	//
	SDCardMessagePose = Posef( Quatf( Vector3f( 0.0f, 1.0f, 0.0f ), 0.0f ),
			Vector3f(  0.00f, 1.76f + 330.0f * VRMenuObject::DEFAULT_TEXEL_SCALE,  -7.39f + 0.5f ) );

	VRMenuFontParms sdcardMessageFontParms( true, true, false, false, false, 0.5f );

	VRMenuSurfaceParms sdcardMessageSurfParms( "",
			"assets/sdcard.png", SURFACE_TEXTURE_DIFFUSE,
			NULL, SURFACE_TEXTURE_MAX,
			NULL, SURFACE_TEXTURE_MAX );

	SDCardMessageScale = 5.0f;

	VRMenuObjectParms sdcardMessageParms( VRMENU_BUTTON, Array< VRMenuComponent* >(), sdcardMessageSurfParms,
			"", ErrorMessagePose, Vector3f( ErrorMessageScale ), sdcardMessageFontParms, ID_SDCARD_MESSAGE,
			VRMenuObjectFlags_t(), VRMenuObjectInitFlags_t( VRMENUOBJECT_INIT_FORCE_POSITION ) );

	parms.PushBack( &sdcardMessageParms );

    Menu->AddItems( menuMgr, font, parms, CenterRootHandle, false );
    parms.Clear();

    SDCardMessageHandle = CenterRoot->ChildHandleForId(menuMgr, ID_SDCARD_MESSAGE);
    SDCardMessage = menuMgr.ToObject( SDCardMessageHandle );

    SDCardMessage->AddFlags( VRMENUOBJECT_DONT_RENDER );
	SDCardMessageTextPosition = Vector3f( 0.0f, -96 * VRMenuObject::DEFAULT_TEXEL_SCALE * SDCardMessageScale, 0.0f );
    SDCardMessage->SetTextLocalPosition( SDCardMessageTextPosition );

    // ==============================================================================
    //
	// error without icon
	//
    PlainErrorMessagePose = Posef( Quatf( Vector3f( 0.0f, 1.0f, 0.0f ), 0.0f ),
			Vector3f(  0.00f, 1.76f + 330.0f * VRMenuObject::DEFAULT_TEXEL_SCALE,  -7.39f + 0.5f ) );

	VRMenuFontParms plainErrorMessageFontParms( true, true, false, false, false, 0.5f );

	VRMenuSurfaceParms plainErrorMessageSurfParms( "",
			NULL, SURFACE_TEXTURE_MAX,
			NULL, SURFACE_TEXTURE_MAX,
			NULL, SURFACE_TEXTURE_MAX );

	PlainErrorMessageScale = 5.0f;

	VRMenuObjectParms plainErrorMessageParms( VRMENU_BUTTON, Array< VRMenuComponent* >(), plainErrorMessageSurfParms,
			"", PlainErrorMessagePose, Vector3f( PlainErrorMessageScale ), plainErrorMessageFontParms, ID_PLAIN_ERROR_MESSAGE,
			VRMenuObjectFlags_t(), VRMenuObjectInitFlags_t( VRMENUOBJECT_INIT_FORCE_POSITION ) );

	parms.PushBack( &plainErrorMessageParms );

    Menu->AddItems( menuMgr, font, parms, CenterRootHandle, false );
    parms.Clear();

    PlainErrorMessageHandle = CenterRoot->ChildHandleForId(menuMgr, ID_PLAIN_ERROR_MESSAGE);
    PlainErrorMessage = menuMgr.ToObject( PlainErrorMessageHandle );

    PlainErrorMessage->AddFlags( VRMENUOBJECT_DONT_RENDER );
    PlainErrorMessageTextPosition = Vector3f( 0.0f, -48 * VRMenuObject::DEFAULT_TEXEL_SCALE * PlainErrorMessageScale, 0.0f );
    PlainErrorMessage->SetTextLocalPosition( PlainErrorMessageTextPosition );

    // ==============================================================================
	//
	// movie browser
	//
	MoviePanelPositions.PushBack( PanelPose( Quatf( up, 0.0f ), Vector3f( -5.59f, 1.76f, -12.55f ), Vector4f( 0.0f, 0.0f, 0.0f, 0.0f ) ) );
	MoviePanelPositions.PushBack( PanelPose( Quatf( up, 0.0f ), Vector3f( -3.82f, 1.76f, -10.97f ), Vector4f( 0.1f, 0.1f, 0.1f, 1.0f ) ) );
	MoviePanelPositions.PushBack( PanelPose( Quatf( up, 0.0f ), Vector3f( -2.05f, 1.76f,  -9.39f ), Vector4f( 0.2f, 0.2f, 0.2f, 1.0f ) ) );
	MoviePanelPositions.PushBack( PanelPose( Quatf( up, 0.0f ), Vector3f(  0.00f, 1.76f,  -7.39f ), Vector4f( 1.0f, 1.0f, 1.0f, 1.0f ) ) );
	MoviePanelPositions.PushBack( PanelPose( Quatf( up, 0.0f ), Vector3f(  2.05f, 1.76f,  -9.39f ), Vector4f( 0.2f, 0.2f, 0.2f, 1.0f ) ) );
	MoviePanelPositions.PushBack( PanelPose( Quatf( up, 0.0f ), Vector3f(  3.82f, 1.76f, -10.97f ), Vector4f( 0.1f, 0.1f, 0.1f, 1.0f ) ) );
	MoviePanelPositions.PushBack( PanelPose( Quatf( up, 0.0f ), Vector3f(  5.59f, 1.76f, -12.55f ), Vector4f( 0.0f, 0.0f, 0.0f, 0.0f ) ) );

	CenterIndex = MoviePanelPositions.GetSizeI() / 2;
	CenterPosition = MoviePanelPositions[ CenterIndex ].Position;

    MovieBrowser = new CarouselBrowserComponent( MovieBrowserItems, MoviePanelPositions );
	MovieRootHandle = CenterRoot->ChildHandleForId(menuMgr, ID_MOVIE_ROOT);
    MovieRoot = menuMgr.ToObject( MovieRootHandle );
    MovieRoot->AddComponent( MovieBrowser );

    //
	// poster containers
    //
    int selectionWidth = 0;
    int selectionHeight = 0;
	GLuint selectionTexture = LoadTextureFromApplicationPackage( "assets/selection.png",
			TextureFlags_t( TEXTUREFLAG_NO_DEFAULT ), selectionWidth, selectionHeight );

	for ( int i = 0; i < MoviePanelPositions.GetSizeI(); ++i )
	{
		Posef posterPose( MoviePanelPositions[ i ].Orientation, MoviePanelPositions[ i ].Position );
		VRMenuObjectParms * posterParms = new VRMenuObjectParms( VRMENU_CONTAINER, Array< VRMenuComponent* >(), VRMenuSurfaceParms(),
				NULL, posterPose, defaultScale, fontParms, VRMenuId_t( ID_MOVIE_POSTER_ROOTS.Get() + i ),
				VRMenuObjectFlags_t( VRMENUOBJECT_FLAG_NO_FOCUS_GAINED ), VRMenuObjectInitFlags_t( VRMENUOBJECT_INIT_FORCE_POSITION ) );

		parms.PushBack( posterParms );
	}

	//
    // selection rectangle
	//
	SelectionComponent = new MovieSelectionComponent( this );
	Array< VRMenuComponent* > panelComps;
	panelComps.PushBack( SelectionComponent );

	VRMenuSurfaceParms panelSurfParms( "",
			selectionTexture, selectionWidth, selectionHeight, SURFACE_TEXTURE_DIFFUSE,
			0, 0, 0, SURFACE_TEXTURE_MAX,
			0, 0, 0, SURFACE_TEXTURE_MAX );

	Quatf rot( up, 0.0f );

	SelectionOffset = Vector3f( 0.0f, 0.0f, 0.01f );
	Posef posterSelectionPose( MoviePanelPositions[ CenterIndex ].Orientation, MoviePanelPositions[ CenterIndex ].Position + SelectionOffset );
	VRMenuObjectParms * selectionParms = new VRMenuObjectParms( VRMENU_BUTTON, panelComps,
			panelSurfParms, NULL, posterSelectionPose, defaultScale, fontParms, ID_MOVIE_SELECTION,
			VRMenuObjectFlags_t(), VRMenuObjectInitFlags_t( VRMENUOBJECT_INIT_FORCE_POSITION ) );

	parms.PushBack( selectionParms );

	Menu->AddItems( menuMgr, font, parms, MovieRootHandle, false );
	DeletePointerArray( parms );
    parms.Clear();

    SelectionBoundsExpandMin = Vector3f( 0.0f );
    SelectionBoundsExpandMax = Vector3f( 0.0f, -0.13f, 0.0f );

	SelectionHandle = MovieRoot->ChildHandleForId( menuMgr, ID_MOVIE_SELECTION );
	SelectionObject = menuMgr.ToObject( SelectionHandle );
	SelectionObject->SetLocalBoundsExpand( SelectionBoundsExpandMin, SelectionBoundsExpandMax );

    //
    // add shadow and 3D icon to panels
    //
    int is3DIconWidth = 0;
    int is3DIconHeight = 0;
	GLuint is3DIconTexture = LoadTextureFromApplicationPackage( "assets/3D_icon.png",
			TextureFlags_t( TEXTUREFLAG_NO_DEFAULT ), is3DIconWidth, is3DIconHeight );

    int shadowWidth = 0;
    int shadowHeight = 0;
	GLuint shadowTexture = LoadTextureFromApplicationPackage( "assets/shadow.png",
			TextureFlags_t( TEXTUREFLAG_NO_DEFAULT ), shadowWidth, shadowHeight );

	Array<VRMenuObject *> menuObjs;
	for ( int i = 0; i < MoviePanelPositions.GetSizeI(); ++i )
	{
		menuHandle_t posterHandle = MovieRoot->ChildHandleForId( menuMgr, VRMenuId_t( ID_MOVIE_POSTER_ROOTS.Get() + i ) );
		VRMenuObject *poster = menuMgr.ToObject( posterHandle );

		//
		// posters
		//
		VRMenuSurfaceParms posterSurfParms( "",
				selectionTexture, PosterWidth, PosterHeight, SURFACE_TEXTURE_DIFFUSE,
				0, 0, 0, SURFACE_TEXTURE_MAX,
				0, 0, 0, SURFACE_TEXTURE_MAX );

		Posef posterPose( Quatf( up, 0.0f ), Vector3f( 0.0f, 0.0f, 0.0f ) );
		VRMenuObjectParms * posterParms = new VRMenuObjectParms( VRMENU_BUTTON, Array< VRMenuComponent* >(),
				posterSurfParms, NULL, posterPose, PosterScale, fontParms, VRMenuId_t( ID_MOVIE_POSTERS.Get() + i ),
				VRMenuObjectFlags_t( VRMENUOBJECT_FLAG_NO_FOCUS_GAINED ), VRMenuObjectInitFlags_t( VRMENUOBJECT_INIT_FORCE_POSITION ) );

		parms.PushBack( posterParms );

		//
		// 3D icon
		//
		VRMenuSurfaceParms is3DIconSurfParms( "",
				is3DIconTexture, is3DIconWidth, is3DIconHeight, SURFACE_TEXTURE_DIFFUSE,
				0, 0, 0, SURFACE_TEXTURE_MAX,
				0, 0, 0, SURFACE_TEXTURE_MAX );

		Is3DIconPose = Posef( Quatf( up, 0.0f ), Vector3f( 0.75f, 1.3f, 0.02f ) );
		VRMenuObjectParms * is3DIconParms = new VRMenuObjectParms( VRMENU_BUTTON, Array< VRMenuComponent* >(),
				is3DIconSurfParms, NULL, Is3DIconPose, PosterScale, fontParms, VRMenuId_t( ID_3D_ICONS.Get() + i ),
				VRMenuObjectFlags_t( VRMENUOBJECT_FLAG_NO_FOCUS_GAINED ) | VRMenuObjectFlags_t( VRMENUOBJECT_DONT_HIT_ALL ), VRMenuObjectInitFlags_t( VRMENUOBJECT_INIT_FORCE_POSITION ) );

		parms.PushBack( is3DIconParms );

		//
		// shadow
		//
		VRMenuSurfaceParms shadowSurfParms( "",
				shadowTexture, shadowWidth, shadowHeight, SURFACE_TEXTURE_DIFFUSE,
				0, 0, 0, SURFACE_TEXTURE_MAX,
				0, 0, 0, SURFACE_TEXTURE_MAX );

		ShadowPose = Posef( Quatf( up, 0.0f ), Vector3f( 0.0f, -1.97f, 0.00f ) );
		VRMenuObjectParms * shadowParms = new VRMenuObjectParms( VRMENU_BUTTON, Array< VRMenuComponent* >(),
				shadowSurfParms, NULL, ShadowPose, PosterScale, fontParms, VRMenuId_t( ID_SHADOWS.Get() + i ),
				VRMenuObjectFlags_t( VRMENUOBJECT_FLAG_NO_FOCUS_GAINED ) | VRMenuObjectFlags_t( VRMENUOBJECT_DONT_HIT_ALL ), VRMenuObjectInitFlags_t( VRMENUOBJECT_INIT_FORCE_POSITION ) );

		parms.PushBack( shadowParms );

	    Menu->AddItems( menuMgr, font, parms, posterHandle, false );

	    DeletePointerArray( parms );
	    parms.Clear();

		menuHandle_t posterImageHandle = poster->ChildHandleForId( menuMgr, VRMenuId_t( ID_MOVIE_POSTERS.Get() + i ) );
		VRMenuObject *posterImage = menuMgr.ToObject( posterImageHandle );

		menuHandle_t is3DIconHandle = poster->ChildHandleForId( menuMgr, VRMenuId_t( ID_3D_ICONS.Get() + i ) );
		VRMenuObject *is3DIconMenuObject = menuMgr.ToObject( is3DIconHandle );

		menuHandle_t shadowHandle = poster->ChildHandleForId( menuMgr, VRMenuId_t( ID_SHADOWS.Get() + i ) );
		VRMenuObject *shadowMenuObject = menuMgr.ToObject( shadowHandle );

		posterImage->SetLocalBoundsExpand( SelectionBoundsExpandMin, SelectionBoundsExpandMax );

		if ( i == CenterIndex )
		{
			CenterPoster = posterImage;
		}

		//
		// add the component
		//
		MoviePosterComponent *posterComp = new MoviePosterComponent();
		posterComp->SetMenuObjects( poster, posterImage, is3DIconMenuObject, shadowMenuObject );
		posterImage->AddComponent( posterComp );

		menuObjs.PushBack( poster );
		MoviePosterComponents.PushBack( posterComp );
	}

	MovieBrowser->SetMenuObjects( menuObjs, MoviePosterComponents );

    // ==============================================================================
    //
    // category browser
    //
    Categories.PushBack( MovieCategoryButton( CATEGORY_TRAILERS, Strings::Category_Trailers ) );
    Categories.PushBack( MovieCategoryButton( CATEGORY_MYVIDEOS, Strings::Category_MyVideos ) );

    CategoryRootHandle = CenterRoot->ChildHandleForId( menuMgr, ID_CATEGORY_ROOT );
    CategoryRoot = menuMgr.ToObject( CategoryRootHandle );
	
    int borderWidth = 0, borderHeight = 0;
    GLuint borderTexture = LoadTextureFromApplicationPackage( "assets/category_border.png",
		TextureFlags_t( TEXTUREFLAG_NO_DEFAULT ), borderWidth, borderHeight );

    VRMenuFontParms categoryFontParms( true, false, false, false, false, 2.2f );

    float itemWidth = 1.10f;
    float centerOffset = ( itemWidth * ( Categories.GetSizeI() - 1 ) ) * 0.5f;
	for ( int i = 0; i < Categories.GetSizeI(); ++i )
	{
    	float x = itemWidth * i - centerOffset;
    	Categories[ i ].Pose = Posef( Quatf( up, 0.0f / 180.0f * Mathf::Pi ), Vector3f( x, 3.6f, -7.39f ) );

    	Array< VRMenuComponent* > panelComps;
		panelComps.PushBack( new MovieCategoryComponent( this, Categories[ i ].Category ) );

		VRMenuSurfaceParms panelSurfParms( "",
				borderTexture, borderWidth, borderHeight, SURFACE_TEXTURE_ADDITIVE,
				0, 0, 0, SURFACE_TEXTURE_MAX,
				0, 0, 0, SURFACE_TEXTURE_MAX );

		Posef textPose( Quatf(), up * -0.05f );
		Vector3f textScale( 1.0f );
		VRMenuObjectParms * p = new VRMenuObjectParms( VRMENU_BUTTON, panelComps,
				panelSurfParms, Categories[ i ].Text, Categories[ i ].Pose, defaultScale, 
				textPose, textScale, categoryFontParms, VRMenuId_t( ID_CATEGORY_ITEMS.Get() + i ),
				VRMenuObjectFlags_t(), VRMenuObjectInitFlags_t( VRMENUOBJECT_INIT_FORCE_POSITION ) );

		parms.PushBack( p );
	}

    Menu->AddItems( menuMgr, font, parms, CategoryRootHandle, false );
    DeletePointerArray( parms );
    parms.Clear();

    CategoryBoundsExpandMin = Vector3f( 0.0f, 0.13f, 0.0f );
    CategoryBoundsExpandMax = Vector3f( 0.0f );

	for ( int i = 0; i < Categories.GetSizeI(); ++i )
	{
		menuHandle_t categoryHandle = CategoryRoot->ChildHandleForId( menuMgr, VRMenuId_t( ID_CATEGORY_ITEMS.Get() + i ) );
		Categories[ i ].Button = menuMgr.ToObject( categoryHandle );
		Categories[ i ].Button->SetLocalBoundsExpand( CategoryBoundsExpandMin, CategoryBoundsExpandMax );
	}

	// ==============================================================================
    //
    // movie title
    //
    {
		MovieTitlePose = Posef( Quatf( up, 0.0f ), Vector3f( 0.0f, 0.045f, -7.37f ) );

	    VRMenuFontParms titleFontParms( true, true, false, false, false, 2.5f );

		VRMenuObjectParms titleParms( VRMENU_STATIC, Array< VRMenuComponent* >(),
				VRMenuSurfaceParms(), "", MovieTitlePose, defaultScale, titleFontParms, ID_MOVIE_TITLE,
				VRMenuObjectFlags_t( VRMENUOBJECT_DONT_HIT_ALL ), VRMenuObjectInitFlags_t( VRMENUOBJECT_INIT_FORCE_POSITION ) );

		parms.PushBack( &titleParms );

		TitleRootHandle = CenterRoot->ChildHandleForId(menuMgr, ID_MOVIE_TITLE_ROOT);
		TitleRoot = menuMgr.ToObject( TitleRootHandle );

		Menu->AddItems( menuMgr, font, parms, TitleRootHandle, false );
		parms.Clear();

		MovieTitleHandle = TitleRoot->ChildHandleForId( menuMgr, ID_MOVIE_TITLE );
		MovieTitle = menuMgr.ToObject( MovieTitleHandle );
    }

	// ==============================================================================
    //
    // swipe icons
    //
    {
		GLuint swipeIconLeftTexture = LoadTextureFromApplicationPackage( "assets/SwipeSuggestionArrowLeft.png",
				TextureFlags_t( TEXTUREFLAG_NO_DEFAULT ), SwipeIconLeftWidth, SwipeIconLeftHeight );

		GLuint swipeIconRightTexture = LoadTextureFromApplicationPackage( "assets/SwipeSuggestionArrowRight.png",
				TextureFlags_t( TEXTUREFLAG_NO_DEFAULT ), SwipeIconRightWidth, SwipeIconRightHeight );

		VRMenuSurfaceParms swipeIconLeftSurfParms( "",
				swipeIconLeftTexture, SwipeIconLeftWidth, SwipeIconLeftHeight, SURFACE_TEXTURE_DIFFUSE,
				0, 0, 0, SURFACE_TEXTURE_MAX,
				0, 0, 0, SURFACE_TEXTURE_MAX );

		VRMenuSurfaceParms swipeIconRightSurfParms( "",
				swipeIconRightTexture, SwipeIconRightWidth, SwipeIconRightHeight, SURFACE_TEXTURE_DIFFUSE,
				0, 0, 0, SURFACE_TEXTURE_MAX,
				0, 0, 0, SURFACE_TEXTURE_MAX );

		float yPos = 1.76f - ( PosterHeight - SwipeIconLeftHeight ) * 0.5f * VRMenuObject::DEFAULT_TEXEL_SCALE * PosterScale.y;
		SwipeIconPose = Posef( Quatf( up, 0.0f ), Vector3f( 0.0f, yPos, -7.17f ) );
		for( int i = 0; i < NumSwipeTrails; i++ )
		{
			Posef swipePose = SwipeIconPose;
			swipePose.Position.x = ( ( PosterWidth + SwipeIconLeftWidth * ( i + 2 ) ) * -0.5f ) * VRMenuObject::DEFAULT_TEXEL_SCALE * PosterScale.x;
			swipePose.Position.z += 0.01f * ( float )i;

			Array< VRMenuComponent* > leftComps;
			leftComps.PushBack( new SwipeHintComponent( MovieBrowser, false, 1.3333f, 0.4f + ( float )i * 0.13333f, 5.0f ) );

			VRMenuObjectParms * swipeIconLeftParms = new VRMenuObjectParms( VRMENU_BUTTON, leftComps,
				swipeIconLeftSurfParms, "", swipePose, PosterScale, fontParms, VRMenuId_t( ID_SWIPE_ICON_LEFT.Get() + i ),
				VRMenuObjectFlags_t( VRMENUOBJECT_FLAG_NO_DEPTH ) | VRMenuObjectFlags_t( VRMENUOBJECT_DONT_HIT_ALL ),
				VRMenuObjectInitFlags_t( VRMENUOBJECT_INIT_FORCE_POSITION ) );

			parms.PushBack( swipeIconLeftParms );

			swipePose.Position.x = ( ( PosterWidth + SwipeIconRightWidth * ( i + 2 ) ) * 0.5f ) * VRMenuObject::DEFAULT_TEXEL_SCALE * PosterScale.x;

			Array< VRMenuComponent* > rightComps;
			rightComps.PushBack( new SwipeHintComponent( MovieBrowser, true, 1.3333f, 0.4f + ( float )i * 0.13333f, 5.0f ) );

			VRMenuObjectParms * swipeIconRightParms = new VRMenuObjectParms( VRMENU_STATIC, rightComps,
				swipeIconRightSurfParms, "", swipePose, PosterScale, fontParms, VRMenuId_t( ID_SWIPE_ICON_RIGHT.Get() + i ),
				VRMenuObjectFlags_t( VRMENUOBJECT_FLAG_NO_DEPTH ) | VRMenuObjectFlags_t( VRMENUOBJECT_DONT_HIT_ALL ),
				VRMenuObjectInitFlags_t( VRMENUOBJECT_INIT_FORCE_POSITION ) );

			parms.PushBack( swipeIconRightParms );
		}

		Menu->AddItems( menuMgr, font, parms, CenterRootHandle, false );
		DeletePointerArray( parms );
		parms.Clear();
    }

	// ==============================================================================
    //
    // resume icon
    //
    {
		int resumeIconWidth = 0, resumeIconHeight = 0;
		GLuint resumeIconTexture = LoadTextureFromApplicationPackage( "assets/resume.png", TextureFlags_t( TEXTUREFLAG_NO_DEFAULT ), resumeIconWidth, resumeIconHeight );

		VRMenuFontParms resumeFontParms( true, true, false, false, false, 0.3f );

		ResumeIconPose = Posef( Quatf( up, 0.0f ), Vector3f( 0.0f, 0.0f, 0.5f ) );

		VRMenuSurfaceParms resumeIconSurfParms( "",
				resumeIconTexture, resumeIconWidth, resumeIconHeight, SURFACE_TEXTURE_DIFFUSE,
				0, 0, 0, SURFACE_TEXTURE_MAX,
				0, 0, 0, SURFACE_TEXTURE_MAX );

		ResumeIconScale = 3.0f;

		VRMenuObjectParms resumeIconParms( VRMENU_STATIC, Array< VRMenuComponent* >(),
				resumeIconSurfParms, Strings::MovieSelection_Resume, ResumeIconPose, Vector3f( ResumeIconScale ), resumeFontParms, ID_RESUME_ICON,
				VRMenuObjectFlags_t( VRMENUOBJECT_DONT_HIT_ALL ), VRMenuObjectInitFlags_t( VRMENUOBJECT_INIT_FORCE_POSITION ) );

		parms.PushBack( &resumeIconParms );

		Menu->AddItems( menuMgr, font, parms, SelectionHandle, false );
		parms.Clear();

		ResumeIconHandle = SelectionObject->ChildHandleForId( menuMgr, ID_RESUME_ICON );
		ResumeIcon = menuMgr.ToObject( ResumeIconHandle );
		ResumeIcon->AddFlags( VRMENUOBJECT_DONT_RENDER );
		ResumeTextPosition = Vector3f( 0.0f, -resumeIconHeight * VRMenuObject::DEFAULT_TEXEL_SCALE * 2.0f, 0.0f );
		ResumeIcon->SetTextLocalPosition( ResumeTextPosition );
    }

    // ==============================================================================
    //
    // timer
    //
    {
		VRMenuFontParms timerFontParms( true, true, false, false, false, 1.0f );

		TimerIconOffset = Vector3f( 0.0f, 0.0f, 0.5f );
		Posef timerIconPose( MoviePanelPositions[ CenterIndex ].Orientation, MoviePanelPositions[ CenterIndex ].Position + TimerIconOffset );

		VRMenuSurfaceParms timerSurfaceParms( "timer",
			"assets/timer.tga", SURFACE_TEXTURE_DIFFUSE,
			"assets/timer_fill.tga", SURFACE_TEXTURE_COLOR_RAMP_TARGET,
			"assets/color_ramp_timer.tga", SURFACE_TEXTURE_COLOR_RAMP );

		TimerIconScale = 2.0f;

		VRMenuObjectParms timerIconParms( VRMENU_STATIC, Array< VRMenuComponent* >(),
				timerSurfaceParms, "10", timerIconPose, Vector3f( TimerIconScale ), timerFontParms, ID_TIMER_ICON,
				VRMenuObjectFlags_t( VRMENUOBJECT_DONT_HIT_ALL ) | VRMenuObjectFlags_t( VRMENUOBJECT_DONT_RENDER ),
				VRMenuObjectInitFlags_t( VRMENUOBJECT_INIT_FORCE_POSITION ) );

		parms.PushBack( &timerIconParms );

		Menu->AddItems( menuMgr, font, parms, MovieRootHandle, false );
		parms.Clear();

		TimerIconHandle = MovieRoot->ChildHandleForId( menuMgr, ID_TIMER_ICON );
		TimerIcon = menuMgr.ToObject( TimerIconHandle );

		// text
		TimerTextPose = Posef( Quatf( up, 0.0f ), Vector3f( 0.0f, -0.5f, 0.0f ) );

		VRMenuSurfaceParms timerTextSurfaceParms( "",
				0, 0, 0, SURFACE_TEXTURE_MAX,
				0, 0, 0, SURFACE_TEXTURE_MAX,
				0, 0, 0, SURFACE_TEXTURE_MAX );

		VRMenuObjectParms timerTextParms( VRMENU_STATIC, Array< VRMenuComponent* >(),
				timerTextSurfaceParms, Strings::MovieSelection_Next, TimerTextPose, defaultScale, timerFontParms, ID_TIMER_TEXT,
				VRMenuObjectFlags_t( VRMENUOBJECT_DONT_HIT_ALL ), VRMenuObjectInitFlags_t( VRMENUOBJECT_INIT_FORCE_POSITION ) );

		parms.PushBack( &timerTextParms );

		Menu->AddItems( menuMgr, font, parms, TimerIconHandle, false );
		parms.Clear();

		TimerTextHandle = SelectionObject->ChildHandleForId( menuMgr, ID_TIMER_TEXT );
		TimerText = menuMgr.ToObject( TimerIconHandle );
    }

    // ==============================================================================
    //
    // reorient message
    //
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

	Menu->AddItems( menuMgr, font, parms, Menu->GetRootHandle(), false );
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

Vector3f MovieSelectionView::ScalePosition( const Vector3f &startPos, const float scale, const float menuOffset ) const
{
	const float eyeHieght = Cinema.SceneMgr.Scene.ViewParms.EyeHeight;

	Vector3f pos = startPos;
	pos.x *= scale;
	pos.y = ( pos.y - eyeHieght ) * scale + eyeHieght + menuOffset;
	pos.z *= scale;
	pos += Cinema.SceneMgr.Scene.FootPos;

	return pos;
}

//
// Repositions the menu for the lobby scene or the theater
//
void MovieSelectionView::UpdateMenuPosition()
{
	OvrVRMenuMgr & menuMgr = Cinema.app->GetVRMenuMgr();

	// scale down when in a theater
	const float scale = Cinema.InLobby ? 1.0f : 0.55f;

	float menuOffset;
	if ( Cinema.InLobby )
	{
		menuOffset = 0.0f;
	}
	else if ( Cinema.SceneMgr.SceneInfo.UseFreeScreen )
	{
		menuOffset = 0.0f;
	}
	else
	{
		menuOffset = 0.5f;
	}

	Vector3f posterScale = PosterScale * scale;
	Array<PanelPose> moviePanels = MoviePanelPositions;
	for ( int i = 0; i < moviePanels.GetSizeI(); ++i )
	{
		moviePanels[ i ].Position = ScalePosition( moviePanels[ i ].Position, scale, menuOffset );

		menuHandle_t posterHandle = MovieRoot->ChildHandleForId( menuMgr, VRMenuId_t( ID_MOVIE_POSTER_ROOTS.Get() + i ) );
		VRMenuObject *poster = menuMgr.ToObject( posterHandle );

		menuHandle_t posterImageHandle = poster->ChildHandleForId( menuMgr, VRMenuId_t( ID_MOVIE_POSTERS.Get() + i ) );
		VRMenuObject *posterImage = menuMgr.ToObject( posterImageHandle );

		posterImage->SetLocalScale( posterScale );
		posterImage->SetLocalBoundsExpand( SelectionBoundsExpandMin * scale, SelectionBoundsExpandMax * scale );

		menuHandle_t is3DIconHandle = poster->ChildHandleForId( menuMgr, VRMenuId_t( ID_3D_ICONS.Get() + i ) );
		VRMenuObject *is3DIconMenuObject = menuMgr.ToObject( is3DIconHandle );

		is3DIconMenuObject->SetLocalScale( posterScale );
		is3DIconMenuObject->SetLocalPosition( Is3DIconPose.Position * scale );

		menuHandle_t shadowHandle = poster->ChildHandleForId( menuMgr, VRMenuId_t( ID_SHADOWS.Get() + i ) );
		VRMenuObject *shadowMenuObject = menuMgr.ToObject( shadowHandle );

		shadowMenuObject->SetLocalScale( posterScale );
		shadowMenuObject->SetLocalPosition( ShadowPose.Position * scale );
	}

	MovieBrowser->SetPanelPoses( menuMgr, MovieRoot, moviePanels );
	CenterPosition = moviePanels[ CenterIndex ].Position;

	SelectionObject->SetLocalPosition( CenterPosition +	SelectionOffset * scale );
	SelectionObject->SetLocalScale( PosterScale * scale );
	SelectionObject->SetLocalBoundsExpand( SelectionBoundsExpandMin * scale, SelectionBoundsExpandMax * scale );

	Vector3f swipePos = SwipeIconPose.Position;
	for( int i = 0; i < NumSwipeTrails; i++ )
	{
		menuHandle_t swipeLeftHandle = CenterRoot->ChildHandleForId( menuMgr, VRMenuId_t( ID_SWIPE_ICON_LEFT.Get() + i ) );
		VRMenuObject *swipeLeftObject = menuMgr.ToObject( swipeLeftHandle );
		swipePos.x = ( ( PosterWidth + SwipeIconLeftWidth * ( i + 2 ) ) * -0.5f ) * VRMenuObject::DEFAULT_TEXEL_SCALE * PosterScale.x;
		swipePos.z += 0.01f * ( float )i;
		swipeLeftObject->SetLocalPosition( ScalePosition( swipePos, scale, menuOffset ) );
		swipeLeftObject->SetLocalScale( PosterScale * scale );

		menuHandle_t swipeRightHandle = CenterRoot->ChildHandleForId( menuMgr, VRMenuId_t( ID_SWIPE_ICON_RIGHT.Get() + i ) );
		VRMenuObject *swipeRightObject = menuMgr.ToObject( swipeRightHandle );
		swipePos.x = ( ( PosterWidth + SwipeIconRightWidth * ( i + 2 ) ) * 0.5f ) * VRMenuObject::DEFAULT_TEXEL_SCALE * PosterScale.x;
		swipeRightObject->SetLocalPosition( ScalePosition( swipePos, scale, menuOffset ) );
		swipeRightObject->SetLocalScale( PosterScale * scale );
	}

	for ( int i = 0; i < Categories.GetSizeI(); ++i )
	{

		Categories[ i ].Button->SetLocalPosition( ScalePosition( Categories[ i ].Pose.Position, scale, menuOffset ) );
		Categories[ i ].Button->SetLocalScale( Vector3f( scale ) );
		Categories[ i ].Button->SetLocalBoundsExpand( CategoryBoundsExpandMin * scale, CategoryBoundsExpandMax * scale );
	}

	MovieTitle->SetLocalPosition( ScalePosition( MovieTitlePose.Position, scale, menuOffset ) );
	MovieTitle->SetLocalScale( Vector3f( scale ) );

	ResumeIcon->SetLocalScale( Vector3f( ResumeIconScale * scale ) );
	ResumeIcon->SetLocalPosition( ResumeIconPose.Position * scale );

	TimerIcon->SetLocalScale( Vector3f( TimerIconScale * scale ) );
	TimerIcon->SetLocalPosition( CenterPosition + TimerIconOffset * scale );

	ErrorMessage->SetLocalPosition( ScalePosition( ErrorMessagePose.Position, scale, menuOffset ) );
	ErrorMessage->SetLocalScale( Vector3f( ErrorMessageScale * scale ) );
	ErrorMessage->SetTextLocalPosition( ErrorMessageTextPosition * scale );

	SDCardMessage->SetLocalPosition( ScalePosition( SDCardMessagePose.Position, scale, menuOffset ) );
	SDCardMessage->SetLocalScale( Vector3f( SDCardMessageScale * scale ) );
	SDCardMessage->SetTextLocalPosition( SDCardMessageTextPosition * scale );

	PlainErrorMessage->SetLocalPosition( ScalePosition( PlainErrorMessagePose.Position, scale, menuOffset ) );
	PlainErrorMessage->SetLocalScale( Vector3f( PlainErrorMessageScale * scale ) );
	PlainErrorMessage->SetTextLocalPosition( PlainErrorMessageTextPosition * scale );

	if ( !Cinema.InLobby && Cinema.SceneMgr.SceneInfo.UseFreeScreen )
	{
		Quatf orientation = Quatf( Cinema.SceneMgr.FreeScreenOrientation );
		CenterRoot->SetLocalRotation( orientation );
		CenterRoot->SetLocalPosition( Cinema.SceneMgr.FreeScreenOrientation.Transform( Vector3f( 0.0f, -1.76f, 0.0f ) ) );
	}
	else
	{
		CenterRoot->SetLocalRotation( Quatf() );
		CenterRoot->SetLocalPosition( Vector3f::ZERO );
	}
}

//============================================================================================

void MovieSelectionView::SelectMovie()
{
	LOG( "SelectMovie");

	// ignore selection while repositioning screen
	if ( RepositionScreen )
	{
		return;
	}

	int lastIndex = MoviesIndex;
	MoviesIndex = MovieBrowser->GetSelection();
	Cinema.SetPlaylist( MovieList, MoviesIndex );

	if ( !Cinema.InLobby )
	{
		if ( lastIndex == MoviesIndex )
		{
			// selected the poster we were just watching
			Cinema.ResumeMovieFromSavedLocation();
		}
		else
		{
			Cinema.ResumeOrRestartMovie();
		}
	}
	else
	{
		Cinema.TheaterSelection();
	}
}

void MovieSelectionView::StartTimer()
{
	const double now = TimeInSeconds();
	TimerStartTime = now;
	TimerValue = -1;
	ShowTimer = true;
}

const MovieDef *MovieSelectionView::GetSelectedMovie() const
{
	int selectedItem = MovieBrowser->GetSelection();
	if ( ( selectedItem >= 0 ) && ( selectedItem < MovieList.GetSizeI() ) )
	{
		return MovieList[ selectedItem ];
	}

	return NULL;
}

void MovieSelectionView::SetMovieList( const Array<const MovieDef *> &movies, const MovieDef *nextMovie )
{
	LOG( "SetMovieList: %d movies", movies.GetSizeI() );

	MovieList = movies;
	DeletePointerArray( MovieBrowserItems );
	for( UPInt i = 0; i < MovieList.GetSize(); i++ )
	{
		const MovieDef *movie = MovieList[ i ];

		LOG( "AddMovie: %s", movie->Filename.ToCStr() );

		CarouselItem *item = new CarouselItem();
		item->texture 		= movie->Poster;
		item->textureWidth 	= movie->PosterWidth;
		item->textureHeight	= movie->PosterHeight;
		item->userFlags 	= movie->Is3D ? 1 : 0;
		MovieBrowserItems.PushBack( item );
	}
	MovieBrowser->SetItems( MovieBrowserItems );

	MovieTitle->SetText( "" );
	LastMovieDisplayed = NULL;

	MoviesIndex = 0;
	if ( nextMovie != NULL )
	{
		for( int i = 0; i < MovieList.GetSizeI(); i++ )
		{
			if ( movies[ i ] == nextMovie )
			{
				StartTimer();
				MoviesIndex = i;
				break;
			}
		}
	}

	MovieBrowser->SetSelectionIndex( MoviesIndex );

	if ( MovieList.GetSizeI() == 0 )
	{
		if ( CurrentCategory == CATEGORY_MYVIDEOS )
		{
			SetError( Strings::Error_NoVideosInMyVideos, false, false );
		}
		else
		{
			SetError( Strings::Error_NoVideosOnPhone, true, false );
		}
	}
	else
	{
		ClearError();
	}
}

void MovieSelectionView::SetCategory( const MovieCategory category )
{
	// default to category in index 0
	int categoryIndex = 0;
	for( int i = 0; i < Categories.GetSizeI(); i++ )
	{
		if ( category == Categories[ i ].Category )
		{
			categoryIndex = i;
			break;
		}
	}

	LOG( "SetCategory: %s", Categories[ categoryIndex ].Text );
	CurrentCategory = Categories[ categoryIndex ].Category;
	for ( int i = 0; i < Categories.GetSizeI(); ++i )
	{
		Categories[ i ].Button->SetHilighted( i == categoryIndex );
	}

	// reset all the swipe icons so they match the current poster
	for( int i = 0; i < NumSwipeTrails; i++ )
	{
		menuHandle_t swipeLeftHandle = CenterRoot->ChildHandleForId( Cinema.app->GetVRMenuMgr(), VRMenuId_t( ID_SWIPE_ICON_LEFT.Get() + i ) );
		VRMenuObject *swipeLeftObject = Cinema.app->GetVRMenuMgr().ToObject( swipeLeftHandle );
		SwipeHintComponent * compLeft = swipeLeftObject->GetComponentByName<SwipeHintComponent>();
		compLeft->Reset( swipeLeftObject );

		menuHandle_t swipeRightHandle = CenterRoot->ChildHandleForId( Cinema.app->GetVRMenuMgr(), VRMenuId_t( ID_SWIPE_ICON_RIGHT.Get() + i ) );
		VRMenuObject *swipeRightObject = Cinema.app->GetVRMenuMgr().ToObject( swipeRightHandle );
		SwipeHintComponent * compRight = swipeRightObject->GetComponentByName<SwipeHintComponent>();
		compRight->Reset( swipeRightObject );
	}

	SetMovieList( Cinema.MovieMgr.GetMovieList( CurrentCategory ), NULL );

	LOG( "%d movies added", MovieList.GetSizeI() );
}

void MovieSelectionView::UpdateMovieTitle()
{
	const MovieDef * currentMovie = GetSelectedMovie();
	if ( LastMovieDisplayed != currentMovie )
	{
		if ( currentMovie != NULL )
		{
			MovieTitle->SetText( currentMovie->Title.ToCStr() );
		}
		else
		{
			MovieTitle->SetText( "" );
		}

		LastMovieDisplayed = currentMovie;
	}
}

void MovieSelectionView::SelectionHighlighted( bool isHighlighted )
{
	if ( isHighlighted && !ShowTimer && !Cinema.InLobby && ( MoviesIndex == MovieBrowser->GetSelection() ) )
	{
		// dim the poster when the resume icon is up and the poster is highlighted
		CenterPoster->SetColor( Vector4f( 0.55f, 0.55f, 0.55f, 1.0f ) );
	}
	else if ( MovieBrowser->HasSelection() )
	{
		CenterPoster->SetColor( Vector4f( 1.0f ) );
	}
}

void MovieSelectionView::UpdateSelectionFrame( const VrFrame & vrFrame )
{
	const double now = TimeInSeconds();
	if ( !MovieBrowser->HasSelection() )
	{
		SelectionFader.Set( now, 0, now + 0.1, 1.0f );
		TimerStartTime = 0;
	}

	if ( !SelectionObject->IsHilighted() )
	{
		SelectionFader.Set( now, 0, now + 0.1, 1.0f );
	}
	else
	{
		MovieBrowser->CheckGamepad( Cinema.app, vrFrame, Cinema.app->GetVRMenuMgr(), MovieRoot );
	}

	SelectionObject->SetColor( Vector4f( SelectionFader.Value( now ) ) );

	if ( !ShowTimer && !Cinema.InLobby && ( MoviesIndex == MovieBrowser->GetSelection() ) )
	{
		ResumeIcon->SetColor( Vector4f( SelectionFader.Value( now ) ) );
		ResumeIcon->SetTextColor( Vector4f( SelectionFader.Value( now ) ) );
		ResumeIcon->RemoveFlags( VRMENUOBJECT_DONT_RENDER );
	}
	else
	{
		ResumeIcon->AddFlags( VRMENUOBJECT_DONT_RENDER );
	}

	if ( ShowTimer && ( TimerStartTime != 0 ) )
	{
		double frac = ( now - TimerStartTime ) / TimerTotalTime;
		if ( frac > 1.0f )
		{
			frac = 1.0f;
			Cinema.SetPlaylist( MovieList, MovieBrowser->GetSelection() );
			Cinema.ResumeOrRestartMovie();
		}
		Vector2f offset( 0.0f, 1.0f - frac );
		TimerIcon->SetColorTableOffset( offset );

		int seconds = TimerTotalTime - ( int )( TimerTotalTime * frac );
		if ( TimerValue != seconds )
		{
			TimerValue = seconds;
			const char * text = StringUtils::Va( "%d", seconds );
			TimerIcon->SetText( text );
		}
		TimerIcon->RemoveFlags( VRMENUOBJECT_DONT_RENDER );
		CenterPoster->SetColor( Vector4f( 0.55f, 0.55f, 0.55f, 1.0f ) );
	}
	else
	{
		TimerIcon->AddFlags( VRMENUOBJECT_DONT_RENDER );
	}
}

void MovieSelectionView::SetError( const char *text, bool showSDCard, bool showErrorIcon )
{
	ClearError();

	LOG( "SetError: %s", text );
	if ( showSDCard )
	{
		SDCardMessage->RemoveFlags( VRMENUOBJECT_DONT_RENDER );
		SDCardMessage->SetText( text );
	}
	else if ( showErrorIcon )
	{
		ErrorMessage->RemoveFlags( VRMENUOBJECT_DONT_RENDER );
		ErrorMessage->SetText( text );
	}
	else
	{
		PlainErrorMessage->RemoveFlags( VRMENUOBJECT_DONT_RENDER );
		PlainErrorMessage->SetText( text );
	}
	TitleRoot->AddFlags( VRMENUOBJECT_DONT_RENDER );
    MovieRoot->AddFlags( VRMENUOBJECT_DONT_RENDER );

    SwipeHintComponent::ShowSwipeHints = false;
}

void MovieSelectionView::ClearError()
{
	LOG( "ClearError" );
	ErrorMessageClicked = false;
	ErrorMessage->AddFlags( VRMENUOBJECT_DONT_RENDER );
	SDCardMessage->AddFlags( VRMENUOBJECT_DONT_RENDER );
	PlainErrorMessage->AddFlags( VRMENUOBJECT_DONT_RENDER );
	TitleRoot->RemoveFlags( VRMENUOBJECT_DONT_RENDER );
    MovieRoot->RemoveFlags( VRMENUOBJECT_DONT_RENDER );

    SwipeHintComponent::ShowSwipeHints = true;
}

bool MovieSelectionView::ErrorShown() const
{
	return ( ErrorMessage->GetFlags() & VRMENUOBJECT_DONT_RENDER ) == 0;
}

Matrix4f MovieSelectionView::DrawEyeView( const int eye, const float fovDegrees )
{
	return Cinema.SceneMgr.DrawEyeView( eye, fovDegrees );
}

Matrix4f MovieSelectionView::Frame( const VrFrame & vrFrame )
{
	// We want 4x MSAA in the lobby
	EyeParms eyeParms = Cinema.app->GetEyeParms();
	eyeParms.multisamples = 4;
	Cinema.app->SetEyeParms( eyeParms );

#if 0
	if ( !Cinema.InLobby && Cinema.SceneMgr.ChangeSeats( vrFrame ) )
	{
		UpdateMenuPosition();
	}
#endif

	if ( Menu->IsOpen() )
	{
		HasOpened = true;
	}

	if ( vrFrame.Input.buttonPressed & BUTTON_B )
	{
		if ( Cinema.InLobby )
		{
			Cinema.app->StartPlatformUI( PUI_CONFIRM_QUIT );
		}
		else
		{
			Cinema.app->GetGuiSys().CloseMenu( Cinema.app, Menu, false );
		}
	}

	// check if they closed the menu with the back button
	if ( !Cinema.InLobby && Menu->IsClosedOrClosing() && HasOpened )
	{
		// if we finished the movie or have an error, don't resume it, go back to the lobby
		if ( ErrorShown() )
		{
			LOG( "Error closed.  Return to lobby." );
			ClearError();
			Cinema.MovieSelection( true );
		}
		else if ( Cinema.IsMovieFinished() )
		{
			LOG( "Movie finished.  Return to lobby." );
			Cinema.MovieSelection( true );
		}
		else
		{
			LOG( "Resume movie." );
			Cinema.ResumeMovieFromSavedLocation();
		}
	}

	if ( !Cinema.InLobby && ErrorShown() )
	{
		if ( vrFrame.Input.buttonPressed & ( BUTTON_TOUCH | BUTTON_A ) )
		{
			Cinema.app->PlaySound( "touch_down" );
		}
		else if ( vrFrame.Input.buttonReleased & ( BUTTON_TOUCH | BUTTON_A ) )
		{
			Cinema.app->PlaySound( "touch_up" );
			ErrorMessageClicked = true;
		}
		else if ( ErrorMessageClicked && ( ( vrFrame.Input.buttonState & ( BUTTON_TOUCH | BUTTON_A ) ) == 0 ) )
		{
			Menu->Close( Cinema.app, Cinema.app->GetGazeCursor(), true );
		}
	}

	if ( Cinema.SceneMgr.FreeScreenActive )
	{
		if ( !RepositionScreen && !SelectionObject->IsHilighted() )
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
				// disable hit detection on selection frame
				SelectionObject->AddFlags( VRMENUOBJECT_DONT_HIT_ALL );
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

		const Matrix4f invViewMatrix = Cinema.SceneMgr.Scene.CenterViewMatrix().Inverted();
		const Vector3f viewPos( GetViewMatrixPosition( Cinema.SceneMgr.Scene.CenterViewMatrix() ) );
		const Vector3f viewFwd( GetViewMatrixForward( Cinema.SceneMgr.Scene.CenterViewMatrix() ) );

		// spawn directly in front
		Quatf rotation( -viewFwd, 0.0f );
		Quatf viewRot( invViewMatrix );
		Quatf fullRotation = rotation * viewRot;

		const float menuDistance = 1.45f;
		Vector3f position( viewPos + viewFwd * menuDistance );

		MoveScreenObj->SetLocalPose( Posef( fullRotation, position ) );
	}

	// while we're holding down the button or touchpad, reposition screen
	if ( RepositionScreen )
	{
		if ( vrFrame.Input.buttonState & ( BUTTON_A | BUTTON_TOUCH ) )
		{
			Cinema.SceneMgr.PutScreenInFront();
			Quatf orientation = Quatf( Cinema.SceneMgr.FreeScreenOrientation );
			CenterRoot->SetLocalRotation( orientation );
			CenterRoot->SetLocalPosition( Cinema.SceneMgr.FreeScreenOrientation.Transform( Vector3f( 0.0f, -1.76f, 0.0f ) ) );
		}
		else
		{
			RepositionScreen = false;
		}
	}
	else
	{
		// reenable hit detection on selection frame.
		// note: we do this on the frame following the frame we disabled RepositionScreen on
		// so that the selection object doesn't get the touch up.
		SelectionObject->RemoveFlags( VRMENUOBJECT_DONT_HIT_ALL );
	}

	UpdateMovieTitle();
	UpdateSelectionFrame( vrFrame );

	return Cinema.SceneMgr.Frame( vrFrame );
}

} // namespace OculusCinema
