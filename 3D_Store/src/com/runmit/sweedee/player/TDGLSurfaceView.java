package com.runmit.sweedee.player;

import java.io.IOException;
import java.net.ResponseCache;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.FloatBuffer;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

import com.runmit.sweedee.R;
import com.runmit.sweedee.report.sdk.ReportActionSender;
import com.runmit.sweedee.util.Util;
import com.runmit.sweedee.util.XL_Log;
import com.superd.sdsdk.SDDevice;
import com.superd.sdsdk.SDDriverType;
import com.superd.sdsdk.SDImageOrder;
import com.superd.sdsdk.SDTrackingListener;

import android.annotation.TargetApi;
import android.content.Context;
import android.graphics.Point;
import android.graphics.SurfaceTexture;
import android.graphics.SurfaceTexture.OnFrameAvailableListener;
import android.media.AudioManager;
import android.media.MediaPlayer;
import android.media.MediaPlayer.OnBufferingUpdateListener;
import android.media.MediaPlayer.OnCompletionListener;
import android.media.MediaPlayer.OnErrorListener;
import android.media.MediaPlayer.OnInfoListener;
import android.media.MediaPlayer.OnPreparedListener;
import android.media.MediaPlayer.OnSeekCompleteListener;
import android.media.MediaPlayer.OnVideoSizeChangedListener;
import android.opengl.GLES11Ext;
import android.opengl.GLES20;
import android.opengl.GLSurfaceView;
import android.opengl.Matrix;
import android.os.Build;
import android.os.Handler;
import android.util.AttributeSet;
import android.util.Log;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.widget.Toast;

public class TDGLSurfaceView extends GLSurfaceView {
	private XL_Log log = new XL_Log(TDGLSurfaceView.class);

	private SurfaceHolder mSurfaceHolder = null;
	private MediaPlayer mMediaPlayer = null;
	private SDDevice mDevice;

	private int mode;

	private Surface mSurface = null;

	private final static int ERRORCODE_OPEN_FLASHLIGHT = 0x101;

	private On3DStateChangeListener mOn3DStateChangeListener;

	public void setOn3DStateChangeListener(On3DStateChangeListener mOn3DStateChangeListener) {
		this.mOn3DStateChangeListener = mOn3DStateChangeListener;
	}

	public void setMode(int mode) {
		log.debug("mode=" + mode);
		this.mode = mode;
		Init();
	}

	private Handler mCallBackHandler = new Handler();

	public TDGLSurfaceView(Context context) {
		super(context);
	}

	public TDGLSurfaceView(Context context, AttributeSet attrs) {
		super(context, attrs);
	}

	private long eye_track_failed_start_time = 0;
	private long eye_track_failed_during_time = 0;
	private boolean isEyteTrackingFailed = false;
	private boolean eye_track_failed_report = true;
	private static final int REPORT_TRACK_FAILEDS_CALE_TIME_4000 = 4000;

	private boolean detect3DPhone() {
		boolean is3DPhone = false;
		mDevice = SDDevice.createSDDevice(TDGLSurfaceView.this, SDDriverType.SD_DRIVER_GLES20);		
		if (mDevice != null) {
			is3DPhone = true;
			mDevice.registerTrackingListener(new SDTrackingListener() {
				@Override
				public void onTracking(boolean tracked, float arg1, float arg2, float arg3) {
					if (!tracked && !isEyteTrackingFailed) {
						eye_track_failed_start_time = System.currentTimeMillis();
						isEyteTrackingFailed = true;
					}
					if (!tracked) {
						eye_track_failed_during_time = (int) (System.currentTimeMillis() - eye_track_failed_start_time);
						if (eye_track_failed_during_time >= REPORT_TRACK_FAILEDS_CALE_TIME_4000 && eye_track_failed_report) {
							ReportActionSender.getInstance().eyeTrack(0, eye_track_failed_during_time);
							eye_track_failed_report = false;
						}
					} else {
						eye_track_failed_during_time = 0;
						isEyteTrackingFailed = false;
						eye_track_failed_report = true;
						eye_track_failed_start_time = 0;
					}

				}
			});
		}
		return is3DPhone;
	}

	private void Init() {
		this.setEGLContextClientVersion(2);
		// 暂时辨别3D手机的后缀为彩虹
		if (mode < 4 && detect3DPhone()) {// mode小于4代表3d视频，16是2d视频
			log.debug("We are using 3d render");
			this.setRenderer(new TDSuper3DGLRender());
		} else {
			log.debug("We are using 2d render");
			this.setRenderer(new TDNormalGLRender());
		}
		this.setRenderMode(GLSurfaceView.RENDERMODE_WHEN_DIRTY);
		mSurfaceHolder = this.getHolder();
	}

	boolean surfaceDestroyed = false;

	public void surfaceDestroyed(SurfaceHolder holder) {
		super.surfaceDestroyed(holder);
		log.debug("surfaceDestroyed mDevice=" + mDevice);
		surfaceDestroyed = true;
		disable3D(false);
		mDevice = null;
		if (mOnSurfaceCallBackListener != null) {
			mOnSurfaceCallBackListener.surfaceDestroyed(holder);
		}
	}

	public void surfaceCreated(SurfaceHolder holder) {
		super.surfaceCreated(holder);
		log.debug("surfaceCreated");
		if (surfaceDestroyed) {
			if (mOnSurfaceCallBackListener != null) {
				mOnSurfaceCallBackListener.surfaceCreated(holder);
			}
			surfaceDestroyed = false;
		}
	}

	public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
		super.surfaceChanged(holder, format, width, height);
		log.debug("surfaceChanged");
		if (mOnSurfaceCallBackListener != null) {
			mOnSurfaceCallBackListener.surfaceChanged(holder, format, width, height);
		}
	}

	// 3D 渲染开始
	class TDSuper3DGLRender implements GLSurfaceView.Renderer, OnFrameAvailableListener {
		private boolean mUpdateVideoFrame = false;
		private int mTextureId = 0;
		private SurfaceTexture mSurfaceTexture = null;

		public TDSuper3DGLRender() {

		}

		public void destroyContext() {
		}

		public int genTexture() {
			int[] textureId = new int[1];
			GLES20.glGenTextures(1, textureId, 0);
			GLES20.glBindTexture(GLES11Ext.GL_TEXTURE_EXTERNAL_OES, textureId[0]);
			GLES20.glTexParameterf(GLES11Ext.GL_TEXTURE_EXTERNAL_OES, GL10.GL_TEXTURE_MIN_FILTER, GL10.GL_LINEAR);
			GLES20.glTexParameterf(GLES11Ext.GL_TEXTURE_EXTERNAL_OES, GL10.GL_TEXTURE_MAG_FILTER, GL10.GL_LINEAR);
			GLES20.glBindTexture(GLES11Ext.GL_TEXTURE_EXTERNAL_OES, 0);
			return textureId[0];
		}

		@Override
		public void onSurfaceCreated(GL10 gl10, EGLConfig config) {
			log.debug("onSurfaceCreated");
			if (mDevice == null) {
				mDevice = SDDevice.createSDDevice(TDGLSurfaceView.this, SDDriverType.SD_DRIVER_GLES20);
			}
			mTextureId = genTexture();

			mSurfaceTexture = new SurfaceTexture(mTextureId);
			mSurfaceTexture.setOnFrameAvailableListener(this);
			mSurface = new Surface(mSurfaceTexture);
			requestRender();

			mCallBackHandler.post(new Runnable() {
				@Override
				public void run() {
					if (mOnSurfaceCallBackListener != null) {
						mOnSurfaceCallBackListener.surfaceCreated(mSurfaceHolder);
					}
				}
			});
		}

		@TargetApi(Build.VERSION_CODES.JELLY_BEAN_MR1)
		@Override
		public void onSurfaceChanged(GL10 gl10, int width, int height) {
			log.debug("onSurfaceChanged=" + width + ",height=" + height);
			if (width > height) {// 判断在横屏下，因为锁屏时候会回调此方法，回调时候width < height
				Point screenSize = new Point();
				int[] viewOnScreen = new int[2];
				getDisplay().getRealSize(screenSize);
				getLocationOnScreen(viewOnScreen);
				if (mDevice == null) {
					mDevice = SDDevice.createSDDevice(TDGLSurfaceView.this, SDDriverType.SD_DRIVER_GLES20);
				}

				mDevice.initEGLContext(screenSize.x, screenSize.y, viewOnScreen[0], viewOnScreen[1], width, height);
				enable3D(false);
				mDevice.setTexture(GLES11Ext.GL_TEXTURE_EXTERNAL_OES, mTextureId, mode);
			}
		}

		@Override
		public void onDrawFrame(GL10 gl) {
			synchronized (TDGLSurfaceView.this) {
				if (mUpdateVideoFrame == true && mSurfaceTexture != null) {
					float mtx[] = new float[16];
					mUpdateVideoFrame = false;
					mSurfaceTexture.updateTexImage();
					mSurfaceTexture.getTransformMatrix(mtx);
					if (mDevice == null) {
						mDevice = SDDevice.createSDDevice(TDGLSurfaceView.this, SDDriverType.SD_DRIVER_GLES20);
					}
					if (mDevice != null) {
						mDevice.setTextureTransformMatrix(mtx);
						mDevice.draw();
					}
				}
			}
		}

		@Override
		public void onFrameAvailable(SurfaceTexture paramSurfaceTexture) {
			synchronized (TDGLSurfaceView.this) {
				mUpdateVideoFrame = true;
			}
			requestRender();
		}
	}

	// 3D 渲染结束

	// 2D 渲染开始
	class TDNormalGLRender implements GLSurfaceView.Renderer, SurfaceTexture.OnFrameAvailableListener {
		private XL_Log log = new XL_Log(TDSuper3DGLRender.class);

		private static final int FLOAT_SIZE_BYTES = 4;
		private static final int TRIANGLE_VERTICES_DATA_STRIDE_BYTES = 5 * FLOAT_SIZE_BYTES;
		private static final int TRIANGLE_VERTICES_DATA_POS_OFFSET = 0;
		private static final int TRIANGLE_VERTICES_DATA_UV_OFFSET = 3;

		private final float[] mTriangleVerticesData = {
				// X, Y, Z, U, V
				-1.0f, -1.0f, 0, 0.f, 0.f, 1.0f, -1.0f, 0, 1.f, 0.f, -1.0f, 1.0f, 0, 0.f, 1.f, 1.0f, 1.0f, 0, 1.f, 1.f, };

		private FloatBuffer mTriangleVertices;

		private final String mVertexShader = "uniform mat4 uMVPMatrix;\n" + "uniform mat4 uSTMatrix;\n" + "attribute vec4 aPosition;\n" + "attribute vec4 aTextureCoord;\n"
				+ "varying vec2 vTextureCoord;\n" + "void main() {\n" + "  gl_Position = uMVPMatrix * aPosition;\n" + "  vTextureCoord = (uSTMatrix * aTextureCoord).xy;\n" + "}\n";

		private final String mFragmentShader = "#extension GL_OES_EGL_image_external : require\n" + "precision highp float;\n" + "varying vec2 vTextureCoord;\n"
				+ "uniform samplerExternalOES sTexture;\n" + "uniform vec3 winSize;\n" + "uniform int rMode;\n" + "void main() {\n" + "" + "	vec2 UV2d = vec2(1.0, 1.0);\n" + " 	if(rMode == 1){\n"
				+ "		UV2d = vec2(0.5, 1.0);\n" + "	}\n" + "	else if(rMode == 2){\n" + "		UV2d = vec2(1.0, 0.5);\n" + "	}\n" +

				"	if(winSize.z<1.0)\n" + "	{\n" + "		if(vTextureCoord.x <= winSize.x)" + "			gl_FragColor = texture2D(sTexture, vTextureCoord*UV2d);\n" + "	}\n" + "	else\n" + "	{\n" +

				"		if(int(mod(vTextureCoord.x*winSize.x, 2.0)) == 1) \n" + "		{\n" + "			gl_FragColor = texture2D(sTexture, vTextureCoord*vec2(0.5, 1));\n" + "		}\n" + "		else \n" + "		{\n"
				+ "			gl_FragColor = texture2D(sTexture, vec2(0.5,0)+vTextureCoord*vec2(0.5, 1));\n" + "		}\n" + "	}\n" + "}\n";

		private float[] mMVPMatrix = new float[16];
		private float[] mSTMatrix = new float[16];
		private float[] mWinSize = new float[3];

		private int mProgram;
		private int mTextureID;
		private int muMVPMatrixHandle;
		private int muSTMatrixHandle;
		private int maPositionHandle;
		private int maTextureHandle;
		private int mWinSizeHandle;
		private int mRenderModeHandle;

		private SurfaceTexture mSurfaceTexture;
		private boolean updateSurface = false;
		private boolean mIsLandScreen = false;

		private int viewportWidth = 0;
		private int viewportHeight = 0;
		private float viewportState = 0.5f;

		public TDNormalGLRender() {
			mTriangleVertices = ByteBuffer.allocateDirect(mTriangleVerticesData.length * FLOAT_SIZE_BYTES).order(ByteOrder.nativeOrder()).asFloatBuffer();
			mTriangleVertices.put(mTriangleVerticesData).position(0);
			Matrix.setIdentityM(mSTMatrix, 0);
		}

		public void destroyContext() {
			mSurface.release();
		}

		public void onDrawFrame(GL10 glUnused) {
			synchronized (this) {
				if (updateSurface) {
					mSurfaceTexture.updateTexImage();
					mSurfaceTexture.getTransformMatrix(mSTMatrix);
					updateSurface = false;
				}
			}

			mWinSize[0] = viewportWidth;
			mWinSize[1] = viewportHeight;
			mWinSize[2] = viewportState;

			GLES20.glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
			GLES20.glClear(GLES20.GL_DEPTH_BUFFER_BIT | GLES20.GL_COLOR_BUFFER_BIT);

			GLES20.glUseProgram(mProgram);
			checkGlError("glUseProgram");

			GLES20.glActiveTexture(GLES20.GL_TEXTURE0);
			GLES20.glBindTexture(GLES11Ext.GL_TEXTURE_EXTERNAL_OES, mTextureID);

			mTriangleVertices.position(TRIANGLE_VERTICES_DATA_POS_OFFSET);
			GLES20.glVertexAttribPointer(maPositionHandle, 3, GLES20.GL_FLOAT, false, TRIANGLE_VERTICES_DATA_STRIDE_BYTES, mTriangleVertices);
			checkGlError("glVertexAttribPointer maPosition");
			GLES20.glEnableVertexAttribArray(maPositionHandle);
			checkGlError("glEnableVertexAttribArray maPositionHandle");

			mTriangleVertices.position(TRIANGLE_VERTICES_DATA_UV_OFFSET);
			GLES20.glVertexAttribPointer(maTextureHandle, 3, GLES20.GL_FLOAT, false, TRIANGLE_VERTICES_DATA_STRIDE_BYTES, mTriangleVertices);
			checkGlError("glVertexAttribPointer maTextureHandle");
			GLES20.glEnableVertexAttribArray(maTextureHandle);
			checkGlError("glEnableVertexAttribArray maTextureHandle");

			Matrix.setIdentityM(mMVPMatrix, 0);
			GLES20.glViewport(0, 0, viewportWidth, viewportHeight);
			GLES20.glUniformMatrix4fv(muMVPMatrixHandle, 1, false, mMVPMatrix, 0);
			GLES20.glUniformMatrix4fv(muSTMatrixHandle, 1, false, mSTMatrix, 0);
			GLES20.glUniform3fv(mWinSizeHandle, 1, mWinSize, 0);
			int m = checkMode();
			GLES20.glUniform1i(mRenderModeHandle, m);

			GLES20.glDrawArrays(GLES20.GL_TRIANGLE_STRIP, 0, 4);
			checkGlError("glDrawArrays");
			GLES20.glFinish();
		}

		private int checkMode() {
			if (mode == SDImageOrder.SDS3D_ORDER_LEFT_RIGHT || mode == SDImageOrder.SDS3D_ORDER_RIGHT_LEFT)
				return 1;
			if (mode == SDImageOrder.SDS3D_ORDER_TOP_BOTTOM || mode == SDImageOrder.SDS3D_ORDER_BOTTOM_TOP)
				return 2;
			return 0;
		}

		public void onSurfaceChanged(GL10 glUnused, int width, int height) {
			log.debug("onSurfaceChanged size: " + width + "x" + height);
			log.debug("Video size viewportWidth: " + viewportWidth + ",video_heigh:" + viewportHeight);
			viewportWidth = width;
			viewportHeight = height;
			// mIsLandScreen = false
			// /*PreferencesUtil.getIsPlayScreenLand(mContext)*/;
			// if(mIsLandScreen){
			// log.debug("onSurfaceChanged 3D ");
			// viewportState = 2.0f; //横屏的时候就是播放左右图 放3D
			// viewportState = 0.5f;
			// }else{
			// log.debug("onSurfaceChanged 2D ");
			// viewportState = 0.5f; //竖屏的时候就是播放半图 放2D
			// }

			viewportState = 0.5f;
			GLES20.glViewport(0, 0, viewportWidth, viewportHeight);
		}

		public void onSurfaceCreated(GL10 glUnused, EGLConfig config) {
			log.debug("onSurfaceCreated");

			mProgram = createProgram(mVertexShader, mFragmentShader);
			if (mProgram == 0) {
				return;
			}
			maPositionHandle = GLES20.glGetAttribLocation(mProgram, "aPosition");
			checkGlError("glGetAttribLocation aPosition");
			if (maPositionHandle == -1) {
				throw new RuntimeException("Could not get attrib location for aPosition");
			}
			maTextureHandle = GLES20.glGetAttribLocation(mProgram, "aTextureCoord");
			checkGlError("glGetAttribLocation aTextureCoord");
			if (maTextureHandle == -1) {
				throw new RuntimeException("Could not get attrib location for aTextureCoord");
			}

			muMVPMatrixHandle = GLES20.glGetUniformLocation(mProgram, "uMVPMatrix");
			checkGlError("glGetUniformLocation uMVPMatrix");
			if (muMVPMatrixHandle == -1) {
				throw new RuntimeException("Could not get attrib location for uMVPMatrix");
			}

			muSTMatrixHandle = GLES20.glGetUniformLocation(mProgram, "uSTMatrix");
			checkGlError("glGetUniformLocation uSTMatrix");
			if (muSTMatrixHandle == -1) {
				throw new RuntimeException("Could not get attrib location for uSTMatrix");
			}

			mWinSizeHandle = GLES20.glGetUniformLocation(mProgram, "winSize");
			checkGlError("glGetUniformLocation winSize");
			if (mWinSizeHandle == -1) {
				throw new RuntimeException("Could not get attrib location for mWinSizeHandle");
			}

			mRenderModeHandle = GLES20.glGetUniformLocation(mProgram, "rMode");
			checkGlError("glGetUniformLocation rMode");
			if (mRenderModeHandle == -1) {
				throw new RuntimeException("Could not get uniform location for mRenderModeHandle");
			}

			synchronized (this) {
				updateSurface = false;
			}

			int[] textures = new int[1];
			GLES20.glGenTextures(1, textures, 0);

			mTextureID = textures[0];
			GLES20.glBindTexture(GLES11Ext.GL_TEXTURE_EXTERNAL_OES, mTextureID);
			checkGlError("glBindTexture mTextureID");

			// Can't do mipmapping with mediaplayer source
			GLES20.glTexParameteri(GLES11Ext.GL_TEXTURE_EXTERNAL_OES, GLES20.GL_TEXTURE_MIN_FILTER, GLES20.GL_NEAREST);
			GLES20.glTexParameteri(GLES11Ext.GL_TEXTURE_EXTERNAL_OES, GLES20.GL_TEXTURE_MAG_FILTER, GLES20.GL_LINEAR);
			// Clamp to edge is the only option
			GLES20.glTexParameteri(GLES11Ext.GL_TEXTURE_EXTERNAL_OES, GLES20.GL_TEXTURE_WRAP_S, GLES20.GL_CLAMP_TO_EDGE);
			GLES20.glTexParameteri(GLES11Ext.GL_TEXTURE_EXTERNAL_OES, GLES20.GL_TEXTURE_WRAP_T, GLES20.GL_CLAMP_TO_EDGE);
			checkGlError("glTexParameteri mTextureID");

			mSurfaceTexture = new SurfaceTexture(mTextureID);
			mSurfaceTexture.setOnFrameAvailableListener(this);
			mSurface = new Surface(mSurfaceTexture);

			requestRender();

			if (mCallBackHandler != null) {
				mCallBackHandler.post(new Runnable() {
					@Override
					public void run() {
						if (mOnSurfaceCallBackListener != null) {
							mOnSurfaceCallBackListener.surfaceCreated(mSurfaceHolder);
						}
					}
				});
			}
		}

		synchronized public void onFrameAvailable(SurfaceTexture surface) {
			synchronized (TDGLSurfaceView.this) {
				updateSurface = true;
			}
			requestRender();
		}

		private int loadShader(int shaderType, String source) {
			int shader = GLES20.glCreateShader(shaderType);
			if (shader != 0) {
				GLES20.glShaderSource(shader, source);
				GLES20.glCompileShader(shader);
				int[] compiled = new int[1];
				GLES20.glGetShaderiv(shader, GLES20.GL_COMPILE_STATUS, compiled, 0);
				if (compiled[0] == 0) {
					log.error("Could not compile shader " + shaderType + ":");
					log.error(GLES20.glGetShaderInfoLog(shader));
					GLES20.glDeleteShader(shader);
					shader = 0;
				}
			}
			return shader;
		}

		private int createProgram(String vertexSource, String fragmentSource) {
			int vertexShader = loadShader(GLES20.GL_VERTEX_SHADER, vertexSource);
			if (vertexShader == 0) {
				return 0;
			}
			int pixelShader = loadShader(GLES20.GL_FRAGMENT_SHADER, fragmentSource);
			if (pixelShader == 0) {
				return 0;
			}
			int program = GLES20.glCreateProgram();
			if (program != 0) {
				GLES20.glAttachShader(program, vertexShader);
				checkGlError("glAttachShader");
				GLES20.glAttachShader(program, pixelShader);
				checkGlError("glAttachShader");
				GLES20.glLinkProgram(program);
				int[] linkStatus = new int[1];
				GLES20.glGetProgramiv(program, GLES20.GL_LINK_STATUS, linkStatus, 0);
				if (linkStatus[0] != GLES20.GL_TRUE) {
					log.error("Could not link program: ");
					log.error(GLES20.glGetProgramInfoLog(program));
					GLES20.glDeleteProgram(program);
					program = 0;
				}
			}
			return program;
		}

		private void checkGlError(String op) {
			int error;
			while ((error = GLES20.glGetError()) != GLES20.GL_NO_ERROR) {
				log.error(op + ": glError " + error);
				throw new RuntimeException(op + ": glError " + error);
			}
		}
	}

	// 2D 渲染结束
	public void prepareAsync(String url) {
		log.debug("mMediaPlayer=" + mMediaPlayer);
		if (mMediaPlayer != null) {
			log.debug("isPlaying=" + mMediaPlayer.isPlaying());
			if (mMediaPlayer.isPlaying()) {
				mMediaPlayer.stop();
			}
			mMediaPlayer.release();
		}
		log.debug("url=" + url + ",mSurface=" + mSurface);
		mMediaPlayer = new MediaPlayer();
		mMediaPlayer.setAudioStreamType(AudioManager.STREAM_MUSIC);
		mMediaPlayer.setOnPreparedListener(new OnPreparedListener() {
			@Override
			public void onPrepared(MediaPlayer mp) {
				log.debug("onPrepared!!!!+++++++++");
				if (mOnPlayerCallBackListener != null) {
					mOnPlayerCallBackListener.onPrepared(mp);
				}
			}
		});
		mMediaPlayer.setOnCompletionListener(new OnCompletionListener() {
			@Override
			public void onCompletion(MediaPlayer mp) {
				log.debug("mVideo onCompletion. ");
				if (mOnPlayerCallBackListener != null) {
					mOnPlayerCallBackListener.onCompletion(mp);
				}
			}
		});
		// error
		mMediaPlayer.setOnErrorListener(new OnErrorListener() {
			@Override
			public boolean onError(MediaPlayer mp, int what, int extra) {
				if (mOnPlayerCallBackListener != null) {
					mOnPlayerCallBackListener.onError(mp, what, extra);
				}
				return true;
			}
		});

		mMediaPlayer.setOnSeekCompleteListener(new OnSeekCompleteListener() {

			@Override
			public void onSeekComplete(MediaPlayer mp) {
				if (mOnPlayerCallBackListener != null) {
					mOnPlayerCallBackListener.onSeekComplete(mp);
				}
			}
		});

		mMediaPlayer.setOnVideoSizeChangedListener(new OnVideoSizeChangedListener() {

			@Override
			public void onVideoSizeChanged(MediaPlayer mp, int width, int height) {
				if (mOnPlayerCallBackListener != null) {
					mOnPlayerCallBackListener.onVideoSizeChanged(mp, width, height);
				}
			}
		});

		mMediaPlayer.setOnBufferingUpdateListener(new OnBufferingUpdateListener() {

			@Override
			public void onBufferingUpdate(MediaPlayer mp, int percent) {
				if (mOnPlayerCallBackListener != null) {
					mOnPlayerCallBackListener.onBufferingUpdate(mp, percent);
				}
			}
		});

		mMediaPlayer.setOnInfoListener(new OnInfoListener() {

			@Override
			public boolean onInfo(MediaPlayer mp, int what, int extra) {
				if (mOnPlayerCallBackListener != null) {
					return mOnPlayerCallBackListener.onInfo(mp, what, extra);
				}
				return false;
			}
		});

		if (mSurface != null) {
			mMediaPlayer.setSurface(mSurface);
		}

		try {
			mMediaPlayer.setDataSource(url);
			mMediaPlayer.prepareAsync();
		} catch (IllegalStateException e) {
			e.printStackTrace();
		} catch (IllegalArgumentException e) {
			e.printStackTrace();
		} catch (SecurityException e) {
			e.printStackTrace();
		} catch (IOException e) {
			e.printStackTrace();
		}

	}

	public void start() {
		log.debug("mMediaPlayer.start() mMediaPlayer=");
		try {
			if (mMediaPlayer != null) {
				mMediaPlayer.start();
			}
		} catch (IllegalStateException e) {
			e.printStackTrace();
		}
		enable3D(false);
	}

	private boolean isEnable3D = false;

	private boolean isUserDisable = false;

	/**
	 * 关闭时候无论什么情况都让它关闭3D效果，比如场景：切后台，退出播放器等
	 * 
	 * @param isUser
	 *            :是否用户主动关闭
	 */
	public void disable3D(boolean isUser) {
		int fromUser = 0;
		if (isUser) {
			fromUser = 1;
		}
		log.debug("mDevice=" + mDevice + ",isEnable3D=" + isEnable3D + ",isUser=" + isUser + ",isUserDisable=" + isUserDisable);
		if (mDevice != null && isEnable3D) {
			mDevice.disable3D();
			isEnable3D = false;
			isUserDisable = isUser;
			if (mOn3DStateChangeListener != null) {
				mOn3DStateChangeListener.onChange(isEnable3D);
			}
			ReportActionSender.getInstance().enable3D(0, 2, fromUser, 1, System.currentTimeMillis());
			return;
		}
		ReportActionSender.getInstance().enable3D(0, 2, fromUser, 0, System.currentTimeMillis());
	}

	private boolean isShowToast = false;

	/**
	 * 打开时候需要根据上一次是否是用户操作
	 * 
	 * @param isUser
	 *            :是否用户主动打开
	 */
	public void enable3D(boolean isUser) {
		int fromUser = 0;
		if (isUser) {
			fromUser = 1;
		}
		boolean can = isUser || (!isUser && !isUserDisable);// 如果此动作是用户操作那肯定可以打开，如果当次动作不是用户操作,那上一次操作也必须不是用户操作
		log.debug("mDevice=" + mDevice + ",isEnable3D=" + isEnable3D + ",isUser=" + isUser + ",isUserDisable=" + isUserDisable + ",can=" + can);
		if (mDevice != null && !isEnable3D && can) {
			startEnable3DTime = System.currentTimeMillis();
			int retcode = mDevice.enable3D(true);
			log.debug("retcode=" + retcode);
			if (retcode == ERRORCODE_OPEN_FLASHLIGHT) {
				if (!isShowToast) {
					Util.showToast(getContext(), getContext().getString(R.string.enable3d_fail_the_flashlight_isopen), Toast.LENGTH_SHORT);
					isShowToast = true;
				}
			} else {
				isEnable3D = true;
				isUserDisable = isUser;
				if (mOn3DStateChangeListener != null) {
					mOn3DStateChangeListener.onChange(isEnable3D);
				}
			}
			ReportActionSender.getInstance().enable3D(1, 2, fromUser, retcode, System.currentTimeMillis());// 当前只有应用有2D/3D切换功能
			return;
		}
		ReportActionSender.getInstance().enable3D(1, 2, fromUser, 0, System.currentTimeMillis());
	}

	private long startEnable3DTime = 0;

	public long getEnable3DCoastTime() {
		if (isEnable3D) {
			return (System.currentTimeMillis() - startEnable3DTime);
		}
		return 0;
	}

	public boolean isEnable3D() {
		return isEnable3D;
	}

	public void pause(boolean isdisable3d) {
		if (isdisable3d) {
			disable3D(false);
		}
		try {
			if (mMediaPlayer != null && mMediaPlayer.isPlaying()) {
				mMediaPlayer.pause();
			}
		} catch (IllegalStateException e) {
			e.printStackTrace();
		}
	}

	public void stopPlayback() {
		log.debug("stopPlayback");
		if (mMediaPlayer != null) {
			new Thread(new Runnable() {

				@Override
				public void run() {
					try {
						mMediaPlayer.stop();
						mMediaPlayer.release();
						mMediaPlayer = null;
					} catch (IllegalStateException e) {
						e.printStackTrace();
					}
				}
			}).start();
		}
	}

	private boolean isInPlaybackState() {
		try {
			return (mMediaPlayer != null && mMediaPlayer.isPlaying());
		} catch (IllegalStateException e) {
			e.printStackTrace();
		}
		return false;
	}

	public int getCurrentPosition() {
		if (isInPlaybackState()) {
			return mMediaPlayer.getCurrentPosition();
		}
		return 0;
	}

	public int getDuration() {
		log.debug("getDuration!!!");
		if (mMediaPlayer != null) {
			return mMediaPlayer.getDuration();
		}
		return 0;
	}

	public boolean isPlaying() {
		return isInPlaybackState();
	}

	public void seekTo(int pos) {
		try {
			mMediaPlayer.seekTo(pos);
		} catch (IllegalStateException e) {
			e.printStackTrace();
		}
	}

	public int getVideoWidth() {
		return mMediaPlayer.getVideoWidth();
	}

	public int getVideoHeight() {
		return mMediaPlayer.getVideoHeight();
	}

	public void stop() {
		log.debug("stop");
		new Thread(new Runnable() {

			@Override
			public void run() {
				try {
					mMediaPlayer.stop();
				} catch (IllegalStateException e) {
					e.printStackTrace();
				}
			}
		}).start();

	}

	interface OnPlayerCallBackListener extends OnBufferingUpdateListener, OnCompletionListener, OnErrorListener, OnInfoListener, OnPreparedListener, OnSeekCompleteListener, OnVideoSizeChangedListener {
	}

	private OnPlayerCallBackListener mOnPlayerCallBackListener;

	public void setOnPlayerCallBackListener(OnPlayerCallBackListener mOnPlayerCallBackListener) {
		this.mOnPlayerCallBackListener = mOnPlayerCallBackListener;
	}

	interface OnSurfaceCallBackListener {
		public void surfaceDestroyed(SurfaceHolder holder);

		public void surfaceCreated(SurfaceHolder holder);

		public void surfaceChanged(SurfaceHolder holder, int format, int width, int height);
	}

	private OnSurfaceCallBackListener mOnSurfaceCallBackListener;

	public void setOnSurfaceCallBacListener(OnSurfaceCallBackListener mOnSurfaceCallBacListener) {
		this.mOnSurfaceCallBackListener = mOnSurfaceCallBacListener;
	}

	interface On3DStateChangeListener {
		public void onChange(boolean isenable3d);
	}

}
