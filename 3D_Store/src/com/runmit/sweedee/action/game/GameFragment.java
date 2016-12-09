package com.runmit.sweedee.action.game;

import java.text.MessageFormat;
import java.util.Arrays;
import java.util.HashSet;
import java.util.List;

import android.app.AlertDialog;
import android.content.DialogInterface;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.graphics.Color;
import android.os.Bundle;
import android.support.v4.app.FragmentManager;
import android.support.v4.app.FragmentStatePagerAdapter;
import android.support.v4.view.ViewPager;
import android.support.v4.view.ViewPager.OnPageChangeListener;
import android.util.DisplayMetrics;
import android.util.TypedValue;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AbsListView;
import android.widget.BaseAdapter;
import android.widget.CheckBox;
import android.widget.CompoundButton;
import android.widget.CompoundButton.OnCheckedChangeListener;
import android.widget.ListView;
import android.widget.TextView;

import com.runmit.sweedee.BaseFragment;
import com.runmit.sweedee.R;
import com.runmit.sweedee.manager.Game3DConfigManager;
import com.runmit.sweedee.util.Util;
import com.runmit.sweedee.util.XL_Log;
import com.runmit.sweedee.view.PagerSlidingTabStrip;
import com.runmit.sweedee.view.TriangleLineView;
import com.tdviewsdk.viewpager.transforms.DepthPageTransformer;

public class GameFragment extends BaseFragment {

    private XL_Log log = new XL_Log(GameFragment.class);

    private PagerSlidingTabStrip mTabs;
    private ViewPager mViewPager;
    private MyAdapter mMyAdapter;
    private View mRootView;
    
    public GameFragment() {
    }
    
    public AbsListView getCurrentListView(){
    	if(mMyAdapter != null && mViewPager != null){
    		BaseFragment f = mMyAdapter.getItem(mViewPager.getCurrentItem());
    		if(f != null){
    			return f.getCurrentListView();
    		}
    	}
    	return null;
    }

    @Override
	public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
		if(mRootView != null){
			return mRootView;
		}
		mRootView = inflater.inflate(R.layout.frg_home_page_game, container, false);
		
        mTabs = (PagerSlidingTabStrip) mRootView.findViewById(R.id.game_detail_tabs);
        mViewPager = (ViewPager) mRootView.findViewById(R.id.detail_viewPager);
        mViewPager.setPageTransformer(true, new DepthPageTransformer());
        DisplayMetrics dm = getResources().getDisplayMetrics();
        mMyAdapter = new MyAdapter(getChildFragmentManager());
        mViewPager.setAdapter(mMyAdapter);
        mTabs.setViewPager(mViewPager);
        mTabs.setIndicatorColorResource(R.color.transparent);
        mTabs.setSelectedTextColorResource(R.color.thin_blue);
        mTabs.setDividerColor(Color.TRANSPARENT);
        mTabs.setUnderlineHeight((int) TypedValue.applyDimension(TypedValue.COMPLEX_UNIT_DIP, 1, dm));
        mTabs.setIndicatorHeight((int) TypedValue.applyDimension(TypedValue.COMPLEX_UNIT_DIP, 3, dm));
        mTabs.setTextSize((int) TypedValue.applyDimension(TypedValue.COMPLEX_UNIT_SP, 17, dm));
        mTabs.setTextColor(getResources().getColor(R.color.white_text_color));
        
        TriangleLineView mTriangleLineView = (TriangleLineView) mRootView.findViewById(R.id.triangleline);
		mTriangleLineView.initTriangle(2);
		
        mTabs.setOnPageChangeListener(mMyAdapter);
        return mRootView;
    }

	@Override
	public void onDestroyView() {
		super.onDestroyView();
        ViewGroup parent = (ViewGroup) mRootView.getParent();
        if (parent != null) {
            parent.removeView(mRootView);
        }
	}
	
    public void onViewPagerResume() {    	
        super.onViewPagerResume();
        if(mMyAdapter!=null){
        	mMyAdapter.getItem(mViewPager.getCurrentItem()).onViewPagerResume();
        }
        
        boolean hasConfigUpdate = Game3DConfigManager.getInstance().hasConfigUpdate();
        log.debug("hasConfigUpdate = "+hasConfigUpdate);
        if(hasConfigUpdate){
        	showConfigUpdateDialog();
        }
    }

    @Override
    public void onViewPagerStop() {    	
        super.onViewPagerStop();
        if(mMyAdapter!=null){
        	mMyAdapter.getItem(mViewPager.getCurrentItem()).onViewPagerStop();
        }
    }
    
    private void showConfigUpdateDialog() {
        AlertDialog.Builder builder = new AlertDialog.Builder(getActivity(),R.style.AlertDialog);
        int convertNum = Game3DConfigManager.getInstance().getConverNum();
        String tip = MessageFormat.format(getString(R.string.find_gameconfig_tip), convertNum);
        builder.setTitle(Util.setColourText(getActivity(), R.string.confirm_title, Util.DialogTextColor.BLUE));
        //builder.setMessage(Util.setColourText(tip, Util.DialogTextColor.BLUE));
        View contentView = LayoutInflater.from(getActivity()).inflate(R.layout.game_update_view, null);
        TextView textView = (TextView) contentView.findViewById(R.id.tv_message);
        textView.setText(tip);
        
        final GameUpdateAdapter adapter = new GameUpdateAdapter(Game3DConfigManager.getInstance().getUpdatePackage());
        
        ListView listView = (ListView) contentView.findViewById(R.id.listView);
        listView.setAdapter(adapter);
        
        builder.setView(contentView);
        
        builder.setPositiveButton(R.string.cancel,
                new android.content.DialogInterface.OnClickListener() {
                    @Override
                    public void onClick(DialogInterface dialog, int which) {
                        dialog.dismiss();
                        Game3DConfigManager.getInstance().refuseUpdate();
                    }
                }
        );
        builder.setNegativeButton(Util.setColourText(getActivity(), R.string.gameconfig_update, Util.DialogTextColor.BLUE),
                new android.content.DialogInterface.OnClickListener() {
                    @Override
                    public void onClick(DialogInterface dialog, int which) {
                        dialog.dismiss();  
                        Game3DConfigManager.getInstance().downloadConfigFiles(adapter.getSelectedList());
                      
                    }
                }
        );
        builder.setCancelable(false);
        builder.show();       
    }
    
    class GameUpdateAdapter extends BaseAdapter{

    	List<PackageInfo> mList;
    	LayoutInflater mInflater;
    	PackageManager mPackageManager;
    	
    	HashSet<PackageInfo> mHashSet = new HashSet<PackageInfo>();
    	
    	public GameUpdateAdapter(List<PackageInfo> dataList){
    		mList = dataList;
    		mInflater = LayoutInflater.from(getActivity());
    		mPackageManager = getActivity().getPackageManager();
    		mHashSet.addAll(mList);
    	}
    	
		@Override
		public int getCount() {			
			return mList.size();
		}

		@Override
		public PackageInfo getItem(int position) {			
			return mList.get(position);
		}

		@Override
		public long getItemId(int position) {			
			return position;
		}

		public List<PackageInfo> getSelectedList(){
			PackageInfo[] arrays = new PackageInfo[mHashSet.size()];
			mHashSet.toArray(arrays);
			return Arrays.asList(arrays);
		}
		
		
		@Override
		public View getView(int position, View convertView, ViewGroup parent) {
			Holder holder = null;
			if (convertView == null) {
				holder = new Holder();
				convertView = mInflater.inflate(R.layout.game_update_listview_item, parent, false);
				holder.textView = (TextView) convertView.findViewById(R.id.textView);				
				holder.checkBox = (CheckBox) convertView.findViewById(R.id.checkBox);
				convertView.setTag(holder);
			} else {
				holder = (Holder) convertView.getTag();
			}
			final PackageInfo packge = getItem(position);
			holder.textView.setText(packge.applicationInfo.loadLabel(mPackageManager));	
			holder.checkBox.setOnCheckedChangeListener(new OnCheckedChangeListener() {				
				@Override
				public void onCheckedChanged(CompoundButton buttonView, boolean isChecked) {
					if(isChecked){
						mHashSet.add(packge);
					}else{
						mHashSet.remove(packge);
					}
				}
			});
			return convertView;
		}
    	
		class Holder {
			public TextView textView;
			public CheckBox checkBox;
		}
    }
    
    
    private SiftGameFragment mGamercmmdFragment = null;
    private AllGameFragment mGameAllFragment = null;

    class MyAdapter extends FragmentStatePagerAdapter implements OnPageChangeListener{
        private final String[] TITLES = {getString(R.string.choiceness), getString(R.string.all_game)};

        @Override
        public int getCount() {
            return TITLES.length;
        }

        @Override
        public CharSequence getPageTitle(int position) {
            return TITLES[position];
        }

        public MyAdapter(FragmentManager fm) {
            super(fm);
        }

        @Override
        public BaseFragment getItem(int position) {
            log.debug("choose item = " + position);
            switch (position) {
                case 0:
                    if (mGamercmmdFragment == null) {
                    	mGamercmmdFragment = new SiftGameFragment();
                    }
                    return mGamercmmdFragment;
                case 1:
                    if (mGameAllFragment == null) {
                    	mGameAllFragment = new AllGameFragment( );
                    }
                    return mGameAllFragment;
            }
            return null;
        }

		@Override
		public void onPageScrollStateChanged(int arg0) {
			// TODO Auto-generated method stub
			
		}

		@Override
		public void onPageScrolled(int arg0, float arg1, int arg2) {
			// TODO Auto-generated method stub
			
		}

        private int mLastSelectPager = 0;
        
		@Override
		public void onPageSelected(int position) {
			if(mLastSelectPager>=0){
				mMyAdapter.getItem(mLastSelectPager).onViewPagerStop();
			}
			mLastSelectPager = position;
			mMyAdapter.getItem(mLastSelectPager).onViewPagerResume();
		}
    }
}
