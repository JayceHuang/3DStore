package com.runmit.sweedee.view.animation;

import android.view.animation.Interpolator;

public class BounceInterpolator implements Interpolator {

	float x0 = 0.6f;
	float y0 = 1.9f;

	float x1 = 0.8f;
	float y1 = 0.5f;

	float x2 = 0.9f;
	float y2 = 1.4f;

	float x3 = 1.0f;// end
	float y3 = 1.0f;

	float a1 = 0;
	float a2 = 0;
	float a3 = 0;

	public BounceInterpolator() {
		a1 = (y0 - y1) / ((x0 - x1) * (x0 - x1));
		a2 = (y1 - y2) / ((x1 - x2) * (x1 - x2));
		a3 = (y2 - y3) / ((x2 - x3) * (x2 - x3));
	}

	public float getInterpolation(float t) {
		float y = 0;
		if (t <= x0) {
			y = y0 / (x0 * x0) * (x0 * x0 - (t - x0) * (t - x0));
		} else if (t <= x1) {
			y = a1 * (t - x1) * (t - x1) + y1;
		} else if (t <= x2) {
			y = a2 * (t - x2) * (t - x2) + y2;
		} else {
			y = a3 * (t - x3) * (t - x3) + y3;
		}
		return y;
	}
}
