/************************************************************************************

Filename    :   MoviePosterComponent.h
Content     :   Menu component for the movie selection menu.
Created     :   August 13, 2014
Authors     :   Jim Dosé

Copyright   :   Copyright 2014 Oculus VR, LLC. All Rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the Cinema/ directory. An additional grant 
of patent rights can be found in the PATENTS file in the same directory.

*************************************************************************************/

#include "CarouselBrowserComponent.h"

#if !defined( MoviePosterComponent_h )
#define MoviePosterComponent_h

using namespace OVR;

namespace OculusCinema {

//==============================================================
// MoviePosterComponent
class MoviePosterComponent : public CarouselItemComponent
{
public:
							MoviePosterComponent();

	static bool 			ShowShadows;

	void 					SetMenuObjects( VRMenuObject * poster, VRMenuObject * posterImage, VRMenuObject * is3DIcon, VRMenuObject * shadow );
	virtual void 			SetItem( VRMenuObject * self, const CarouselItem * item, const PanelPose &pose );

private:
    virtual eMsgStatus      OnEvent_Impl( App * app, VrFrame const & vrFrame, OvrVRMenuMgr & menuMgr,
                                    VRMenuObject * self, VRMenuEvent const & event );

    VRMenuObject * 			Poster;
	VRMenuObject * 			PosterImage;
    VRMenuObject * 			Is3DIcon;
    VRMenuObject * 			Shadow;
};

} // namespace OculusCinema

#endif // MoviePosterComponent_h
