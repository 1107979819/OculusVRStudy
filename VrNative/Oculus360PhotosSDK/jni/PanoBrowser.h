/************************************************************************************

Filename    :   PanoBrowser.h
Content     :   Subclass for panoramic image files
Created     :   August 13, 2014
Authors     :   John Carmack, Warsam Osman

Copyright   :   Copyright 2014 Oculus VR, LLC. All Rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the Oculus360Photos/ directory. An additional grant
of patent rights can be found in the PATENTS file in the same directory.

************************************************************************************/

#ifndef OVR_PanoSwipeDir_h
#define OVR_PanoSwipeDir_h

#include "VRMenu/FolderBrowser.h"

namespace OVR
{
	class PanoBrowser : public OvrFolderBrowser
	{
	public:
		// only one of these every needs to be created
		static  PanoBrowser *		Create(
			App *					app,
			const Array<String>& 	searchPaths,
			OvrMetaData &			metaData,
			unsigned				thumbWidth,
			float					horizontalPadding,
			unsigned				thumbHeight,
			float					verticalPadding,
			unsigned 				numSwipePanels,
			float					swipeRadius );

		// Swiping when the menu is inactive can cycle through files in
		// the directory.  Step can be 1 or -1.
		virtual	OvrMetaDatum *		NextFileInDirectory( const int step );
		void						AddToFavorites( OvrMetaDatum * const panoData );
		void						RemoveFromFavorites( OvrMetaDatum * const panoData );

		// Called when a panel is activated
		virtual void				OnPanelActivated( OvrMetaDatum * panelData );

		// Called on opening menu - used to rebuild favorites using FavoritesBuffer
		virtual void				OnBrowserOpen();

		// Reload FavoritesBuffer with what's currently in favorites folder in FolderBrowser
		void						ReloadFavoritesBuffer();

		// Called on a background thread
		//
		// Create the thumbnail image for the file, which will
		// be saved out as a _thumb.jpg.
		virtual unsigned char *		CreateThumbnail( const char * filename, int & width, int & height );

		// Called on a background thread to load thumbnail
		virtual	unsigned char *		LoadThumbnail( const char * filename, int & width, int & height );

		// Adds thumbnail extension to a file to find/create its thumbnail
		virtual String				ThumbName( const String & s );

		// Called when no media found
		virtual void				OnMediaNotFound( App * app, String & title, String & imageFile, String & message );

		// Called if touch up is activated without a focused item
		virtual bool				OnTouchUpNoFocused();

		int							GetNumPanosInActive() const;

	private:
		PanoBrowser(
			App * app,
			const Array< String > & searchPaths,
			OvrMetaData & metaData,
			float panelWidth,
			float panelHeight,
			float radius,
			unsigned numSwipePanels,
			unsigned thumbWidth,
			unsigned thumbHeight )
			: OvrFolderBrowser( app, searchPaths, metaData, panelWidth, panelHeight, radius, numSwipePanels, thumbWidth, thumbHeight )
			, BufferDirty( false )
		{
		}

		virtual ~PanoBrowser()
		{
		}

		// Favorites buffer - this is built by PanoMenu and used to rebuild Favorite's menu
		// if an item in it is null, it's been deleted from favorites
		struct Favorite
		{
			Favorite()
			: Data( NULL )
			, IsFavorite( false )
			{}
			OvrMetaDatum *	Data;
			bool			IsFavorite;
		};
		Array< Favorite >	FavoritesBuffer;
		bool				BufferDirty;
	};

	unsigned char * LoadPageJpg( const char * cbzFileName, int pageNum, int & width, int & height );

}

#endif	// OVR_PanoSwipeDir_h
