/************************************************************************************

Filename    :   MainActivity.java
Content     :   
Created     :   
Authors     :   

Copyright   :   Copyright 2014 Oculus VR, LLC. All Rights reserved.

*************************************************************************************/
package com.oculusvr.vrscene;
import android.os.Bundle;
import android.util.Log;
import android.view.SurfaceView;

import com.oculusvr.vrlib.VrActivity;

public class MainActivity extends VrActivity {
	private SurfaceView sfl,sfr;
	public static final String TAG = "VrScene";
	
	/** Load jni .so on initialization */
	static {
		Log.d( TAG, "LoadLibrary" );
		System.loadLibrary( "vrscene" );
	}

	public static native long nativeSetAppInterface(VrActivity act);
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		Log.d( TAG, "onCreate" );
		super.onCreate(savedInstanceState);
		appPtr = nativeSetAppInterface( this );		
		sfl = (SurfaceView)findViewById(R.id.leftSurface);
		sfr = (SurfaceView)findViewById(R.id.rightSurface);
	
	}
	
}
