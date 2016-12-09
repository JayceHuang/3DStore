package com.runmit.sweedee.action.login;

import java.util.List;

import android.content.Context;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.CheckedTextView;

import com.runmit.sweedee.R;
import com.runmit.sweedee.model.OpenCountryCode.CountryInfo;

public class MySimpleArrayAdapter extends BaseAdapter {
	private final Context context;
	private final List<CountryInfo> countryList;
	LayoutInflater inflater;

	public MySimpleArrayAdapter(Context context, List<CountryInfo> values) {
		this.context = context;
		this.countryList = values;
		inflater = LayoutInflater.from(context);
	}

	@Override
	public View getView(int position, View convertView, ViewGroup parent) {
		View rowView = inflater.inflate(R.layout.dialog_text, parent, false);
		CheckedTextView textView = (CheckedTextView) rowView.findViewById(android.R.id.text1);
		textView.setText(getItem(position).name);
		return rowView;
	}

	@Override
	public int getCount() {
		return countryList.size();
	}

	@Override
	public CountryInfo getItem(int position) {
		return countryList.get(position);
	}

	@Override
	public long getItemId(int position) {
		return position;
	}
}
