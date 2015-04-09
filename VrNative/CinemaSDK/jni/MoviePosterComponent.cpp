/************************************************************************************

Filename    :   MoviePosterComponent.cpp
Content     :   Menu component for the movie selection menu.
Created     :   August 13, 2014
Authors     :   Jim Dosé

Copyright   :   Copyright 2014 Oculus VR, LLC. All Rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the Cinema/ directory. An additional grant 
of patent rights can be found in the PATENTS file in the same directory.

*************************************************************************************/

#include "MoviePosterComponent.h"

namespace OculusCinema {

bool MoviePosterComponent::ShowShadows = true;

//==============================
//  MoviePosterComponent::
MoviePosterComponent::MoviePosterComponent() :
	CarouselItemComponent( VRMenuEventFlags_t() ),
    Poster( NULL ),
	PosterImage( NULL ),
    Is3DIcon( NULL ),
    Shadow( NULL )
{
}

//==============================
//  MoviePosterComponent::OnEvent_Impl
eMsgStatus MoviePosterComponent::OnEvent_Impl( App * app, VrFrame const & vrFrame, OvrVRMenuMgr & menuMgr,
        VRMenuObject * self, VRMenuEvent const & event )
{
	return MSG_STATUS_ALIVE;
}

//==============================
//  MoviePosterComponent::SetMenuObjects
void MoviePosterComponent::SetMenuObjects( VRMenuObject * poster, VRMenuObject * posterImage, VRMenuObject * is3DIcon, VRMenuObject * shadow )
{
	Poster = poster;
	PosterImage = posterImage;
    Is3DIcon = is3DIcon;
    Shadow = shadow;
}

//==============================
//  MoviePosterComponent::SetItem
void MoviePosterComponent::SetItem( VRMenuObject * self, const CarouselItem * item, const PanelPose &pose )
{
	Poster->SetLocalPosition( pose.Position );
	Poster->SetLocalRotation( pose.Orientation );

	if ( item != NULL )
	{
		PosterImage->SetText( item->name.ToCStr() );
		PosterImage->SetSurfaceTexture( 0, 0, SURFACE_TEXTURE_DIFFUSE,
			item->texture, item->textureWidth, item->textureHeight );
		PosterImage->SetColor( pose.Color );
		PosterImage->SetTextColor( pose.Color );

		Is3DIcon->SetColor( pose.Color );
		Shadow->SetColor( pose.Color );

		if ( item->userFlags & 1 )
		{
			Is3DIcon->RemoveFlags( VRMENUOBJECT_DONT_RENDER );
		}
		else
		{
			Is3DIcon->AddFlags( VRMENUOBJECT_DONT_RENDER );
		}

		if ( ShowShadows )
		{
			Shadow->RemoveFlags( VRMENUOBJECT_DONT_RENDER );
		}
		else
		{
			Shadow->AddFlags( VRMENUOBJECT_DONT_RENDER );
		}

		PosterImage->RemoveFlags( VRMENUOBJECT_DONT_RENDER );
	}
	else
	{
		Is3DIcon->AddFlags( VRMENUOBJECT_DONT_RENDER );
		Shadow->AddFlags( VRMENUOBJECT_DONT_RENDER );
		PosterImage->AddFlags( VRMENUOBJECT_DONT_RENDER );
	}
}

} // namespace OculusCinema
