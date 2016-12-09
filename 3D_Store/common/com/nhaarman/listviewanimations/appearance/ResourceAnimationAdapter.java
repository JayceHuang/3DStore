/*
 * Copyright 2014 Niek Haarman
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
package com.nhaarman.listviewanimations.appearance;

import android.animation.Animator;
import android.animation.AnimatorInflater;
import android.annotation.SuppressLint;
import android.content.Context;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseAdapter;

/**
 * An implementation of AnimationAdapter which bases the animations on
 * resources.
 */
@SuppressLint("NewApi") 
public abstract class ResourceAnimationAdapter extends AnimationAdapter {

    
    private final Context mContext;

    @SuppressWarnings("UnusedDeclaration")
    protected ResourceAnimationAdapter(final BaseAdapter baseAdapter, final Context context) {
        super(baseAdapter);
        mContext = context;
    }

    
    @Override
    public Animator[] getAnimators(final ViewGroup parent, final View view) {
        return new Animator[]{AnimatorInflater.loadAnimator(mContext, getAnimationResourceId())};
    }

    /**
     * Get the resource id of the animation to apply to the views.
     */
    protected abstract int getAnimationResourceId();

}