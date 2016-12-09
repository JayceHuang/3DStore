package com.runmit.sweedee.action.search;

import android.os.Handler;

public class FoldingFaces extends DefaultEffectFaces {

	private float expandSpeed = 2.0f;
	private int flipNums = 4;			// 摺叠段数

	private float currentPointAngle = 0;// 摺叠夹角的一半(0~90)  0:完全折起， 90:完全展开 
	private final float MAX_FlipAngle = 90;	// 最大展开角
	
	private float friction = 0.90f;
	private float spring = 0.5f;
	private float vx = 0;
	
	public FoldingFaces(Handler handler, DefaultEffectFacesParams params) {
		super(handler, params);
	}
	
	private boolean e80 = false;
	
	protected void updateV() {
		switch (mParams.move_type) {
		case Easying:
			easingMethod_ease();
			break;
		case Spring:
			easingMethod_spring();
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
		currentPointAngle += expandSpeed ;
		
		if(currentPointAngle > MAX_FlipAngle *0.95 && !e80){
			setEmptyMessage(FlipRenderer.MSG_ANIMATION_PREV_FIN);
			e80 = true;
		}
		
		if (currentPointAngle > MAX_FlipAngle) {
			currentPointAngle = MAX_FlipAngle;
			setEmptyMessage(FlipRenderer.MSG_ANIMATION_FIN);
			isAnimatorEnd = true;
		}
	}
	
	private void easingMethod_ease(){
		float dx = ( MAX_FlipAngle - currentPointAngle) / 10 ;
		currentPointAngle += dx ;
		
		if(Math.abs(dx) < 1 && !e80){
			setEmptyMessage(FlipRenderer.MSG_ANIMATION_PREV_FIN);
			e80 = true;
		}
		
		if (Math.abs(dx) < 0.1) {
			currentPointAngle = MAX_FlipAngle;
			setEmptyMessage(FlipRenderer.MSG_ANIMATION_FIN);
			isAnimatorEnd = true;
		}
			
	}
	
	private void easingMethod_spring() {
		float dx = MAX_FlipAngle - currentPointAngle;
		float ax = dx * spring;
		vx += ax;
		vx *= friction;
		
		if(Math.abs(ax) < 0.1 && !e80 ){
			setEmptyMessage(FlipRenderer.MSG_ANIMATION_PREV_FIN);
			e80 = true;
		}
		if(Math.abs(ax) < 0.01){
			setEmptyMessage(FlipRenderer.MSG_ANIMATION_FIN);
			isAnimatorEnd = true;
		}
		currentPointAngle += vx ;
	}
	
	protected void genBuffers() {
		int points= 2 + 4 * flipNums; 	// 顶点数目
		int vers = 3 * points;			// 顶点数组大小
		int texcoors = 2 * points;		// 纹理坐标数组大小
		int verIndxs = 4*flipNums * 3;	// 顶点索引大小
		int triNums = 4 * flipNums *3;		// 三角面数
		
		float w = screenW, h =  screenH / flipNums;
		float margin = 0f, deep = 0f;
		
		verticesArr = new float[vers];			// 清空顶点数组 
		texCoordArr = new float[texcoors];		// 清空纹理坐标数组 
		vertxIndxArr = new short[verIndxs];		// 清空顶点索引数组
		normalizerArr = new float[triNums];		// 法线
				
		/**
		 *  固定顶部  0,1 两点
		 */
		verticesArr[0] = margin;
		verticesArr[1] = screenH;
		verticesArr[2] = deep;
		
		texCoordArr[0] = 0f;
		texCoordArr[1] = 0f;
		
		verticesArr[3] = w - margin;
		verticesArr[4] = screenH;
		verticesArr[5] = deep;
		
		texCoordArr[2] = tw;
		texCoordArr[3] = 0f;
		
		float sinValue = (float) Math.sin(d2r(currentPointAngle));
		float cosValue = (float) Math.cos(d2r(currentPointAngle));
		
		/**
		 *  2,3,4,5 顶点(x,y,z) 纹理(u,v)
		 */
		for(int i = 1; i <= flipNums; i++){
			verticesArr[12*i-6] = margin;
			verticesArr[12*i-5] = (float) (screenH - (i - 1 + 0.5f)* h * sinValue);
			verticesArr[12*i-4] = (float) (- 0.5 * h * cosValue);
			
			texCoordArr[8*i-4] = 0f;
			texCoordArr[8*i-3] = th * (i - 0.5f) / flipNums;
			
			verticesArr[12*i-3] = w - margin;
			verticesArr[12*i-2] = (float) (screenH - (i - 0.5f) * h * sinValue);
			verticesArr[12*i-1] = (float) (- 0.5 * h * cosValue);
			
			texCoordArr[8*i-2] = tw;
			texCoordArr[8*i-1] = th * ( i - 0.5f) / flipNums;
			
			verticesArr[12*i] = margin;
			verticesArr[12*i+1] = (float) (screenH - i * h * sinValue);
			verticesArr[12*i+2] = deep;
			
			texCoordArr[8*i] = 0f;
			texCoordArr[8*i+1] = th * i / flipNums;
			
			verticesArr[12*i+3] = w - margin;
			verticesArr[12*i+4] = (float) (screenH - i * h * sinValue);
			verticesArr[12*i+5] = deep;
			
			texCoordArr[8*i+2] = tw;
			texCoordArr[8*i+3] = th * i / flipNums;			
		}
		
		/**
		 *  顶点索引
		 *  (1,0,2), (1,2,3), (3,2,4), (3,4,5)
		 */
		
		for(int i = 0; i < flipNums; i++){
			vertxIndxArr[12*i] 	 = (short) (4*i+1);
			vertxIndxArr[12*i+1] = (short) (4*i);
			vertxIndxArr[12*i+2] = (short) (4*i+2);
			
			normalizerArr[i] = 0;
			normalizerArr[i+1] = -cosValue;
			normalizerArr[i+2] = sinValue;
			
			
			vertxIndxArr[12*i+3] = (short) (4*i+1);
			vertxIndxArr[12*i+4] = (short) (4*i+2);
			vertxIndxArr[12*i+5] = (short) (4*i+3);

			normalizerArr[i+3] = 0;
			normalizerArr[i+4] = -cosValue;
			normalizerArr[i+5] = sinValue;
			
			vertxIndxArr[12*i+6] = (short) (4*i+3);
			vertxIndxArr[12*i+7] = (short) (4*i+2);
			vertxIndxArr[12*i+8] = (short) (4*i+4);
			
			normalizerArr[i+6] = 0;
			normalizerArr[i+7] = cosValue;
			normalizerArr[i+8] = sinValue;

			
			vertxIndxArr[12*i+9]  = (short) (4*i+3);
			vertxIndxArr[12*i+10] = (short) (4*i+4);
			vertxIndxArr[12*i+11] = (short) (4*i+5);
			
			normalizerArr[i+9] = 0;
			normalizerArr[i+10] = cosValue;
			normalizerArr[i+11] = sinValue;
		}
		
		super.genBuffers();
	}
}
