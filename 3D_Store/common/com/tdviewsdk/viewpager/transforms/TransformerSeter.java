/**
 * 
 */
package com.tdviewsdk.viewpager.transforms;

import java.util.ArrayList;
import android.app.Activity;
import android.app.Dialog;
import android.support.v4.view.ViewPager.PageTransformer;
import android.view.View;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemClickListener;
import android.widget.ArrayAdapter;
import android.widget.ListView;

/**
 * @author Sven.Zhan
 * ViewPager 切换动画的设置器
 */
public class TransformerSeter {	
	
	private static final ArrayList<TransformerItem> TRANSFORM_CLASSES;

	static {
		TRANSFORM_CLASSES = new ArrayList<TransformerItem>();
		TRANSFORM_CLASSES.add(new TransformerItem(DefaultTransformer.class));
		TRANSFORM_CLASSES.add(new TransformerItem(AccordionTransformer.class));
		TRANSFORM_CLASSES.add(new TransformerItem(BackgroundToForegroundTransformer.class));
		TRANSFORM_CLASSES.add(new TransformerItem(CubeInTransformer.class));
		TRANSFORM_CLASSES.add(new TransformerItem(CubeOutTransformer.class));
		TRANSFORM_CLASSES.add(new TransformerItem(DepthPageTransformer.class));
		TRANSFORM_CLASSES.add(new TransformerItem(FlipHorizontalTransformer.class));
		TRANSFORM_CLASSES.add(new TransformerItem(FlipVerticalTransformer.class));
		TRANSFORM_CLASSES.add(new TransformerItem(ForegroundToBackgroundTransformer.class));
		TRANSFORM_CLASSES.add(new TransformerItem(RotateDownTransformer.class));
		TRANSFORM_CLASSES.add(new TransformerItem(RotateUpTransformer.class));
		TRANSFORM_CLASSES.add(new TransformerItem(StackTransformer.class));
		TRANSFORM_CLASSES.add(new TransformerItem(TabletTransformer.class));
		TRANSFORM_CLASSES.add(new TransformerItem(ZoomInTransformer.class));
		TRANSFORM_CLASSES.add(new TransformerItem(ZoomOutSlideTransformer.class));
		TRANSFORM_CLASSES.add(new TransformerItem(ZoomOutTranformer.class));
	}
	
	public void showTransformers(Activity activity,final AnimationSetListener listener){
	    final Dialog dialog = new Dialog(activity);
        dialog.setTitle("主Tab切换动画设置");
        ListView listView = new ListView(activity);
        final ArrayAdapter<TransformerItem> adapter = new ArrayAdapter<TransformerItem>(activity, android.R.layout.simple_list_item_1, android.R.id.text1, TRANSFORM_CLASSES);
        listView.setAdapter(adapter);
        listView.setOnItemClickListener(new OnItemClickListener() {
			@Override
			public void onItemClick(AdapterView<?> parent, View view, int position, long id) {
				TransformerItem item = adapter.getItem(position);
				if(listener != null){
					try {
						listener.onPagerAnimationSet(item.clazz.newInstance());
					} catch (InstantiationException e) {
						e.printStackTrace();
					} catch (IllegalAccessException e) {
						e.printStackTrace();
					}
				}
				dialog.dismiss();
			}        	
		});
        dialog.setContentView(listView);
        dialog.show();
	}
	
	
	private static final class TransformerItem {
		
		final String title;
		final Class<? extends PageTransformer> clazz;

		public TransformerItem(Class<? extends PageTransformer> clazz) {
			this.clazz = clazz;
			title = clazz.getSimpleName().replace("Transformer", "");
		}

		@Override
		public String toString() {
			return title;
		}
	}
	
	public interface AnimationSetListener{
		
		public void onPagerAnimationSet(PageTransformer pa);
		
	}
	
}
