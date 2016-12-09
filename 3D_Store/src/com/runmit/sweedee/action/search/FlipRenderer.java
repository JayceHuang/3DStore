package com.runmit.sweedee.action.search;

import static javax.microedition.khronos.opengles.GL10.GL_AMBIENT;
import static javax.microedition.khronos.opengles.GL10.GL_COLOR_BUFFER_BIT;
import static javax.microedition.khronos.opengles.GL10.GL_DEPTH_BUFFER_BIT;
import static javax.microedition.khronos.opengles.GL10.GL_DEPTH_TEST;
import static javax.microedition.khronos.opengles.GL10.GL_LEQUAL;
import static javax.microedition.khronos.opengles.GL10.GL_LIGHTING;
import static javax.microedition.khronos.opengles.GL10.GL_MODELVIEW;
import static javax.microedition.khronos.opengles.GL10.GL_NICEST;
import static javax.microedition.khronos.opengles.GL10.GL_PERSPECTIVE_CORRECTION_HINT;
import static javax.microedition.khronos.opengles.GL10.GL_POSITION;
import static javax.microedition.khronos.opengles.GL10.GL_PROJECTION;
import static javax.microedition.khronos.opengles.GL10.GL_SMOOTH;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

import com.runmit.sweedee.action.search.DefaultEffectFaces.DefaultEffectFacesParams;

import android.opengl.GLSurfaceView;
import android.opengl.GLU;
import android.os.Handler;
import android.os.Message;
import android.view.View;

public class FlipRenderer implements GLSurfaceView.Renderer {

	protected static final int MSG_ANIMATION_FIN = 10001;

	public static final int MSG_ANIMATION_PREV_FIN = MSG_ANIMATION_FIN+1;
	
	private FlipViewGroup flipViewGroup;

	private DefaultEffectFaces paperfaces;
	
	private boolean created = false;
	
	private Handler handler = new Handler(){		
		@Override
		public void handleMessage(Message msg) {
			switch (msg.what) {
			case MSG_ANIMATION_FIN:
				flipViewGroup.obtainAniFinished();
				break;
			case MSG_ANIMATION_PREV_FIN:
				flipViewGroup.obtainAniReachFinish();
				break;
			default:
				break;
			}
		}
	};

	private boolean candraw = false;

	public FlipRenderer(FlipViewGroup flipViewGroup, DefaultEffectFacesParams param) {
		this.flipViewGroup = flipViewGroup;
		switch (param.ef_type) {
		case Folding:
			paperfaces = new FoldingFaces(handler, param);	//new CubeFaces(handler, param);//
			break;
		case Scale:
			paperfaces = new ScaleFaces(handler, param);
			break;
		default:
			break;
		}	
	}

	public DefaultEffectFaces getCards() {
		return paperfaces;
	}

	@Override
	public void onSurfaceCreated(GL10 gl, EGLConfig config) {
		gl.glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
		gl.glShadeModel(GL_SMOOTH);
		gl.glClearDepthf(1.0f);
		gl.glEnable(GL_DEPTH_TEST);
		gl.glDepthFunc(GL_LEQUAL);
		gl.glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
		
		created = true;
		
		paperfaces.invalidateTexture();
		flipViewGroup.reloadTexture();
	}
	
	public static float d2r(float degree) {
		return degree * (float) Math.PI / 180f;
	}

	public static float[] light0Position = {0, 0, 100f, 0f};

	@Override
	public void onSurfaceChanged(GL10 gl, int width, int height) {
		gl.glViewport(0, 0, width, height);

		gl.glMatrixMode(GL_PROJECTION);
		gl.glLoadIdentity();

		float fovy = 45f;
		float eyeZ = height / 2f / (float) Math.tan(d2r(fovy / 2));

		GLU.gluPerspective(gl, fovy, (float) width / (float) height, 0.5f, Math.max(2500.0f, eyeZ));

		gl.glMatrixMode(GL_MODELVIEW);
		gl.glLoadIdentity();

		GLU.gluLookAt(gl,
			width / 2.0f, height / 2f, eyeZ,
			width / 2.0f, height / 2.0f, 0.0f,
			0.0f, 1.0f, 0.0f
		);

//		gl.glEnable(GL_LIGHTING);
//		gl.glEnable(GL10.GL_LIGHT0);
		
		light0Position = new float[]{width*.5f, height*.5f, 0, 1.0f};
		gl.glLightfv(GL10.GL_LIGHT0, GL_POSITION, light0Position, 0);
		
		gl.glLightfv(GL10.GL_LIGHT0, GL_AMBIENT, new float[]{3.5f, 3.5f, 3.5f, 3f}, 0);
		gl.glLightfv(GL10.GL_LIGHT0, GL10.GL_DIFFUSE, new float[] { 1.0f, 1.0f, 1.0f, 1.0f }, 0); 
		gl.glLightfv(GL10.GL_LIGHT0, GL10.GL_SPECULAR, new float[] { 0.0f, 0.0f, 1.0f, 1.0f }, 0);
	}

	@Override
	public void onDrawFrame(GL10 gl) {
		gl.glClearColor(0.f, 0.f, 0.f, 0.0f);
		gl.glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		if (candraw) {
			paperfaces.draw(gl);	
		}		
	}

	public void updateTexture(View view) {
		if (created) {
			//Logger.i("updateTexture");
			paperfaces.reloadTexture(view, null);
			candraw = true;
		}
		//flipViewGroup.getSurfaceView().requestRender();
	}

	public static void checkError(GL10 gl) {
		int error = gl.glGetError();
		if (error != 0) {
			throw new RuntimeException(GLU.gluErrorString(error));
		}
	}
}
