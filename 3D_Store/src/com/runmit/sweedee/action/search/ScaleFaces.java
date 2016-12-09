package com.runmit.sweedee.action.search;

import android.os.Handler;

public class ScaleFaces extends DefaultEffectFaces {

	float left = 100, top = 250, right=200, bottom = 100, deep = 0;

	private static boolean e80 = false;
	private float spring = 0.50f;	// 衰减
	private float friction = 0.590f; // 弹力
	
	public ScaleFaces(Handler handler, DefaultEffectFacesParams params) {
		super(handler, params);
		if (params.scaleRange != null){
            left = params.scaleRange[0];
            top = params.scaleRange[1];
            right= params.scaleRange[2];
            bottom = params.scaleRange[3];
        }else {
            left = 0;
            top = 250;
            right=100;
            bottom = 10;
        }

		deep = 0;
		e80 = false;
	}
	
	protected void updateV() {
		
		switch (mParams.move_type) {
		case Easying:
			easingMethod_ease();
			break;
		case Spring:
			easingMethod_spring();
			break;
		case Normal:
		default:
			break;
		}
		
		// 效果展示完毕
		genBuffers();
	}
	
	
	private float vxl = 0;
	private float vxr = 0;
	private float vxb = 0;
	private float vxt = 0;
	// 弹性运动
	private void easingMethod_spring() {
		float target = 0;
		float dx;
		float dxl = target - left;
		float ax = dxl * spring;
		boolean isPreFin = false;
		boolean isFin = false;
		vxl += ax;
		vxl *= friction;
		left += vxl;
		dx = dxl;

		float dxb = target - bottom;
		ax = dxb * spring;
		vxb += ax;
		vxb *= friction;
		bottom += vxb;
		if(dxb > dx){
			isPreFin = Math.abs(ax) < 2;
			isFin = Math.abs(ax) < 0.02;			
		}

		target = screenH;
		float dxt = target - top;
		ax = dxt * spring;
		vxt += ax;
		vxt *= friction;
		top += vxt;
		if(dxt > dx){
			isPreFin = Math.abs(ax) < 2;
			isFin = Math.abs(ax) < 0.02;			
		}
		
		target = screenW;
		float dxr = target - right;
		ax = dxr * spring;
		vxr += ax;
		vxr *= friction;
		right += vxr;
		if(dxr > dx){
			isPreFin = Math.abs(ax) < 2;
			isFin = Math.abs(ax) < 0.02;			
		}
		
		if(isPreFin && !e80 ){
			setEmptyMessage(FlipRenderer.MSG_ANIMATION_PREV_FIN);
			e80 = true;
		}
		
		if(isFin) {	
			setEmptyMessage(FlipRenderer.MSG_ANIMATION_FIN);
			isAnimatorEnd = true;
		}
	}
	// 缓动运动
	private void easingMethod_ease() {
		float rrrr = 0.3f;
		float dx_l = - left * rrrr;
		left += dx_l;

		float dx_t = (screenH - top) * rrrr;
		top += dx_t;
		
		float dx_r = (screenW - right) * rrrr;
		right += dx_r;
		
		float dx_b = - bottom * rrrr;
		bottom += dx_b;
		
		float dx = Math.max(Math.max(dx_l, dx_t), Math.max(dx_r, dx_b));
		if(Math.abs(dx) < 0.02 && !e80 ){
			setEmptyMessage(FlipRenderer.MSG_ANIMATION_PREV_FIN);
			e80 = true;
		}
		
		if(Math.abs(dx) < 0.001) {	
			setEmptyMessage(FlipRenderer.MSG_ANIMATION_FIN);
			isAnimatorEnd = true;
		}
	}

	protected void genBuffers() {
		verticesArr = new float[] {
			left, top, deep,
			right, top, deep,
			left, bottom, deep,
			right,bottom, deep
		};
		
		texCoordArr = new float[]{
				0, 0,
				tw, 0,
				0, th,
				tw, th
		};
		
		vertxIndxArr = new short[]{
				1, 0, 2,
				1, 2, 3
			};
		
		super.genBuffers();
	}
}
