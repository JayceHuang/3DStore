package com.runmit.sweedee.action.pay;

import java.text.MessageFormat;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Date;
import java.util.List;
import java.util.TimeZone;

import android.annotation.SuppressLint;
import android.content.Context;
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
import com.runmit.sweedee.manager.data.EventCode.AccountEventCode;
import com.runmit.sweedee.model.VOPrice;
import com.runmit.sweedee.model.VOUserPurchase;
import com.runmit.sweedee.model.VOUserPurchase.PurchaseRecord;
import com.runmit.sweedee.view.EmptyView;
import com.runmit.sweedee.view.EmptyView.Status;

public class PurchaseRecordFragment extends Fragment implements StoreApplication.OnNetWorkChangeListener{

	private ListView mListView;

    private EmptyView mEmptyView;
    
    private View rly_back;
    
    private View mRootView;
    
    private PurchaseRecordAdapter mAdapter;

    private List<PurchaseRecord> mList = new ArrayList<PurchaseRecord>();
    
    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
    	mRootView = inflater.inflate(R.layout.activity_purchase_record, container, false);
    	
         mEmptyView = (EmptyView) mRootView.findViewById(R.id.rl_empty_tip);
         mListView = (ListView) mRootView.findViewById(R.id.listview);
         mAdapter = new PurchaseRecordAdapter(getActivity());
         mListView.setAdapter(mAdapter);
         mEmptyView.setEmptyTip(R.string.no_purchase_record_tip).setEmptyPic(R.drawable.ic_paypal_history).setStatus(Status.Loading);
         AccountServiceManager.getInstance().getUserPurchases(1, 100, mCallBallHandler);
         
    	return mRootView;
    }

    @SuppressLint("HandlerLeak")
    Handler mCallBallHandler = new Handler() {
        @Override
        public void handleMessage(Message msg) {
            if (msg.what == AccountEventCode.EVENT_GET_USER_PURCHASES) {
                VOUserPurchase vo = null;
                if (msg.arg1 == 0) {
                    vo = (VOUserPurchase) msg.obj;
                }
                onLoadComplete(vo, msg.arg1);
            }
        }
    };

    private void onLoadComplete(VOUserPurchase vo, int ret) {
        if (vo != null && vo.data != null) {
            mList.clear();
            for(PurchaseRecord mPurchaseRecord : vo.data){
				if(Float.parseFloat(mPurchaseRecord.amount) > 0){
					mList.add(mPurchaseRecord);
				}
			}
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
        public PurchaseRecord getItem(int position) {
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
                convertView = mInflater.inflate(R.layout.purchase_record_item, null);
                holder = new ViewHolder();
                holder.tv_date = (TextView) convertView.findViewById(R.id.tv_date);
                holder.tv_name = (TextView) convertView.findViewById(R.id.tv_name);
                holder.tv_paytype = (TextView) convertView.findViewById(R.id.tv_paytype);
                holder.tv_amount = (TextView) convertView.findViewById(R.id.tv_amount);
                convertView.setTag(holder);
            } else {
                holder = (ViewHolder) convertView.getTag();
            }
            PurchaseRecord record = getItem(position);
            holder.tv_date.setText(getDate(record.createTime));
            holder.tv_name.setText(record.productName);
           
            if(record.currencyId == VOPrice.VIRTUAL_CURRENCYID){//虚拟币
            	holder.tv_amount.setText(MessageFormat.format(getString(R.string.account_balance), record.amount));
            	holder.tv_paytype.setText("");//设置为空，不用visible
            }else{
            	float value = Float.valueOf(record.amount) / 100;
            	if(value == 0.0f){
            		holder.tv_amount.setText("");
                	holder.tv_paytype.setText("");
            	}else{
            		holder.tv_amount.setText(record.symbol + " " + value);
                	holder.tv_paytype.setText("(" + record.paymentName + ")");
            	}
            }
            return convertView;
        }

        class ViewHolder {

            public TextView tv_date;

            public TextView tv_name;

            public TextView tv_paytype;

            public TextView tv_amount;
        }

        private String getDate(String milms) {
            String ret = "";
            try {
                long time = Long.parseLong(milms);
                ret = dateFomatter.format(new Date(time));
            } catch (Exception e) {
            }
            return ret;
        }
    }
}
