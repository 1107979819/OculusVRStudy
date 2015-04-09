/************************************************************************************

Filename    :   ScrubberActivity.java
Content     :   UI for playback controls and scrub bar.
Created     :   November 4, 2013
Authors     :   Jim Dose, based on a fork of ScrubberActivity.java from VrVideo by Dean Beeler.

Copyright   :   Copyright 2014 Oculus VR, LLC. All Rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the Cinema/ directory. An additional grant 
of patent rights can be found in the PATENTS file in the same directory.

*************************************************************************************/
package com.oculus.cinemasdk;

import com.oculus.cinemasdk.R;

import android.app.Activity;
import android.media.MediaPlayer;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.util.Log;
import android.view.View;
import android.widget.ImageButton;
import android.widget.ImageView;
import android.widget.SeekBar;
import android.widget.SeekBar.OnSeekBarChangeListener;
import android.widget.TextView;
import android.widget.RelativeLayout;
import android.graphics.Rect;
import android.graphics.drawable.Drawable;
import android.graphics.Typeface;

public class ScrubberActivity extends Activity {

	public static final String TAG = "scrubber";
	
	public interface ScrubberInterface {

		public abstract void onScrubberUpdate( ScrubberActivity anActivity );
		public abstract MediaPlayer getMediaPlayer();
		public abstract void setPosition( int positionMS );
		public abstract boolean isPlaying();
	}
	
	private final int			UI_NO_EVENT = 0;
	private final int			UI_RW_PRESSED = 1;
	private final int			UI_PLAY_PRESSED = 2;
	private final int			UI_FF_PRESSED = 3;
	private final int			UI_CAROUSEL_PRESSED = 4;
	private final int			UI_CLOSE_UI_PRESSED = 5;
	private final int			UI_USER_TIMEOUT = 6;
	private final int			UI_SEEK_PRESSED = 7;

	volatile private boolean 	playerShouldUpdate = false;
	
	private boolean 			seekBarTouchDown = false;
	
	private boolean				gotLayout = false;
	
	private SeekBar				seekBar;
	private RelativeLayout 		seekBarBackground;
	private int 				seekBarBackgroundColor;
	private Rect 				seekBarRect;
	
	private TextView			movieTitle;

	private RelativeLayout 		playbackControls;
	private boolean 			playbackControlsStartPress = false;
	private Rect 				playbackControlsRect;
	
	private ImageView			seekIcon;
	private int					currentSeekSpeed;
	
	private TextView			seekTimeCurrent;
	private TextView			seekTimeSelect;
	private final float 		seekTimeMinX = 364;
	private final float 		seekTimeMaxX = 586;
	
	private boolean				showSeekTimeSelect = false;
	
	private ImageButton 		rwButton;
	private int 				rwImage;
	private boolean 			rwButtonStartPress = false;
	private Rect 				rwButtonRect;

	private ImageButton 		pauseButton;
	private int 				pauseImage;
	private boolean 			pauseButtonStartPress = false;
	private Rect 				pauseButtonRect;

	private ImageButton 		ffButton;
	private int 				ffImage;
	private boolean 			ffButtonStartPress = false;
	private Rect 				ffButtonRect;

	private ImageButton 		movieSelectionButton;
	private int 				movieSelectionImage;
	private boolean 			movieSelectionButtonStartPress = false;
	private Rect 				movieSelectionButtonRect;
	
	private long				uiIdleStartTime;
	private final long			uiIdleTotalTime = 4000;
	
	private Typeface			font;
	
	private ScrubberInterface 	scrubberCallback;
	
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		font = Typeface.createFromAsset( getAssets(), "ClearSans-Bl.otf" );
		setContentView(R.layout.activity_scrubber);
	}
	
	private int getRelativeLeft(View myView) {
	    if (myView.getParent() == myView.getRootView())
	        return myView.getLeft();
	    else
	        return myView.getLeft() + getRelativeLeft((View) myView.getParent());
	}

	private int getRelativeRight(View myView) {
	    if (myView.getParent() == myView.getRootView())
	        return myView.getWidth();
	    else
	        return myView.getWidth() + getRelativeLeft( myView );
	}

	private int getRelativeTop(View myView) {
	    if (myView.getParent() == myView.getRootView())
	        return myView.getTop();
	    else
	        return myView.getTop() + getRelativeTop( (View) myView.getParent());
	}
	
	private int getRelativeBottom(View myView) {
	    if (myView.getParent() == myView.getRootView())
	        return myView.getBottom();
	    else
	        return myView.getBottom() + getRelativeTop((View) myView.getParent());
	}
	
	private Rect getRect( View myView )
	{
		Rect r = new Rect();
		
		r.left   = getRelativeLeft( myView );
		r.right  = getRelativeRight( myView );
		r.top  	 = getRelativeTop( myView );
		r.bottom = getRelativeBottom( myView );
		
		return r;
	}
	
	private String getTimeString( final int time )
	{
		int seconds = time / 1000;
		int minutes = seconds / 60;
		int hours = minutes / 60;
		seconds = seconds % 60;
		minutes = minutes % 60;
		
		String text;
		if ( hours > 0 )
		{
			text = String.format( "%d:%02d:%02d", hours, minutes, seconds );
		}
		else if ( minutes > 0 )
		{
			text = String.format( "%d:%02d", minutes, seconds );
		}
		else
		{
			text = String.format( "0:%02d", seconds );
		}
		
		return text;
	}


	public void setScrubberCallback( final ScrubberInterface scrubberCallback )
	{		
		Log.d(TAG, "setScrubberCallback " + this);
		seekBar = ( SeekBar )findViewById( R.id.movieProgressBar );
		seekBarBackground = ( RelativeLayout )findViewById( R.id.movieProgressBarBackground );
		seekBarBackgroundColor = R.color.seek_background_inactive;
		seekBarBackground.setBackgroundColor( getResources().getColor( seekBarBackgroundColor ) );
		
		this.scrubberCallback = scrubberCallback;
		
		final Drawable thumb = seekBar.getThumb(); 
		thumb.mutate().setAlpha(0); 

		int duration = 0;
		int position = 0;
		if ( scrubberCallback.getMediaPlayer() != null )
		{
			try
			{
				duration = scrubberCallback.getMediaPlayer().getDuration();
				position = scrubberCallback.getMediaPlayer().getCurrentPosition();
			}
			
			catch( IllegalStateException ise )
        	{
        		// Can be thrown by the media player if state changes while we're being
        		// queued for execution on the main thread
				Log.d( TAG, "setScrubberCallback(): Caught exception: " + ise.getMessage() );
        	}
		}

		seekBar.setMax( duration );
		seekBar.setProgress( position );
		
        seekBar.setOnSeekBarChangeListener(new OnSeekBarChangeListener() {
            public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
                if ( seekBarTouchDown )
                {
                	if ( scrubberCallback.getMediaPlayer() != null )
                	{
            			Log.d( TAG, "seek to " + progress + " fromUser: " + fromUser );
           				scrubberCallback.setPosition( progress );
                	}
                }
            }
 
            public void onStartTrackingTouch(SeekBar seekBar) {
            	Log.d(TAG, "startTrackingTouch" );
            	seekBarTouchDown = true;
            }
 
            public void onStopTrackingTouch(SeekBar seekBar) {
            	Log.d(TAG, "stopTrackingTouch" );
            	seekBarTouchDown = false;
            }
        });
        
    	playbackControls = ( RelativeLayout )findViewById( R.id.playbackControls );
    	
    	movieTitle = ( TextView )findViewById( R.id.movieTitle );
    	movieTitle.setTypeface( font );

    	seekIcon = ( ImageView )findViewById( R.id.seek_speed );
    	seekIcon.setVisibility( View.INVISIBLE );
    	currentSeekSpeed = 0;

    	seekTimeCurrent = ( TextView )findViewById( R.id.seekTimeCurrent );
    	seekTimeCurrent.setTypeface( font );
    	seekTimeSelect = ( TextView )findViewById( R.id.seekTimeSelect );
    	seekTimeSelect.setTypeface( font );
    	
		seekTimeCurrent.setVisibility( View.INVISIBLE );    	
    	seekTimeSelect.setVisibility( View.INVISIBLE );
    	showSeekTimeSelect = false;
    	
		movieSelectionButton = (ImageButton)findViewById( R.id.btnPlaylist );
		movieSelectionImage = R.drawable.btn_playlist;
		movieSelectionButton.setImageResource( movieSelectionImage );
		movieSelectionButtonStartPress = false;
		
		rwButton = (ImageButton)findViewById( R.id.btnRewind );
		rwImage = R.drawable.img_btn_rw;
		rwButton.setImageResource( rwImage );
		rwButtonStartPress = false;

		pauseButton = (ImageButton)findViewById( R.id.btnPause );
		pauseImage = R.drawable.btn_pause;
		pauseButton.setImageResource( pauseImage );
		pauseButtonStartPress = false;

		ffButton = (ImageButton)findViewById( R.id.btnFastForward );
		ffImage = R.drawable.img_btn_ff;
		ffButton.setImageResource( ffImage );
		ffButtonStartPress = false;

		final ScrubberActivity sa = this;
		
		// Fire off an update thread to move the scrubber every quarter second
		Thread t = new Thread( "Player update thread" ){
			public void run() 
			{
				while( playerShouldUpdate )
				{
					// Notify update of the view every second -- perform the callback on the main
					// thread so the UI can cooperate with us
					(new Handler (Looper.getMainLooper ())).post (new Runnable () 
			        {
			            @Override
			            public void run () 
			            {
			            	try
			            	{
					    		if ( scrubberCallback.getMediaPlayer() != null )
					    		{
					    			int duration = 0;
					    			int currentPosition = 0;

					    			try
				    				{
				    					duration = scrubberCallback.getMediaPlayer().getDuration();
				    					currentPosition = scrubberCallback.getMediaPlayer().getCurrentPosition();
				    				}
				    				
				    				catch( IllegalStateException ise )
				    	        	{
				    	        		// Can be thrown by the media player if state changes while we're being
				    	        		// queued for execution on the main thread
				    					Log.d( TAG, "setScrubberCallback.UpdateThread(): Caught exception: " + ise.getMessage() );
				    	        	}

									seekTimeCurrent.setText( getTimeString( currentPosition ) );
									seekBar.setMax( duration );
								
									if ( !seekBarTouchDown )
									{
										seekBar.setProgress( currentPosition );
									}

									float frac = ( duration == 0 ) ? 0.0f : ( ( float )currentPosition / ( float )duration );
									seekTimeCurrent.setX( seekTimeMinX + ( int )( ( seekTimeMaxX - seekTimeMinX ) * frac ) );
									seekTimeCurrent.setVisibility( View.VISIBLE );
					    		}
					    		else
					    		{
					    			seekTimeCurrent.setVisibility( View.INVISIBLE );
									seekTimeCurrent.setText( getTimeString( 0 ) );

					    			seekBar.setMax( 0 );
									seekBar.setProgress( 0 );
					    		}
				            	
				            	scrubberCallback.onScrubberUpdate( sa );
			            	}
			            	catch( IllegalStateException ise )
			            	{
			            		// Can be thrown by the media player if state changes while we're being
			            		// queued for execution on the main thread
			            	}
			            }
			        });
					
					try {
						Thread.sleep( 200 );
					} catch (InterruptedException e) {
					}
				}
			};
		};
		
		uiIdleStartTime = System.currentTimeMillis(); 

		playerShouldUpdate = true;
		t.start();
	}
	
	private void clearButtonPresses()
	{
		rwButtonStartPress = false;
		pauseButtonStartPress = false;
		ffButtonStartPress = false;
		movieSelectionButtonStartPress = false;
		playbackControlsStartPress = false;
	}
	
	public int gazeCursor( final float x, final float y, final boolean press, final boolean release, final int seekSpeed ) {
		int uiEvent = UI_NO_EVENT;
		boolean isPlaying = scrubberCallback.isPlaying();
		boolean updateUI = false;
		
		if ( !gotLayout )
		{
			rwButtonRect = getRect( rwButton );
			rwButtonRect.left += 10;
			rwButtonRect.right -= 10;
			pauseButtonRect = getRect( pauseButton );
			pauseButtonRect.left += 10;
			pauseButtonRect.right -= 10;
			ffButtonRect = getRect( ffButton );
			ffButtonRect.left += 10;
			ffButtonRect.right -= 10;
			movieSelectionButtonRect = getRect( movieSelectionButton );
			playbackControlsRect = getRect( playbackControls );			
			seekBarRect = getRect( seekBarBackground );

	    	if ( pauseButtonRect.bottom != 0.0f )
	    	{
	    		gotLayout = true;
	    	}
	    	else
	    	{
	    		return 0;
	    	}
		}
		
		//
		// seek bar
		//
		if ( seekBarTouchDown )
		{
			uiEvent = UI_SEEK_PRESSED;
		}
		
		//
		// Pause button
		//
		boolean insidePause = pauseButtonRect.contains( ( int )x, ( int )y );
		if ( insidePause )
		{
			if ( press )
			{
				clearButtonPresses();
				pauseButtonStartPress = true;
			}
			
			if ( release )
			{
				if ( pauseButtonStartPress )
				{
					Log.d(TAG, "pauseButton");
					uiEvent = UI_PLAY_PRESSED;
				}
				
				clearButtonPresses();
			}
		}
		else if ( release )
		{
			pauseButtonStartPress = false;
		}

		int nextPauseImage = 0;		
		if ( isPlaying )
		{
			if ( insidePause )
			{
				if ( pauseButtonStartPress )
				{
					nextPauseImage = R.drawable.btn_pause_pressed;
				}
				else
				{
					nextPauseImage = R.drawable.btn_pause_hover;
				}
			}
			else
			{
				nextPauseImage = R.drawable.btn_pause;
			}
		}
		else
		{
			if ( insidePause )
			{
				if ( pauseButtonStartPress )
				{
					nextPauseImage = R.drawable.btn_play_pressed;
				}
				else
				{
					nextPauseImage = R.drawable.btn_play_hover;
				}
			}
			else
			{
				nextPauseImage = R.drawable.btn_play;
			}
		}

		if ( ( nextPauseImage != pauseImage ) && !seekBarTouchDown )
		{
			pauseImage = nextPauseImage;
			pauseButton.setImageResource( pauseImage );
			updateUI = true;
		}

		//
		// rw button
		//
		boolean insideRW = rwButtonRect.contains( ( int )x, ( int )y );
		if ( insideRW )
		{
			if ( press )
			{
				clearButtonPresses();
				rwButtonStartPress = true;
			}
			
			if ( release )
			{
				if ( rwButtonStartPress )
				{
					Log.d(TAG, "rwButton");
					uiEvent = UI_RW_PRESSED;
				}
				
				clearButtonPresses();
			}
		}
		else if ( release )
		{
			rwButtonStartPress = false;
		}

		int nextRWImage = 0;		
		if ( insideRW )
		{
			if ( rwButtonStartPress )
			{
				nextRWImage = R.drawable.img_btn_rw_pressed;
			}
			else
			{
				nextRWImage = R.drawable.img_btn_rw_hover;
			}
		}
		else
		{
			nextRWImage = R.drawable.img_btn_rw;
		}

		if ( ( nextRWImage != rwImage ) && !seekBarTouchDown )
		{
			rwImage = nextRWImage;
			rwButton.setImageResource( rwImage );
			updateUI = true;
		}

		//
		// FF button
		//
		boolean insideFF = ffButtonRect.contains( ( int )x, ( int )y );
		if ( insideFF )
		{
			if ( press )
			{
				clearButtonPresses();
				ffButtonStartPress = true;
			}
			
			if ( release )
			{
				if ( ffButtonStartPress )
				{
					Log.d(TAG, "ffButton");
					uiEvent = UI_FF_PRESSED;
				}
				
				clearButtonPresses();
			}
		}
		else if ( release )
		{
			ffButtonStartPress = false;
		}

		int nextFFImage = 0;		
		if ( insideFF )
		{
			if ( ffButtonStartPress )
			{
				nextFFImage = R.drawable.img_btn_ff_pressed;
			}
			else
			{
				nextFFImage = R.drawable.img_btn_ff_hover;
			}
		}
		else
		{
			nextFFImage = R.drawable.img_btn_ff;
		}

		if ( ( nextFFImage != ffImage ) && !seekBarTouchDown )
		{
			ffImage = nextFFImage;
			ffButton.setImageResource( ffImage );
			updateUI = true;
		}

		//
		// Movie selection button
		//
		boolean insideMovieSelection = movieSelectionButtonRect.contains( ( int )x, ( int )y );
		if ( insideMovieSelection )
		{
			if ( press )
			{
				clearButtonPresses();
				movieSelectionButtonStartPress = true;
			}
			
			if ( release )
			{
				if ( movieSelectionButtonStartPress )
				{
					// open movie selection
					Log.d(TAG, "movieSelectionButton");
					uiEvent = UI_CAROUSEL_PRESSED;
				}
				
				clearButtonPresses();
			}
		}
		else if ( release )
		{
			movieSelectionButtonStartPress = false;
		}

		int nextMovieSelectionImage = R.drawable.btn_playlist;
		if ( insideMovieSelection )
		{
			if ( movieSelectionButtonStartPress )
			{
				nextMovieSelectionImage = R.drawable.btn_playlist_pressed;
			}
			else 
			{
				nextMovieSelectionImage = R.drawable.btn_playlist_hover; 
			}
		}
		
		if ( nextMovieSelectionImage != movieSelectionImage )
		{
			movieSelectionImage = nextMovieSelectionImage;
			movieSelectionButton.setImageResource( movieSelectionImage );
			updateUI = true;
		}
		
		//
		// Check for clicks outside of playback controls
		//
		boolean insidePlaybackControls = playbackControlsRect.contains( ( int )x, ( int )y );
		if ( !insidePlaybackControls )
		{
			if ( press )
			{
				clearButtonPresses();
				playbackControlsStartPress = true;
			}
			
			if ( release )
			{
				if ( playbackControlsStartPress )
				{
					// open movie selection
					Log.d(TAG, "outside playbackControls");
					uiEvent = UI_CLOSE_UI_PRESSED;
				}
				
				clearButtonPresses();
			}
		}
		else if ( release )
		{
			playbackControlsStartPress = false;
		}
		
		if ( currentSeekSpeed != seekSpeed )
		{
			currentSeekSpeed = seekSpeed;
			int seekImage = 0;
			switch( seekSpeed )
			{
			case -5:
				seekImage = R.drawable.img_seek_rw32x;
				break;

			case -4:
				seekImage = R.drawable.img_seek_rw16x;
				break;

			case -3:
				seekImage = R.drawable.img_seek_rw8x;
				break;

			case -2:
				seekImage = R.drawable.img_seek_rw4x;
				break;

			case -1:
				seekImage = R.drawable.img_seek_rw2x;
				break;

			case 0:
				seekImage = 0;
				break;

			case 1:
				seekImage = R.drawable.img_seek_ff2x;
				break;

			case 2:
				seekImage = R.drawable.img_seek_ff4x;
				break;

			case 3:
				seekImage = R.drawable.img_seek_ff8x;
				break;

			case 4:
				seekImage = R.drawable.img_seek_ff16x;
				break;

			case 5:
				seekImage = R.drawable.img_seek_ff32x;
				break;
			}
			
			if ( seekImage == 0 )
			{
				seekIcon.setVisibility( View.INVISIBLE );
				seekIcon.setImageResource( R.drawable.img_seek_ff2x );
			}
			else
			{
				seekIcon.setVisibility( View.VISIBLE );
				seekIcon.setImageResource( seekImage );
			}
			updateUI = true;
		}

		boolean insideSeekBar = seekBarRect.contains( ( int )x, ( int )y );
		int color = ( insideSeekBar || ( seekSpeed != 0 ) ) ? R.color.seek_background_active : R.color.seek_background_inactive;
		if ( color != seekBarBackgroundColor )
		{			
			seekBarBackgroundColor = color;
			seekBarBackground.setBackgroundColor( getResources().getColor( seekBarBackgroundColor ) );
			updateUI = true;
		}
		
		if ( insideSeekBar )
		{
			showSeekTimeSelect = true;
			seekTimeSelect.setVisibility( View.VISIBLE );
	    	
			int duration = 0;
			try
			{
				duration = scrubberCallback.getMediaPlayer().getDuration();
			}
			
			catch( IllegalStateException ise )
        	{
        		// Can be thrown by the media player if state changes while we're being
        		// queued for execution on the main thread
				Log.d( TAG, "ScrubberActivity::gazeCursor(): Caught IllegalStateException" );
        	}			
			
	    	float frac = ( float )( x - seekBarRect.left ) / ( float )seekBarRect.width();
	    	seekTimeSelect.setX( seekTimeMinX + ( seekTimeMaxX - seekTimeMinX ) * frac );
	    	seekTimeSelect.setText( getTimeString( ( int )( ( float )duration * frac ) ) );
			
			updateUI = true;
		}
		else if ( showSeekTimeSelect ) 
		{
			seekTimeSelect.setVisibility( View.INVISIBLE );
			showSeekTimeSelect = false;
			updateUI = true;
		}		
		
		if ( updateUI )
		{
			scrubberCallback.onScrubberUpdate( this );
		}
		
		// check idle
		long now = System.currentTimeMillis();
		if ( insidePlaybackControls || ( seekSpeed != 0 ) || !isPlaying )
		{
			// update our idle time
			uiIdleStartTime = now;
		}
		else if ( uiEvent == UI_NO_EVENT )
		{
			long idleTime = now - uiIdleStartTime;
			if ( idleTime > uiIdleTotalTime )
			{				
				uiEvent = UI_USER_TIMEOUT;
			}
		}

		return uiEvent;
	}

	public void setVideoName( String aName )
	{
		movieTitle.setText( aName );
	}
	
	public void onDestroy()
	{
		Log.d(TAG, "scrubber.onDestroy" );
		super.onDestroy();
		playerShouldUpdate = false;
	}

	public int[] getPlaybackControlsRect() {
		int[] arr = new int[ 4 ];

		if ( !gotLayout )
		{
			arr[ 0 ] = -1;
			arr[ 1 ] = -1;
			arr[ 2 ] = -1;
			arr[ 3 ] = -1;
		}
		else
		{
			arr[ 0 ] = playbackControlsRect.left;
			arr[ 1 ] = playbackControlsRect.bottom;
			arr[ 2 ] = playbackControlsRect.right;
			arr[ 3 ] = playbackControlsRect.top;
		}

		return arr;
	}
}
