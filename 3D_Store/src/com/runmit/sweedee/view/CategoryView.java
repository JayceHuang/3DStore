package com.runmit.sweedee.view;

import java.util.List;

import com.runmit.sweedee.R;
import com.runmit.sweedee.util.DensityUtil;
import com.unionpay.mobile.android.widgets.n;

import android.R.integer;
import android.content.Context;
import android.util.AttributeSet;
import android.view.Gravity;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.LinearLayout;
import android.widget.RadioButton;
import android.widget.RadioGroup;
import android.widget.RadioGroup.OnCheckedChangeListener;
import android.widget.TextView;

public class CategoryView extends LinearLayout implements
		OnCheckedChangeListener {

	private LayoutInflater inflater;
	public boolean isAddAll = true;
	private RadioGroup radioGroup;
	private View divier;
	public String label;
	public int backgroundResID;
	public int textSize;
	private DensityUtil densityUtil;
	public int labelMargin;
	public int radioMargin;

	public CategoryView(Context context) {
		this(context, null);
	}

	public CategoryView(Context context, AttributeSet attrs) {
		super(context, attrs);
		inflater = LayoutInflater.from(context);
		densityUtil=new DensityUtil(context);
		
	}

	public void add(List<String> list) {
		if (list.size() > 0) {
			ViewGroup view = (ViewGroup) inflater.inflate(R.layout.item_movie_catalogfilter,
					null);
			ViewGroup tabView=(ViewGroup) view.findViewById(R.id.item_moviecatalog_tabview);
			if(label!=null){
			TextView labeView=new TextView(view.getContext());
			LinearLayout.LayoutParams params = new RadioGroup.LayoutParams(
					RadioGroup.LayoutParams.WRAP_CONTENT,
					RadioGroup.LayoutParams.WRAP_CONTENT);
			params.leftMargin = 0;
			params.rightMargin =densityUtil.dip2px(labelMargin);
			labeView.setLayoutParams(params);
			labeView.setTextSize(textSize);
			labeView.setText(label+":");
			tabView.addView(labeView,0);
		}
		divier = view.findViewById(R.id.item_catalogfilter_divier);
		addView(view);
		radioGroup = (RadioGroup) view.findViewById(R.id.container);
		if (isAddAll) {
			RadioButton bt = newRadioButton("全部");
			bt.setTag(1);
			bt.setTextSize(textSize);
			radioGroup.addView(bt);
		}
		for (int i = 0; i < list.size(); i++) {
			RadioButton bt = newRadioButton(list.get(i));
			bt.setTextSize(textSize);
			if (isAddAll) {
				bt.setTag(i + 1);
			} else {
				bt.setTag(i);
			}
			radioGroup.addView(bt);
		}
			radioGroup.setOnCheckedChangeListener(this);
		}
	}

	private RadioButton newRadioButton(String text) {
		RadioButton button = new RadioButton(getContext());
		RadioGroup.LayoutParams params = new RadioGroup.LayoutParams(
				RadioGroup.LayoutParams.WRAP_CONTENT,
				RadioGroup.LayoutParams.WRAP_CONTENT);
		params.leftMargin = 0;
		params.rightMargin =densityUtil.dip2px(radioMargin);
		button.setLayoutParams(params);
		button.setButtonDrawable(android.R.color.transparent);
		button.setTextColor(getResources().getColorStateList(
				backgroundResID));
		button.setText(text);
		return button;
	}

	public void setDefaultChecked() {
		if (radioGroup.getChildAt(0) != null) {
			RadioButton button = (RadioButton) radioGroup.getChildAt(0);
			button.setSelected(true);
			radioGroup.check(button.getId());
		}
	}

	public void setTag(Object object) {
		radioGroup.setTag(object);
	}
	
	public void setDivierVisible(int visiable) {
		divier.setVisibility(visiable);
	}

	@Override
	public void onCheckedChanged(RadioGroup group, int checkedId) {
		if (mListener != null) {
			mListener.click(group, checkedId);
		}
	}

	public void setOnClickCategoryListener(OnClickCategoryListener l) {
		mListener = l;
	}

	private OnClickCategoryListener mListener;

	public interface OnClickCategoryListener {
		public void click(RadioGroup group, int checkedId);
	}
}