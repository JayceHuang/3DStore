/**
 * Copyright 2015 Bartosz Lipinski
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.runmit.sweedee.view.flippablestackview;

import android.content.Context;
import android.support.v4.view.PagerAdapter;
import android.util.AttributeSet;

/**
 * Created by Bartosz Lipinski
 * 31.01.15
 */
public class FlippableStackView extends OrientedViewPager {
    
    private static final float DEFAULT_TOP_STACKED_SCALE = 0.4f;
    private static final float DEFAULT_OVERLAP_FACTOR = 0.4f;
    
    public FlippableStackView(Context context) {
        super(context);
    }

    public FlippableStackView(Context context, AttributeSet attrs) {
        super(context, attrs);
    }

    public void initStack(int numberOfStacked) {
        setPageTransformer(false, new StackPageTransformer(numberOfStacked, DEFAULT_TOP_STACKED_SCALE, DEFAULT_OVERLAP_FACTOR));
        setOffscreenPageLimit(numberOfStacked + 1);
    }

    @Override
    public void setAdapter(PagerAdapter adapter) {
        super.setAdapter(adapter);
        setCurrentItem(adapter.getCount() - 1);
    }
}
