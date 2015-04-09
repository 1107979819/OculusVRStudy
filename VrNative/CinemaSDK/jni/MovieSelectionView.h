/************************************************************************************

Filename    :   MovieSelectionView.h
Content     :
Created     :	6/19/2014
Authors     :   Jim Dosé

Copyright   :   Copyright 2014 Oculus VR, LLC. All Rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the Cinema/ directory. An additional grant 
of patent rights can be found in the PATENTS file in the same directory.

*************************************************************************************/

#if !defined( MovieSelectionView_h )
#define MovieSelectionView_h

#include "LibOVR/Src/Kernel/OVR_Array.h"
#include "Lerp.h"
#include "View.h"
#include "CarouselBrowserComponent.h"
#include "MovieManager.h"

using namespace OVR;

namespace OculusCinema {

class CinemaApp;
class CarouselBrowserComponent;
class MovieSelectionComponent;

class MovieSelectionView : public View
{
public:
										MovieSelectionView( CinemaApp &cinema );
	virtual 							~MovieSelectionView();

	virtual void 						OneTimeInit( const char * launchIntent );
	virtual void						OneTimeShutdown();

	virtual void 						OnOpen();
	virtual void 						OnClose();

	virtual Matrix4f 					DrawEyeView( const int eye, const float fovDegrees );
	virtual Matrix4f 					Frame( const VrFrame & vrFrame );
	virtual bool						Command( const char * msg );
	virtual bool 						OnKeyEvent( const int keyCode, const KeyState::eKeyEventType eventType );

    void 								SetMovieList( const Array<const MovieDef *> &movies, const MovieDef *nextMovie );

	void 								SelectMovie( void );
	void 								SelectionHighlighted( bool isHighlighted );
    void 								SetCategory( const MovieCategory category );
	void								SetError( const char *text, bool showSDCard, bool showErrorIcon );
	void								ClearError();

private:
    class MovieCategoryButton
    {
    public:
    	MovieCategory 	Category;
    	const char *	Text;
    	Posef			Pose;
    	VRMenuObject *	Button;

    					MovieCategoryButton( const MovieCategory category, const char * text ) :
    						Category( category ), Text( text ), Pose(), Button( NULL ) {}
    };

private:
    CinemaApp &							Cinema;

    static const VRMenuId_t  			ID_CENTER_ROOT;
    static const VRMenuId_t  			ID_ERROR_MESSAGE;
    static const VRMenuId_t  			ID_SDCARD_MESSAGE;
    static const VRMenuId_t  			ID_PLAIN_ERROR_MESSAGE;
	static const VRMenuId_t  			ID_MOVIE_SELECTION;
    static const VRMenuId_t  			ID_MOVIE_ROOT;
    static const VRMenuId_t  			ID_MOVIE_POSTER_ROOTS;
    static const VRMenuId_t  			ID_MOVIE_POSTERS;
    static const VRMenuId_t  			ID_CATEGORY_ROOT;
    static const VRMenuId_t				ID_CATEGORY_ITEMS;
    static const VRMenuId_t				ID_MOVIE_TITLE_ROOT;
    static const VRMenuId_t				ID_MOVIE_TITLE;
	static const VRMenuId_t 			ID_SWIPE_ICON_LEFT;
	static const VRMenuId_t 			ID_SWIPE_ICON_RIGHT;
	static const VRMenuId_t 			ID_RESUME_ICON;
	static const VRMenuId_t 			ID_TIMER_ICON;
	static const VRMenuId_t 			ID_TIMER_TEXT;
    static const VRMenuId_t 			ID_3D_ICONS;
    static const VRMenuId_t 			ID_SHADOWS;
    static const VRMenuId_t 			ID_MOVE_SCREEN;

	VRMenu *							Menu;

	menuHandle_t						CenterRootHandle;
	VRMenuObject *						CenterRoot;

	menuHandle_t						ErrorMessageHandle;
	VRMenuObject * 						ErrorMessage;
	Posef								ErrorMessagePose;
	float								ErrorMessageScale;
	Vector3f							ErrorMessageTextPosition;
	
	bool								ErrorMessageClicked;

	menuHandle_t						SDCardMessageHandle;
	VRMenuObject * 						SDCardMessage;
	Posef								SDCardMessagePose;
	float								SDCardMessageScale;
	Vector3f							SDCardMessageTextPosition;

	menuHandle_t						PlainErrorMessageHandle;
	VRMenuObject * 						PlainErrorMessage;
	Posef								PlainErrorMessagePose;
	float								PlainErrorMessageScale;
	Vector3f							PlainErrorMessageTextPosition;

	menuHandle_t						MovieRootHandle;
    VRMenuObject *						MovieRoot;

    menuHandle_t						CategoryRootHandle;
    VRMenuObject *						CategoryRoot;
    Vector3f							CategoryBoundsExpandMin;
    Vector3f							CategoryBoundsExpandMax;

    menuHandle_t						TitleRootHandle;
	VRMenuObject *						TitleRoot;

	menuHandle_t						MovieTitleHandle;
	VRMenuObject * 						MovieTitle;
	Posef								MovieTitlePose;

	menuHandle_t						SelectionHandle;
	VRMenuObject *						SelectionObject;
	MovieSelectionComponent *			SelectionComponent;
	Vector3f							SelectionOffset;
	Vector3f							SelectionBoundsExpandMin;
	Vector3f							SelectionBoundsExpandMax;

	VRMenuObject *						CenterPoster;
	int									CenterIndex;
	Vector3f							CenterPosition;
	Posef 								Is3DIconPose;
	Posef 								ShadowPose;

	Posef								SwipeIconPose;
	int 								SwipeIconLeftWidth;
	int									SwipeIconLeftHeight;
	int 								SwipeIconRightWidth;
	int									SwipeIconRightHeight;

	menuHandle_t						ResumeIconHandle;
	VRMenuObject *						ResumeIcon;
	Posef								ResumeIconPose;
	float								ResumeIconScale;
	Vector3f							ResumeTextPosition;

	menuHandle_t						TimerIconHandle;
	VRMenuObject *						TimerIcon;
	Vector3f							TimerIconOffset;
	float								TimerIconScale;
	double								TimerStartTime;
	int									TimerValue;
	bool								ShowTimer;

	menuHandle_t						TimerTextHandle;
	VRMenuObject *						TimerText;
	Posef								TimerTextPose;

	menuHandle_t						MoveScreenHandle;
	VRMenuObject *						MoveScreenObj;
	Lerp								MoveScreenAlpha;

	Lerp								SelectionFader;

	CarouselBrowserComponent *			MovieBrowser;
	Array<CarouselItem *> 				MovieBrowserItems;
	Array<PanelPose>					MoviePanelPositions;

    Array<CarouselItemComponent *>	 	MoviePosterComponents;

	Array<MovieCategoryButton>			Categories;
    MovieCategory			 			CurrentCategory;
	
	Array<const MovieDef *> 			MovieList;
	int									MoviesIndex;

	const MovieDef *					LastMovieDisplayed;

	bool								HasOpened;
	bool								RepositionScreen;

private:
	const MovieDef *					GetSelectedMovie() const;

	void 								CreateMenu( App * app, OvrVRMenuMgr & menuMgr, BitmapFont const & font );
	Vector3f 							ScalePosition( const Vector3f &startPos, const float scale, const float menuOffset ) const;
	void 								UpdateMenuPosition();

	void								StartTimer();

	void								UpdateMovieTitle();
	void								UpdateSelectionFrame( const VrFrame & vrFrame );

	bool								ErrorShown() const;
};

} // namespace OculusCinema

#endif // MovieSelectionView_h
