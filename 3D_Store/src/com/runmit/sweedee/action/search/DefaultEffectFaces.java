package com.runmit.sweedee.action.search;

import static com.runmit.sweedee.action.search.FlipRenderer.checkError;
import static javax.microedition.khronos.opengles.GL10.GL_BLEND;
import static javax.microedition.khronos.opengles.GL10.GL_CW;
import static javax.microedition.khronos.opengles.GL10.GL_CCW;
import static javax.microedition.khronos.opengles.GL10.GL_CLAMP_TO_EDGE;
import static javax.microedition.khronos.opengles.GL10.GL_FLOAT;
import static javax.microedition.khronos.opengles.GL10.GL_ONE_MINUS_SRC_ALPHA;
import static javax.microedition.khronos.opengles.GL10.GL_POSITION;
import static javax.microedition.khronos.opengles.GL10.GL_SRC_ALPHA;
import static javax.microedition.khronos.opengles.GL10.GL_TEXTURE_2D;
import static javax.microedition.khronos.opengles.GL10.GL_TEXTURE_COORD_ARRAY;
import static javax.microedition.khronos.opengles.GL10.GL_TEXTURE_WRAP_S;
import static javax.microedition.khronos.opengles.GL10.GL_TEXTURE_WRAP_T;
import static javax.microedition.khronos.opengles.GL10.GL_TRIANGLE_STRIP;
import static javax.microedition.khronos.opengles.GL10.GL_UNSIGNED_SHORT;
import static javax.microedition.khronos.opengles.GL10.GL_VERTEX_ARRAY;
import static javax.microedition.khronos.opengles.GL10.GL_TRIANGLES;
import static javax.microedition.khronos.opengles.GL10.GL_NORMAL_ARRAY;
import static javax.microedition.khronos.opengles.GL10.GL_COLOR_ARRAY;
import static javax.microedition.khronos.opengles.GL10.GL_CULL_FACE;
import static javax.microedition.khronos.opengles.GL10.GL_FRONT;
import static javax.microedition.khronos.opengles.GL10.GL_BACK;

import java.nio.Buffer;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.FloatBuffer;
import java.nio.ShortBuffer;

import javax.microedition.khronos.opengles.GL10;

import com.umeng.analytics.MobclickAgent;

import android.graphics.Bitmap;
import android.os.Handler;
import android.view.View;

public abstract class DefaultEffectFaces {
	
	private Texture textureFromView;	// 由view生成的texture
	private Bitmap viewCacheBmp;		// view 位图
	
	protected float[] verticesArr;		// 顶点数组 
	protected float[] texCoordArr;		// 纹理坐标数组
	protected short[] vertxIndxArr;		// 顶点索引 
	protected float[] normalizerArr;		// 法向量
	protected float[] colorArr;
	
	private FloatBuffer vertexBuffer;	// 顶点数组缓冲
	private ShortBuffer indexBuffer;	// 纹理坐标数组缓冲
	private FloatBuffer textureBuffer;	// 顶点索引 缓冲
	private FloatBuffer normalBuffer;		// 法向量
	private FloatBuffer colorBuffer;
	
	protected float screenW;				// 频幕宽
	protected float screenH;				// 频幕高
	protected float tw;
	protected float th;
	
	protected DefaultEffectFacesParams mParams;
	private Handler mHandler;			// 动画回调处理
	protected boolean isAnimatorEnd = false; // 动画结束
	

	private int frames;

	private long startTime;

	private long elapseTime;
	protected float mRotateX;
	protected float mRotateY;
	private float ddddy;
	
	// bg =============================================
	/*private float[] verticesBG;		// 顶点数组 
	private float[] texCoordBG;		// 纹理坐标数组

	private Texture textureFromBG;	// 由bg生成的texture
	private Bitmap bgCacheBmp;		// 背景位图
	private FloatBuffer vertexGBBuffer;
	private FloatBuffer textureBGBuffer;
	private ShortBuffer indexBGBuffer;	// 纹理坐标数组缓冲
	static boolean isset = false;
	*/
	public DefaultEffectFaces(Handler handler, DefaultEffectFacesParams params) {
		mHandler = handler;
		mParams = params;
		frames = 0;
		elapseTime = 1000000000 / 60;
		mRotateX = mRotateY = 0;
		ddddy = 0;
	}
	
	/**
	 *  设置纹理
	 * @param gl
	 */
	protected void applyTexture(GL10 gl) {
		if (viewCacheBmp != null) {
			if (textureFromView != null)
				textureFromView.destroy(gl);

			textureFromView = Texture.createTexture(viewCacheBmp, gl);
			
			screenW = viewCacheBmp.getWidth();
			screenH = viewCacheBmp.getHeight();
			
			tw = viewCacheBmp.getWidth() / (float) textureFromView.getWidth();
			th = viewCacheBmp.getHeight() / (float) textureFromView.getHeight();
			
			checkError(gl);
			
			viewCacheBmp.recycle();
			viewCacheBmp = null;
		}		
	}
		
	/**
	 *  清除纹理
	 */
	public void invalidateTexture() {
		//Texture is vanished when the gl context is gone, no need to delete it explicitly
		textureFromView = null;
	}
	
	/**
	 *  重新截图
	 * @param 新传入的view
	 */
	public void reloadTexture(View v, Bitmap bg) {
		viewCacheBmp = Activity3DEffectTool.takeScreenshot(v);
	}
			
	// 线性运动
	public void draw(GL10 gl) {
		applyTexture(gl);
		
		if (isAnimatorEnd) {
			drawMethod(gl);
			return;
		}
		if(System.nanoTime() - startTime >= elapseTime) {  
            frames = 0;
            startTime = System.nanoTime();
            updateV();

    		ddddy +=5;
        }
		
		drawMethod(gl);
	}

	protected abstract void updateV();
	
	protected void genBuffers() {
		vertexBuffer = toFloatBuffer(verticesArr);
		indexBuffer = toShortBuffer(vertxIndxArr);
		
		if (texCoordArr != null) {
			textureBuffer = toFloatBuffer(texCoordArr);	
		}
		if (colorArr != null) {
			colorBuffer= toFloatBuffer(colorArr);
		}
		
		if (normalizerArr != null) {
			normalBuffer = toFloatBuffer(normalizerArr);
		}		
	}

	/**
	 * opengl 绘图
	 * @param gl
	 */
	public void drawMethod(GL10 gl) {
		
		if (vertexBuffer == null)
			return;

		gl.glFrontFace(GL_CCW);

		gl.glEnable(GL_CULL_FACE);
		gl.glCullFace(GL_BACK);

		gl.glEnableClientState(GL_VERTEX_ARRAY);
		if (normalizerArr != null) {
			gl.glEnableClientState(GL_NORMAL_ARRAY);
			gl.glNormalPointer(GL_FLOAT, 0, normalBuffer);
//			String logstr = "[";
//			int i;
//			for ( i= 0; i < normalizerArr.length-1; i++) {
//				logstr += normalizerArr[i]+",";
//			}
//			logstr += normalizerArr[i]+"]"; 
//			Log.d("mzh", "nor:" + logstr);
			checkError(gl);
		}

		gl.glEnable(GL_BLEND);
		gl.glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		//gl.glBlendFunc(GL_SRC_ALPHA, GL_DST_ALPHA);
//		float[] light0Position = new float[]{screenW*.5f, screenH*.5f, (float) (200 * Math.sin(d2r(ddddy)))+ 200, 1f};
//		gl.glLightfv(GL10.GL_LIGHT0, GL_POSITION, light0Position, 0);
		gl.glColor4f(1f, 1.0f, 1f, 1.0f);
		
		if (colorArr != null) {
			gl.glEnableClientState(GL_COLOR_ARRAY);
			gl.glColorPointer(4, GL_FLOAT, 0,  colorBuffer);
			checkError(gl);
		}
		

		// =====================================================================================
		if (texCoordArr != null && isValidTexture(textureFromView)) {
			gl.glEnable(GL_TEXTURE_2D);
			gl.glEnableClientState(GL_TEXTURE_COORD_ARRAY);
			gl.glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			gl.glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			gl.glTexCoordPointer(2, GL_FLOAT, 0, textureBuffer);
			
			gl.glBindTexture(GL_TEXTURE_2D, textureFromView.getId()[0]);
			checkError(gl);
		}
		
		gl.glPushMatrix();
//		gl.glTranslatef(350, 400, 200);
//		gl.glRotatef(mRotateX, 1, 0, 0);
//		gl.glRotatef(mRotateY, 0, 1, 0);
//		gl.glTranslatef(100f, 200f, 0f);
//		gl.glScalef(.8f, .8f, 1f);		
		gl.glVertexPointer(3, GL_FLOAT, 0, vertexBuffer);
		gl.glDrawElements(GL_TRIANGLES, vertxIndxArr.length, GL_UNSIGNED_SHORT, indexBuffer);
//		gl.glDrawElements(GL_LINES, indices.length, GL_UNSIGNED_SHORT, indexBuffer);
		checkError(gl);
		
		gl.glPopMatrix();

		if (texCoordArr != null && isValidTexture(textureFromView)) {
			gl.glDisableClientState(GL_TEXTURE_COORD_ARRAY);
			gl.glDisable(GL_TEXTURE_2D);
		}
		
		checkError(gl);
		
		if (colorArr != null) {
			gl.glDisableClientState(GL_COLOR_ARRAY);
		}

		gl.glDisable(GL_BLEND);
		if (normalizerArr != null) {
			gl.glDisableClientState(GL_NORMAL_ARRAY);
		}
		gl.glDisableClientState(GL_VERTEX_ARRAY);
		gl.glDisable(GL_CULL_FACE);
	}
	
	/**
	 * 检测纹理是否可用
	 * @param t
	 * @return
	 */
	private boolean isValidTexture(Texture t) {
		return t != null && !t.isDestroyed();
	}
		
	// 其他 ===================================================================================
	
	protected void setEmptyMessage(int MSG){
		mHandler.sendEmptyMessage(MSG);
	}
	
	protected float d2r(float _angle) {
		return (float) (Math.PI * _angle /180.0f);
	}
	
	private FloatBuffer toFloatBuffer(float[] v) {
		ByteBuffer buf = ByteBuffer.allocateDirect(v.length * 4);
		buf.order(ByteOrder.nativeOrder());
		FloatBuffer buffer = buf.asFloatBuffer();
		buffer.put(v);
		buffer.position(0);
		return buffer;
	}

	private ShortBuffer toShortBuffer(short[] v) {
		ByteBuffer buf = ByteBuffer.allocateDirect(v.length * 2);
		buf.order(ByteOrder.nativeOrder());
		ShortBuffer buffer = buf.asShortBuffer();
		buffer.put(v);
		buffer.position(0);
		return buffer;
	}
	
	public static class DefaultEffectFacesParams {
		public static enum EffectType {
			Folding,
			Scale
		}

		public static enum EasyingType {
			Normal,
			Easying,
			Spring
		}
		
		EffectType ef_type;
		EasyingType move_type;

        public int scaleRange [];

		public DefaultEffectFacesParams(){
			ef_type = EffectType.Folding;
			move_type = EasyingType.Easying;
		}
		
		public DefaultEffectFacesParams(EffectType ef, EasyingType ease){
			this.ef_type = ef;
			this.move_type = ease;
		}
	}
}
