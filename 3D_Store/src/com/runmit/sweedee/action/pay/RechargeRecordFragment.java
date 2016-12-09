package com.runmit.sweedee.action.pay;

import java.text.MessageFormat;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Date;
import java.util.List;
import java.util.TimeZone;

import android.annotation.SuppressLint;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.support.v4.app.Fragment;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.ListView;
import android.widget.TextView;

import com.runmit.sweedee.R;
import com.runmit.sweedee.StoreApplication;
import com.runmit.sweedee.manager.AccountServiceManager;
import com.runmit.sweedee.manager.WalletManager;
import com.runmit.sweedee.manager.data.EventCode.AccountEventCode;
import com.runmit.sweedee.model.RechargeHistory;
import com.runmit.sweedee.model.RechargeHistory.Recharge;
import com.runmit.sweedee.model.VOPrice;
import com.runmit.sweedee.view.EmptyView;
import com.runmit.sweedee.view.EmptyView.Status;

public class RechargeRecordFragment extends Fragment implements StoreApplication.OnNetWorkChangeListener {

	private ListView mListView;

	private EmptyView mEmptyView;

	private View mRootView;

	private PurchaseRecordAdapter mAdapter;

	private List<Recharge> mList = new ArrayList<Recharge>();
	
	private MyReceiver mMyReceiver;

	@Override
	public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
		mRootView = inflater.inflate(R.layout.activity_purchase_record, container, false);

		mMyReceiver = new MyReceiver();
		IntentFilter intentFilter = new IntentFilter(WalletManager.INTENT_ACTION_USER_WALLET_CHANGE);
		getActivity().registerReceiver(mMyReceiver, intentFilter);
		
		mEmptyView = (EmptyView) mRootView.findViewById(R.id.rl_empty_tip);
		mListView = (ListView) mRootView.findViewById(R.id.listview);
		mAdapter = new PurchaseRecordAdapter(getActivity());
		mListView.setAdapter(mAdapter);
		mEmptyView.setEmptyTip(R.string.no_recharge_record_tip).setEmptyPic(R.drawable.ic_paypal_history).setStatus(Status.Loading);
		initData();
		return mRootView;
	}

	@Override
	public void onDestroy() {
		super.onDestroy();
		getActivity().unregisterReceiver(mMyReceiver);
	}
	
	public class MyReceiver extends BroadcastReceiver{

		@Override
		public void onReceive(Context context, Intent intent) {
			if(intent.getAction().equals(WalletManager.INTENT_ACTION_USER_WALLET_CHANGE)){
				mList.clear();
				mAdapter.notifyDataSetChanged();
				mEmptyView.setStatus(Status.Loading);
				initData();
			}
		}
	}
	
	private void initData(){
		int status = 2;
		int page = 1;
		int count = 1000;
		AccountServiceManager.getInstance().getRechargeRecord(status, page, count, mCallBallHandler);
	}
	
	@SuppressLint("HandlerLeak")
	private Handler mCallBallHandler = new Handler() {
		@Override
		public void handleMessage(Message msg) {
			if (msg.what == AccountEventCode.EVENT_GET_RECHARGE_RECORD) {
				RechargeHistory mRechargeHistory = null;
				if (msg.arg1 == 0) {
					mRechargeHistory = (RechargeHistory) msg.obj;
				}
				onLoadComplete(mRechargeHistory, msg.arg1);
			}
		}
	};

	private void onLoadComplete(RechargeHistory mRechargeHistory, int ret) {
		if (mRechargeHistory != null && mRechargeHistory.data != null) {
			mList.clear();
			mList.addAll(mRechargeHistory.data);
		}
		if (mList.size() > 0) {
			mListView.setVisibility(View.VISIBLE);
			mEmptyView.setStatus(Status.Gone);
		} else {
			mEmptyView.setStatus(Status.Empty);
			mListView.setVisibility(View.GONE);
		}
		mAdapter.notifyDataSetChanged();
	}

	@Override
	public void onChange(boolean isNetWorkAviliable, NetType mNetType) {
	}

	class PurchaseRecordAdapter extends BaseAdapter {

		LayoutInflater mInflater;

		SimpleDateFormat dateFomatter;

		@SuppressLint("SimpleDateFormat")
		public PurchaseRecordAdapter(Context ctx) {
			mInflater = LayoutInflater.from(ctx);
			dateFomatter = new SimpleDateFormat("yyyy-MM-dd HH:mm");
			dateFomatter.setTimeZone(TimeZone.getDefault());
		}

		@Override
		public int getCount() {
			return mList.size();
		}

		@Override
		public Recharge getItem(int position) {
			return mList.get(position);
		}

		@Override
		public long getItemId(int position) {
			return position;
		}

		@Override
		public View getView(int position, View convertView, ViewGroup parent) {
			ViewHolder holder = null;
			if (convertView == null) {
				convertView = mInflater.inflate(R.layout.recharge_record_item, parent,false);
				holder = new ViewHolder();
				holder.tv_date = (TextView) convertView.findViewById(R.id.tv_date);
				holder.tv_amount = (TextView) convertView.findViewById(R.id.tv_amount);
				convertView.setTag(holder);
			} else {
				holder = (ViewHolder) convertView.getTag();
			}
			Recharge record = getItem(position);
			holder.tv_date.setText(dateFomatter.format(new Date(record.createTime)));
			if(record.currencyId == VOPrice.VIRTUAL_CURRENCYID){//虚拟币
				holder.tv_amount.setText(MessageFormat.format(getString(R.string.present_money), record.amount));
			}else{
				holder.tv_amount.setText(record.symbol + " " + Float.valueOf(record.amount) / 100);
			}
			return convertView;
		}

		class ViewHolder {
			public TextView tv_date;
			public TextView tv_amount;
		}
	}
}
