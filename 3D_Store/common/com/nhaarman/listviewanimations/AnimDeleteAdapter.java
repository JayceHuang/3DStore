package com.nhaarman.listviewanimations;

import java.util.ArrayList;
import java.util.Collection;
import java.util.List;
import java.util.Map;
import java.util.TreeMap;

import android.animation.Animator;
import android.animation.AnimatorListenerAdapter;
import android.animation.AnimatorSet;
import android.animation.ObjectAnimator;
import android.animation.PropertyValuesHolder;
import android.util.Log;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseAdapter;

/** 
 * @author Sven.Zhan
 * GridView 删除动画的Adapter
 *
 */
public abstract class AnimDeleteAdapter extends BaseAdapter{	
	
	String TAG = AnimDeleteAdapter.class.getSimpleName();
	
	private Map<Integer,View> mViewsMap = new TreeMap<Integer,View>();
	
	private Map<Integer,View> mPreiousViewMap = new TreeMap<Integer,View>();
	
	private int notifyTimes = 0;
	
	/**父GridView的宽度*/
	private int mParentWidth = 0;
	
	private final int ANIM_DURATION = 400;
	
	private final int ANIM_DELAY = 200;
	
	/**记录下GridView中删除的position*/
	private List<Integer> mDelPositionList = new ArrayList<Integer>();
	
	private void addView(View view) {
		AninViewHolder holder = (AninViewHolder) view.getTag();
		mViewsMap.put(holder.position, view);
	}
	
	
	/**
	 * 缩放删除动画
	 * @param listener 删除结束的回调监听器
	 */
	public void animScaleDelete(final AnimDeleteListener listener) {
		AnimatorSet animSet = new AnimatorSet();		
		List<Animator> objAnimList = new ArrayList<Animator>();
		PropertyValuesHolder pvAlpha = PropertyValuesHolder.ofFloat("alpha", 1f, 0f);
		PropertyValuesHolder pvScaleX = PropertyValuesHolder.ofFloat("scaleX", 1f, 0f);
		PropertyValuesHolder pvScaleY = PropertyValuesHolder.ofFloat("scaleY", 1f, 0f);		
		final Collection<View> viewList = mViewsMap.values();	
		final List<View> deleteList = new ArrayList<View>();
		for (View view : viewList) {
			AninViewHolder holder = (AninViewHolder) view.getTag();
			if(holder.isSelected){
				view.setPivotX(view.getWidth());
				view.setPivotY(0);
				mDelPositionList.add(holder.position);
				ObjectAnimator objAnim = ObjectAnimator.ofPropertyValuesHolder(view, pvAlpha, pvScaleX, pvScaleY);
				objAnimList.add(objAnim);
				deleteList.add(view);
			}
		}		
		animSet.playTogether(objAnimList);
		animSet.setDuration(ANIM_DURATION);
		animSet.addListener(new AnimatorListenerAdapter(){
			@Override
			public void onAnimationEnd(Animator animation) {
				mPreiousViewMap.clear();
				mPreiousViewMap.putAll(mViewsMap);
				mViewsMap.clear();				
				if(listener != null){
					listener.onAnimDeleteEnd();
				}
			}			
		});
		animSet.start();
	}
	
	protected void restorView(int position){
		View view = mPreiousViewMap.get(position);
		if(view != null){
//			ObjectAnimator objAnim = ObjectAnimator.ofFloat(view, "alpha", 1f);
//			objAnim.setDuration(ANIM_DURATION);
//			objAnim.setStartDelay(ANIM_DELAY);
//			objAnim.start();
//			Log.d(TAG, "pivot = ["+view.getPivotX() + " , "+view.getPivotY()+"] HW = ["+view.getWidth()+" , "+view.getHeight()+"]");
			view.setAlpha(1f);
			view.setScaleX(1f);
			view.setScaleY(1f);
			view.setPivotX(view.getWidth() / 2);
			view.setPivotY(view.getHeight() / 2);
		}
	}
	
	/**
	 * GridView item项删除时，其他项补齐的动画（实际上删除项的View并没有在GridView中删除，我们要在这个方法中对它进行属性还原）
	 * @param position
	 */
	protected void restorViewByMove(int position){
		
		int len = mDelPositionList.size();
		
		/*第一个删除项前面的View无需还原*/
		if(mDelPositionList.isEmpty() || position < mDelPositionList.get(0)){
			return;
		}		
		final View view = mPreiousViewMap.get(position);
		
		/**取得所有View的postion*/
		int lens = mPreiousViewMap.keySet().size();
		Integer[] positions = new Integer[lens];
		mPreiousViewMap.keySet().toArray(positions);
		
		//Log.d(TAG, "positions  = "+Arrays.toString(positions));
		final AninViewHolder holder = (AninViewHolder) view.getTag();	
		
		int steps = 0;//这个位置的View需要从下个steps位置的View移动过来		
		
		if(holder.isSelected){//他后面还有几个需要删除的就是他需要移动的步数
			for(int i = position;i >= mDelPositionList.get(0) && i <= positions[lens-1]; i++){
				AninViewHolder temp = (AninViewHolder) mPreiousViewMap.get(i).getTag();
				if(temp.isSelected){
					steps++;
				}
			}	
		}else{//他前面还有几个需要删除的就是他需要移动的步数
			steps++;
			for(int i = 0;i > mDelPositionList.get(0) && i <= position; i++){				
				AninViewHolder temp = (AninViewHolder) mPreiousViewMap.get(i).getTag();
				if(temp.isSelected){
					steps++;
				}
			}	
		}	
		View moveTo = mPreiousViewMap.get(position + steps);
		
		if(holder.isAniming || moveTo == null){
			Log.w(TAG, "restorView position = "+position+" delListSize = "+len+" steps = "+steps);
			return;
		}
		
		holder.isAniming = true;
		float moveX = moveTo.getX()- view.getX();
		final float moveY = moveTo.getY() - view.getY();
		int width = view.getWidth();
		Log.d(TAG, "restorView position = "+position+" delListSize = "+len+" steps = "+steps+" moveX = "+moveX+" moveY = "+moveY);		
		AnimatorSet animSet  = new AnimatorSet();
		if(moveY != 0f){
			float moveX1 = moveX - width - moveTo.getX();
			float moveX12 = (mParentWidth - view.getX());
			ObjectAnimator otranslateX1 = ObjectAnimator.ofFloat(view, "translationX", moveX,moveX1);
			ObjectAnimator otranslateX2 = ObjectAnimator.ofFloat(view, "translationX", moveX12,0);
			otranslateX1.addListener(new AnimatorListenerAdapter(){
				@Override
				public void onAnimationStart(Animator animation) {
					view.setTranslationY(moveY);
				}
			});
			otranslateX2.addListener(new AnimatorListenerAdapter(){
				@Override
				public void onAnimationStart(Animator animation) {
					view.setTranslationY(0);
				}
			});			
			animSet.playSequentially(otranslateX1,otranslateX2);			
		}else{
			ObjectAnimator otranslateX1 = ObjectAnimator.ofFloat(view, "translationX", moveX,0);
			animSet.play(otranslateX1);
		}	
		
		/**都移动完了，需要清除*/
		if(position == positions[lens-len-1]){
			mDelPositionList.clear();
		}
		
		animSet.addListener(new AnimatorListenerAdapter(){
			@Override
			public void onAnimationStart(Animator animation) {
				view.setAlpha(1f);
				view.setScaleX(1f);
				view.setScaleY(1f);
			}

			@Override
			public void onAnimationEnd(Animator animation) {
				holder.isAniming = false;
			}
			
		});
		animSet.setDuration(ANIM_DURATION);
		animSet.start();
	}
	
	/**
	 * @param listener
	 * 移动ItemView
	 * @deprecated
	 */
	private void moveViews(final AnimDeleteListener listener){
		int deleteNum = 0;//删除的个数
		final Collection<View> viewList = mViewsMap.values();
		final List<View> deleteList = new ArrayList<View>();
		List<Animator> objAnimList = new ArrayList<Animator>();
		for (View view : viewList) {
			int moveStep = 0;//每个View需要移动的格数
			AninViewHolder holder = (AninViewHolder)view.getTag();
			int position = holder.position;
			boolean isSelect = holder.isSelected;
			if(isSelect){
				deleteList.add(view);
				deleteNum++;
			}else{
				moveStep = deleteNum;
				View moveTo = mViewsMap.get(position - moveStep);
				PropertyValuesHolder translateX = PropertyValuesHolder.ofFloat("translationX", 0,moveTo.getX()-view.getX());
				PropertyValuesHolder translateY = PropertyValuesHolder.ofFloat("translationY", 0,moveTo.getY()-view.getY());
				ObjectAnimator objAnim = ObjectAnimator.ofPropertyValuesHolder(view, translateX,translateY);
				objAnimList.add(objAnim);
			}
		}
		AnimatorSet animSet = new AnimatorSet();
		animSet.playTogether(objAnimList);
		animSet.setDuration(ANIM_DURATION);		
		animSet.start();
	}
	
	@Override
	public View getView(int position, View convertView, ViewGroup parent) {		
		if(mParentWidth <= 0) mParentWidth = parent.getWidth();
		convertView = initConverView(position, convertView, parent);
		AninViewHolder holder = (AninViewHolder) convertView.getTag();
		holder.position = position;
		
		/**此处选择一种恢复View的方式*/
//		restorViewByMove(position);
		restorView(position);
		
		showHolder(position,holder);
		addView(convertView);	
		return convertView;
	}	

	@Override
	public void notifyDataSetChanged() {
		Log.d(TAG, "notifyDataSetChanged");
		notifyTimes++;
		super.notifyDataSetChanged();
	}
	
	public boolean hasNotifiedOnce(){
		return notifyTimes > 1;
	}
	
	abstract protected View initConverView(int position, View convertView, ViewGroup parent);
	
	abstract protected void showHolder(int position,AninViewHolder holder);

	public class AninViewHolder{
		
		public int position;	
		
		public boolean isSelected;
		
		public boolean isAniming = false;
	}
	
	public interface AnimDeleteListener {
		public void onAnimDeleteEnd();
	}
}


