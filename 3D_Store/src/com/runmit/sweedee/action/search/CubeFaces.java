package com.runmit.sweedee.action.search;

import android.os.Handler;

public class CubeFaces extends DefaultEffectFaces {

	public CubeFaces(Handler handler, DefaultEffectFacesParams params) {
		super(handler, params);
	}
	
	
	protected void updateV() {
		switch (mParams.move_type) {
		case Easying:
			
			break;
		case Spring:
			
			break;
		case Normal:
			easingMethod_constantSpeed();
			break;
		default:
			break;
		}
		
		genBuffers();
	}
	// 匀速运动
	private void easingMethod_constantSpeed(){
		mRotateY =30f;
		mRotateX = 10f ;
	}
	
	protected void genBuffers() {
		
		float width = 400, height = 400, deep = 400;
		
		float left = -width*.5f, right = width*.5f, bottom =-height*.5f, top = height*.5f, front = deep*.5f, back=-deep*.5f;
		
		verticesArr = new float[]{
			right, top, front,
			left, top, front,
			left, bottom, front,
			right,bottom, front,
			right, top, back,
			left, top, back,
			left, bottom, back,
			right,bottom, back
		};
		
		colorArr = new float[] {
			1, 0, 0, 1,
			0, 1, 0, 1,
			0, 0, 1, 1,
			1, 1, 0, 1,
			
			1, 0, 1, 1,
			0, 1, 1, 1,
			0, 0, 0, 1,
			1, 1, 1, 1,
		};
		
		vertxIndxArr = new short[] {
			// front
			0, 1, 2,
			0, 2, 3,
			// bottom
			3, 2, 6,
			3, 6, 7,
			//back
			7, 6, 5,
			7, 5, 4,
			//top
			4, 5, 1,
			4, 1, 0,
			//right
			4, 0, 3,
			4, 3, 7,
			//left
			1, 5, 6,
			1, 6, 2
		};
		
		normalizerArr = new float []{
			0, 0, 1,
			0, 0, 1,
			
			0, -1, 0,
			0, -1, 0,
			
			0, 0, -1,
			0, 0, -1,
			
			0, 1, 0,
			0, 1, 0,
			
			1, 0, 0,
			1, 0, 0,
			
			-1, 0, 0,
			-1, 0, 0
		};
		
		super.genBuffers();
	}
}
