package com.runmit.sweedee.action.search;

import android.graphics.Bitmap;
import android.opengl.GLUtils;

import javax.microedition.khronos.opengles.GL10;

import static javax.microedition.khronos.opengles.GL10.*;

public class Texture {
	private int[] id = {0};

	private int width, height;
	private int contentWidth, contentHeight;

	private boolean destroyed = false;

	private Texture() {
	}

	public static Texture createTexture(Bitmap bitmap, GL10 gl) {
		Texture t = new Texture();

		int w = Integer.highestOneBit(bitmap.getWidth() - 1) << 1;
		int h = Integer.highestOneBit(bitmap.getHeight() - 1) << 1;

		t.contentWidth = bitmap.getWidth();
		t.contentHeight = bitmap.getHeight();
		t.width = w;
		t.height = h;

		gl.glGenTextures(1, t.id, 0);
		gl.glBindTexture(GL_TEXTURE_2D, t.id[0]);
		gl.glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		gl.glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		/*
		 gl.glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		 gl.glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		 */
		gl.glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, null);
		GLUtils.texSubImage2D(GL_TEXTURE_2D, 0, 0, 0, bitmap);

		return t;
	}

	public void destroy(GL10 gl) {
		if (id[0] != 0)
			gl.glDeleteTextures(1, id, 0);

		id[0] = 0;
	}

	public boolean isDestroyed() {
		return destroyed;
	}

	public int[] getId() {
		return id;
	}

	public int getWidth() {
		return width;
	}

	public int getHeight() {
		return height;
	}

	public int getContentWidth() {
		return contentWidth;
	}

	public int getContentHeight() {
		return contentHeight;
	}
}
