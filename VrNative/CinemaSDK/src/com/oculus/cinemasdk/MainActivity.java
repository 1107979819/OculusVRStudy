/************************************************************************************

Filename    :   MainActivity.java
Content     :   Media player controller.
Created     :   September 3, 2013
Authors     :	Jim Dose, based on a fork of MainActivity.java from VrVideo by John Carmack.   

Copyright   :   Copyright 2014 Oculus VR, LLC. All Rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the Cinema/ directory. An additional grant 
of patent rights can be found in the PATENTS file in the same directory.

*************************************************************************************/
package com.oculus.cinemasdk;

import java.io.File;
import java.io.IOException;
import java.io.FileOutputStream;
import java.lang.IllegalStateException;
import java.lang.IllegalArgumentException;
import java.util.ArrayList;
import java.util.List;

import android.app.Activity;
import android.content.Intent;
import android.content.SharedPreferences.Editor;
import android.graphics.Bitmap;
import android.graphics.SurfaceTexture;
import android.graphics.Matrix;
import android.media.MediaMetadataRetriever;
import android.media.MediaPlayer;
import android.media.MediaPlayer.OnCompletionListener;
import android.media.MediaPlayer.OnSeekCompleteListener;
import android.media.ThumbnailUtils;
import android.net.Uri;
import android.os.Bundle;
import android.provider.MediaStore;
import android.util.Log;import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.View;
import android.content.Context;
import android.media.AudioManager;
import android.media.AudioManager.OnAudioFocusChangeListener;
import com.oculusvr.vrlib.VrActivity;

public class MainActivity extends VrActivity implements SurfaceHolder.Callback,
		MediaPlayer.OnVideoSizeChangedListener, 
		ScrubberActivity.ScrubberInterface,
		AudioManager.OnAudioFocusChangeListener		
{
	public static final String TAG = "Cinema";

	/** Load jni .so on initialization */
	static 
	{
		Log.d(TAG, "LoadLibrary");
		System.loadLibrary("cinema");
	}
	
	public static final int MinimumRemainingResumeTime = 60000;	// 1 minute
	public static final int MinimumSeekTimeForResume = 60000;	// 1 minute

	public static final String SCRUBBER_ID = "Scrubber";

	public static native void nativeSetParms( long appPtr, float screenDist );
	public static native void nativeSetVideoSize( long appPtr, int width, int height, int rotation );
	public static native SurfaceTexture nativePrepareNewVideo( long appPtr );
	public static native float nativeGetScreenDist( long appPtr );

	String 				currentMovieFilename;
	String 				currentMovieDisplayName;
	
	// The movie screen will extend from -1 to 1 horizontally, with
	// the vertical height adjusted for aspect ratio.
	// These are not relevant now, but saved as examples of preferences
	float 				screenDist = 1.6f;
	
	boolean				playbackFinished = true;
	boolean				playbackFailed = false;
	
	private boolean 	waitingForSeek = false;
	private boolean 	haveSeekWaiting = false;
	private int 		nextSeekPosition = 0;
	
	protected View 		projectedView;

	SurfaceTexture 		uiTexture = null;
	Surface 			uiSurface = null;
	
	SurfaceTexture 		movieTexture = null;
	Surface 			movieSurface = null;

	MediaPlayer 		mediaPlayer = null;
	AudioManager 		audioManager = null;

	public static native long nativeSetAppInterface(VrActivity act);
	@Override
	protected void onCreate(Bundle savedInstanceState) 
	{
		Log.d(TAG, "onCreate");
		super.onCreate(savedInstanceState);
		appPtr = nativeSetAppInterface( this );

		// Restore preferences
		screenDist = getPreferences(MODE_PRIVATE).getFloat("screenDist", 1.2f);
		passCurrentParmsToNative( false );
		
		audioManager = ( AudioManager )getSystemService( Context.AUDIO_SERVICE );
	}

	@Override
	protected void onDestroy() 
	{
		// Abandon audio focus if we still hold it
		releaseAudioFocus();
		super.onDestroy();
    }

	@Override
	protected void onPause() 
	{
		Log.d(TAG, "onPause()");
		
		pauseMovie();

		// fetch any updated screenDist from native
		// and save to preferences
		passCurrentParmsToNative( true );

		super.onPause();
	}

	protected void onResume() 
	{
		Log.d(TAG, "onResume()");
		super.onResume();
	}
	
    public void onAudioFocusChange(int focusChange) 
    {
		switch( focusChange ) 
		{
		case AudioManager.AUDIOFOCUS_GAIN:
			// resume() if coming back from transient loss, raise stream volume if duck applied
			Log.d(TAG, "onAudioFocusChangedListener: AUDIOFOCUS_GAIN");
			break;
		case AudioManager.AUDIOFOCUS_LOSS:				// focus lost permanently
			// stop() if isPlaying
			Log.d(TAG, "onAudioFocusChangedListener: AUDIOFOCUS_LOSS");		
			break;
		case AudioManager.AUDIOFOCUS_LOSS_TRANSIENT:	// focus lost temporarily
			// pause() if isPlaying
			Log.d(TAG, "onAudioFocusChangedListener: AUDIOFOCUS_LOSS_TRANSIENT");	
			break;
		case AudioManager.AUDIOFOCUS_LOSS_TRANSIENT_CAN_DUCK:	// focus lost temporarily
			// lower stream volume
			Log.d(TAG, "onAudioFocusChangedListener: AUDIOFOCUS_LOSS_TRANSIENT_CAN_DUCK");		
			break;
		default:
			break;
		}
	}
    
	/**
	 * @param fetchScreenDist
	 *            get latest screenDist from native, which may have moved it
	 *            interactively
	 */
	void passCurrentParmsToNative(boolean fetchScreenDist) 
	{
		if (fetchScreenDist) 
		{
			screenDist = nativeGetScreenDist( appPtr );
		}
		nativeSetParms( appPtr, screenDist );

		Editor edit = getPreferences(MODE_PRIVATE).edit();
		edit.putFloat("screenDist", screenDist);
		edit.commit();
	}
	
	public int getPosition()
	{
		if (mediaPlayer != null) 
		{
			try
			{
				return mediaPlayer.getCurrentPosition();
			}
			catch( IllegalStateException ise )
        	{
        		// Can be thrown by the media player if state changes while we're being
        		// queued for execution on the main thread
				Log.d( TAG, "getPosition(): Caught illegalStateException" );
				return 0;
        	}
		}
		return 0;	
	}

	@Override
	public void setPosition( int positionMilliseconds )
	{
		try
		{
			if (mediaPlayer != null) 
			{
				boolean wasPlaying = isPlaying();
				if (wasPlaying) 
				{
					mediaPlayer.pause();
				}
				int duration = mediaPlayer.getDuration();
				int newPosition = positionMilliseconds;
				if (newPosition >= duration) 
				{
					Log.d(TAG, "seek past end");
					// pause if seeking past end
					mediaPlayer.seekTo(duration);
					return;
				}
				if (newPosition < 0) 
				{
					newPosition = 0;
				}
				
				if ( waitingForSeek )
				{
					haveSeekWaiting = true;
					nextSeekPosition = newPosition;
				}
				else
				{
					waitingForSeek = true;
					Log.d(TAG, "seek started");
					mediaPlayer.seekTo( newPosition );
				}
	
				if ( wasPlaying ) 
				{
					mediaPlayer.start();
				}
			}
		}

		catch( IllegalStateException ise )
		{
			// Can be thrown by the media player if state changes while we're being
			// queued for execution on the main thread
			Log.d( TAG, "setPosition(): Caught illegalStateException" );
		}
	}
	
	public void seekDelta(int deltaMilliseconds) 
	{
		try
		{
			if (mediaPlayer != null) 
			{
				boolean wasPlaying = isPlaying();
				if ( wasPlaying ) 
				{
					mediaPlayer.pause();
				}
				
				int position = mediaPlayer.getCurrentPosition();
				int duration = mediaPlayer.getDuration();
				int newPosition = position + deltaMilliseconds;
				if (newPosition >= duration) 
				{
					Log.d(TAG, "seek past end");
					// pause if seeking past end
					mediaPlayer.seekTo(duration);
					return;
				}
				if (newPosition < 0) 
				{
					newPosition = 0;
				}
				
				if ( waitingForSeek )
				{
					haveSeekWaiting = true;
					nextSeekPosition = newPosition;
				}
				else
				{
					waitingForSeek = true;
					Log.d(TAG, "seek started");
					mediaPlayer.seekTo( newPosition );
				}
	
				if ( wasPlaying ) 
				{
					mediaPlayer.start();
				}
			}
		}
		
		catch( IllegalStateException ise )
    	{
    		// Can be thrown by the media player if state changes while we're being
    		// queued for execution on the main thread
			Log.d( TAG, "seekDelta(): Caught illegalStateException" );
    	}
	}

	// called from native code
	public boolean isPlaybackFinished()
	{
		return playbackFinished;
	}
	
	// called from native code
	public boolean hadPlaybackError()
	{
		return playbackFailed;
	}
	
	// called from native code
	public void showUI()
	{
    	runOnUiThread( new Thread()
    	{
		 @Override
    		public void run()
    		{
			 	Log.v(TAG, "showUI()");
			 	if (getLocalActivityManager().getActivity(SCRUBBER_ID) == null)
			 	{
			 		showScrubber();
			 	}
    		}
    	}	);
	}
	
	public void hideUI()
	{
    	runOnUiThread( new Thread()
    	{
		 @Override
    		public void run()
    		{
			 	Log.v(TAG, "hideUI()");

			 	if (getLocalActivityManager().getActivity(SCRUBBER_ID) != null)
			 	{
					hideScrubber();
				} 
    		}
    	}	);
	}
	
	int getRotationFromMetadata( final String filePath ) 
	{
		MediaMetadataRetriever metaRetriever = new MediaMetadataRetriever();
		metaRetriever.setDataSource(filePath);
		String value = metaRetriever.extractMetadata(MediaMetadataRetriever.METADATA_KEY_VIDEO_ROTATION);
		metaRetriever.release();

		if (value == null) 
		{
			return 0;
		}

		if (value.equals("0")) 
		{
			return 0;
		} 
		else if (value.equals("90")) 
		{
			return 90;
		} 
		else if (value.equals("180")) 
		{
			return 180;
		} 
		else if (value.equals("270")) 
		{
			return 270;
		} 

		return 0;
	}
	
	public boolean createVideoThumbnail( final String videoFilePath, final String outputFilePath, final int width, final int height )
	{
		Log.e( TAG, "Create video thumbnail: " + videoFilePath + "\noutput path: " + outputFilePath );
		Bitmap bmp = ThumbnailUtils.createVideoThumbnail( videoFilePath,  MediaStore.Images.Thumbnails.MINI_KIND );
		if ( bmp == null )
		{
			return false;
		}

		float desiredAspectRatio = ( float )width / ( float )height;
		float aspectRatio = ( float )bmp.getWidth() / ( float )bmp.getHeight();
		
		int cropWidth = bmp.getWidth();
		int cropHeight = bmp.getHeight();
		boolean shouldCrop = false;
		
		if ( aspectRatio < desiredAspectRatio )
		{
			cropWidth = bmp.getWidth();
			cropHeight = ( int )( ( float )cropWidth / desiredAspectRatio );
			shouldCrop = true;
		}
		else if ( aspectRatio > desiredAspectRatio )
		{
			cropHeight = bmp.getHeight();
			cropWidth = ( int )( ( float )cropHeight * desiredAspectRatio );
			shouldCrop = true;
		}
		
		if ( shouldCrop )
		{
			int cropX = ( bmp.getWidth() - cropWidth ) / 2;
			int cropY = ( bmp.getHeight() - cropHeight ) / 2;
			
			try {
				Bitmap croppedBmp = Bitmap.createBitmap( bmp, cropX, cropY, cropWidth, cropHeight, new Matrix(), false );
				if ( croppedBmp == null )
				{
					return false;
				}
				
				bmp = croppedBmp;
			}
			
			catch ( Exception e ) 
			{
				Log.e( TAG, "Cropping video thumbnail failed: " + e.getMessage() );
				return false;
			}
		}
		
		boolean failed = false;
		FileOutputStream out = null;
		try {
		    out = new FileOutputStream( outputFilePath );
		    bmp.compress( Bitmap.CompressFormat.PNG, 100, out );
		}
		
		catch ( Exception e ) 
		{
			failed = true;
			Log.e( TAG, "Writing video thumbnail failed: " + e.getMessage() );
		}
		
		finally 
		{
		    try 
		    {
		        if (out != null) 
		        {
		            out.close();
		        }
		    } 
		    
		    catch (IOException e) 
		    {
				failed = true;
				Log.e( TAG, "Closing video thumbnail failed: " + e.getMessage() );
		    }
		}	
		
		if ( !failed )
		{
			Log.e( TAG, "Wrote " + outputFilePath );
		}
		
		return !failed;
	}

	// ---------------------------------------------------------------------------
	// 2D sub-activity life-cycle replication
	
	// Since VrActivity subclasses ActivityGroup, it is free to fire off
	// additional activities which will then
	// be managed by the local activity manager by ID
	@SuppressWarnings("deprecation")
	private void showScrubber() 
	{
		Log.v(TAG, "showScrubber()");
		
		projectedView = getLocalActivityManager().startActivity(
				SCRUBBER_ID,
				new Intent( this, ScrubberActivity.class )
						.addFlags( Intent.FLAG_ACTIVITY_CLEAR_TOP ) )
				.getDecorView();

		if( projectedView != null ) 
		{
			ScrubberActivity scrubber = (ScrubberActivity) getLocalActivityManager().getActivity( SCRUBBER_ID );

			scrubber.setScrubberCallback( this );
			
			Log.v(TAG, "scrubber.setVideoName");
			scrubber.setVideoName( currentMovieDisplayName );
			
			projectedView.measure(0, 0);

			int layoutWidth = projectedView.getMeasuredWidth();
			int layoutHeight = projectedView.getMeasuredHeight();			
			
			projectedView.layout(0, 0, layoutWidth, layoutHeight );
			Log.d(TAG, "layout: " + layoutWidth + ", " + layoutHeight );

			createActivityDialog( projectedView, (Activity)scrubber );
		}
	}

	@SuppressWarnings("deprecation")
	private void hideScrubber() 
	{
		Log.v(TAG, "hideScrubber()");
	 	if (getLocalActivityManager().getActivity(SCRUBBER_ID) != null) 
	 	{
	 		getLocalActivityManager().destroyActivity(SCRUBBER_ID, true);
	 	}
		hideActivityDialog();
	}

	@Override
	public void onScrubberUpdate(ScrubberActivity anActivity) 
	{
		refreshActivityDialog( projectedView );
	}

	@Override
	public MediaPlayer getMediaPlayer() 
	{
		synchronized (this) 
		{
			return mediaPlayer;
		}
	}

	// ==================================================================================

	public void onVideoSizeChanged(MediaPlayer mp, int width, int height) 
	{
		Log.v(TAG, String.format("onVideoSizeChanged: %dx%d", width, height));
		int rotation = getRotationFromMetadata( currentMovieFilename );
		nativeSetVideoSize( appPtr, width, height, rotation );
	}

	public void pauseMovie() 
	{
		Log.d(TAG, "pauseMovie()" );
		if (mediaPlayer != null) 
		{
			if ( isPlaying() )
			{
				saveCurrentMovieLocation();
			}
			
			try
			{
				mediaPlayer.pause();
			}
			
			catch ( IllegalStateException t )
			{
				Log.e(TAG, "pauseMovie() caught illegalStateException" );
			}			
		}
	}

	public void resumeMovie() 
	{
		Log.d(TAG, "resumeMovie()" );
		if (mediaPlayer != null) 
		{
			try
			{
				mediaPlayer.start();
			}

			catch ( IllegalStateException t )
			{
				Log.e(TAG, "resumeMovie() caught illegalStateException" );
			}
		}
	}

	String fileNameFromPathName(String pathname) 
	{
		File f = new File(pathname);
		return f.getName();
	}
	
	public void togglePlaying()
	{
		Log.d(TAG,  "MainActivity.togglePlaying()");
		if (mediaPlayer != null) 
		{
			if ( isPlaying() ) 
			{
				pauseMovie();
			}
			else 
			{
				resumeMovie();
			}
		}
		else
		{
			Log.d(TAG, "mediaPlayer == null" );
		}
	}

	@Override
	public boolean isPlaying()
	{
		if (mediaPlayer !=null) 
		{
			try
			{
				return mediaPlayer.isPlaying();
			}
			
			catch ( IllegalStateException t )
			{
				Log.e(TAG, "isPlaying() caught illegalStateException" );
				return false;
			}
		}
		return false;
	}

	public void stopMovie()
	{
		Log.v(TAG, "stopMovie" );
		
		synchronized (this) 
		{
			if (mediaPlayer !=null) 
			{
				// don't save location if not playing
				if ( isPlaying() )
				{
					saveCurrentMovieLocation();
				}
				
				try
				{
					mediaPlayer.stop();
				}
				
				catch ( IllegalStateException t )
				{
					Log.e(TAG, "mediaPlayer.stop threw illegalStateException" );
				}
	
				mediaPlayer.release();
				mediaPlayer = null;
			}
			releaseAudioFocus();
			
			playbackFailed = false;
			playbackFinished = true;
		}
	}
	
	public boolean checkForMovieResume( final String pathName ) 
	{
		Log.d(TAG, "checkForMovieResume: " + pathName );

		try
		{
			final int seekPos = getPreferences(MODE_PRIVATE).getInt( pathName + "_pos", 0);
			final int duration = getPreferences(MODE_PRIVATE).getInt( pathName + "_len", -1);
		
			Log.d(TAG, "Saved Location: " + seekPos );
			Log.d(TAG, "Saved duration: " + duration );
			
			if ( seekPos < MinimumSeekTimeForResume )
			{
				Log.d(TAG, "below minimum.  Restart movie." );
				return false;
			}
			
			// early versions didn't save a duration, so if we don't have one, it's ok to resume
			if ( duration == -1 )
			{
				Log.d(TAG, "No duration.  Resume movie." );
				return true;
			}
			
			if ( seekPos > ( duration - MinimumRemainingResumeTime ) )
			{
				Log.d(TAG, "Past maximum.  Restart movie." );
				return false;
			}
			
			Log.d(TAG, "Resume movie." );		
			return true;
		}
		
		catch ( IllegalStateException t )
		{
			Log.e(TAG, "checkForMovieResume caught exception: " + t.getMessage());
			return false;
		}
	}
	
	// called from native code for starting movie
	public void startMovieFromNative( final String pathName, final String displayName, final boolean resumePlayback, final boolean isEncrypted ) 
	{
		// set playbackFinished and playbackFailed to false immediately so it's set when we return to native
		playbackFinished = false;
		playbackFailed = false;
		
    	runOnUiThread( new Thread()
    	{
		 @Override
    		public void run()
    		{
			 	startMovie( pathName, displayName, resumePlayback, isEncrypted );
    		}
    	} 	);
	}
	
	public void Fail( final String message )
	{
		Log.e(TAG, message );
		mediaPlayer.release();
		mediaPlayer = null;
		playbackFinished = true;
		playbackFailed = true;
		releaseAudioFocus();		
	}

	@SuppressWarnings("deprecation")
	public void startMovie( final String pathName, final String displayName, final boolean resumePlayback, boolean isEncrypted ) 
	{
		Log.v(TAG, "startMovie " + pathName + " resume " + resumePlayback );
		
		synchronized (this) 
		{
			requestAudioFocus();
	
			playbackFinished = false;
			playbackFailed = false;
			
			waitingForSeek = false;
			haveSeekWaiting = false;
			nextSeekPosition = 0;		
	
			currentMovieFilename = pathName;
			currentMovieDisplayName = displayName;
			
			// Have native code pause any playing movie,
			// allocate a new external texture,
			// and create a surfaceTexture with it.
			movieTexture = nativePrepareNewVideo( appPtr );
			movieSurface = new Surface(movieTexture);
	
			if (mediaPlayer != null) 
			{
				mediaPlayer.release();
			}
	
			Log.v(TAG, "MediaPlayer.create");
	
			mediaPlayer = new MediaPlayer();
			mediaPlayer.setOnVideoSizeChangedListener(this);
			mediaPlayer.setSurface(movieSurface);
	
			try 
			{
				Log.v(TAG, "mediaPlayer.setDataSource()");
				mediaPlayer.setDataSource(currentMovieFilename);
			} 
			
			catch (IOException t) 
			{
				Fail( "mediaPlayer.setDataSource threw IOException" );
				return;
			}
			
			catch ( IllegalStateException t )
			{
				Fail( "mediaPlayer.setDataSource threw illegalStateException" );
				return;
			}
	
			catch ( IllegalArgumentException t ) 
			{
				Fail( "mediaPlayer.setDataSource threw IllegalArgumentException" );
				return;
			}
	
			try 
			{
				Log.v(TAG, "mediaPlayer.prepare");
				mediaPlayer.prepare();
			} 
			
			catch (IOException t) 
			{
				Fail( "mediaPlayer.prepare threw IOException" );
				return;
			}
			
			catch ( IllegalStateException t )
			{
				Fail( "mediaPlayer.prepare threw illegalStateException" );
				return;
			}
		
			// ensure we're at max volume
			mediaPlayer.setVolume( 1.0f, 1.0f );
			
			Log.v(TAG, "mediaPlayer.start");
	
			// If this movie has a saved position, seek there before starting
			Log.d(TAG, "checkForMovieResume: " + currentMovieFilename );
			final int seekPos = getPreferences( MODE_PRIVATE ).getInt( currentMovieFilename + "_pos", 0);
			Log.v(TAG, "seekPos = " + seekPos );
			Log.v(TAG, "resumePlayback = " + resumePlayback );
			
			try
			{
				if ( resumePlayback && ( seekPos > 0) ) 
				{
					Log.v(TAG, "resuming at saved location");
					mediaPlayer.seekTo(seekPos);
				}
				else
				{
					// start at beginning
					Log.v(TAG, "start at beginning");
					mediaPlayer.seekTo(0);
				}
			}
			
			catch ( IllegalStateException t )
			{
				Fail( "mediaPlayer.seekTo threw illegalStateException" );
				return;
			}
	
			mediaPlayer.setLooping(false);
			mediaPlayer.setOnCompletionListener ( new OnCompletionListener() {
	        	public void onCompletion( MediaPlayer mp )
	        	{
	        		Log.v(TAG, "mediaPlayer.onCompletion");
	        		playbackFinished = true;
	        		saveCurrentMovieLocation();
	        		releaseAudioFocus();
	        	}        			
	        });
			
			mediaPlayer.setOnSeekCompleteListener( new OnSeekCompleteListener() {
	        	public void onSeekComplete( MediaPlayer mp )
	        	{
	        		if ( haveSeekWaiting )
	        		{
	        			mediaPlayer.seekTo( nextSeekPosition );
	        			haveSeekWaiting = false;
	        		}
	        		else
	        		{
	        			waitingForSeek = false;
	        		}
	        	}        			
	        });
			
			try
			{
				mediaPlayer.start();
			}
	
			catch ( IllegalStateException t )
			{
				Fail( "mediaPlayer.start threw illegalStateException" );
			}
	
			// Save the current movie now that it was successfully started
			Editor edit = getPreferences(MODE_PRIVATE).edit();
			edit.putString("currentMovie", currentMovieFilename);
			edit.commit();
	
			ScrubberActivity scrubber = (ScrubberActivity) getLocalActivityManager()
					.getActivity(SCRUBBER_ID);
		 	
			if ( scrubber != null )
			{
				Log.v(TAG, "scrubber.setVideoName");
				scrubber.setVideoName( currentMovieDisplayName );
			}
		}
		
		Log.v(TAG, "returning");
	}

	void requestAudioFocus()
	{
		// Request audio focus
		int result = audioManager.requestAudioFocus( this, AudioManager.STREAM_MUSIC,
			AudioManager.AUDIOFOCUS_GAIN );
		if ( result == AudioManager.AUDIOFOCUS_REQUEST_GRANTED ) 
		{
			Log.d(TAG,"startMovie(): GRANTED audio focus");
		} 
		else if ( result == AudioManager.AUDIOFOCUS_REQUEST_GRANTED ) 
		{
			Log.d(TAG,"startMovie(): FAILED to gain audio focus");
		}				
	}
	
	void releaseAudioFocus()
	{
		audioManager.abandonAudioFocus( this );
	}

	/*
	 * Whenever we pause or switch to another movie, save the current movie
	 * position so we will return there when the same file is viewed again.
	 */
	void saveCurrentMovieLocation() 
	{
		Log.d(TAG, "saveCurrentMovieLocation()" );
		if (mediaPlayer == null) 
		{
			return;
		}
		if (currentMovieFilename.length() < 1) 
		{
			return;
		}

		int duration = 0;
		int currentPos = 0;
		
		try
		{
			duration = mediaPlayer.getDuration();
			currentPos = mediaPlayer.getCurrentPosition();
		}
		
		catch( IllegalStateException ise )
    	{
    		// Can be thrown by the media player if state changes while we're being
    		// queued for execution on the main thread
			Log.d( TAG, "saveCurrentMovieLocation(): Caught IllegalStateException" );
    	}

		if ( playbackFinished )
		{
			currentPos = 0;
		}
		
		// Save the current movie now that it was successfully started
		Editor edit = getPreferences(MODE_PRIVATE).edit();
		Log.d(TAG, "set resume point: " + currentMovieFilename );
		Log.d(TAG, "pos: " + currentPos );
		Log.d(TAG, "len: " + duration );
		edit.putInt(currentMovieFilename + "_pos",	currentPos);
		edit.putInt(currentMovieFilename + "_len",	duration);
		edit.commit();
	}
	
	public int gazeCursor( final float x, final float y, final boolean press, final boolean release, final int seekSpeed ) {
		try
		{
			ScrubberActivity scrubber = (ScrubberActivity) getLocalActivityManager().getActivity( SCRUBBER_ID );
			if ( scrubber != null )
			{
				return scrubber.gazeCursor( x, y, press, release, seekSpeed );
			}
		}
		
		catch ( Exception t )
		{
			Log.e(TAG, "MainActivity::gazeCursor caught exception: "  + t.getMessage());
			return 0;	
		}
		
		return 0;
	}
	
	public int[] getPlaybackControlsRect() {
		try
		{
			ScrubberActivity scrubber = (ScrubberActivity) getLocalActivityManager().getActivity( SCRUBBER_ID );
			if ( scrubber != null )
			{
				return scrubber.getPlaybackControlsRect();
			}
		}
		
		catch ( Exception t )
		{
			Log.e(TAG, "MainActivity::getPlaybackControlsRect caught exception: "  + t.getMessage());
		}
		
		int[] arr = new int[ 4 ];
		arr[ 0 ] = -1;
		arr[ 1 ] = -1;
		arr[ 2 ] = -1;
		arr[ 3 ] = -1;
		return arr;
	}
}
