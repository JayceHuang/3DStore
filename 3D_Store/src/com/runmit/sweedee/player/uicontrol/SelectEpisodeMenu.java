//package com.runmit.sweedee.player.uicontrol;
//
//import java.util.List;
//
//import android.annotation.SuppressLint;
//import android.content.Context;
//import android.graphics.Color;
//import android.text.TextUtils;
//import android.util.DisplayMetrics;
//import android.util.Log;
//import android.view.Gravity;
//import android.view.LayoutInflater;
//import android.view.View;
//import android.view.View.OnClickListener;
//import android.view.ViewGroup;
//import android.widget.BaseAdapter;
//import android.widget.ListView;
//import android.widget.RelativeLayout;
//import android.widget.RelativeLayout.LayoutParams;
//import android.widget.TextView;
//
//import com.runmit.sweedee.R;
//import com.runmit.sweedee.player.data.PlayVideoInfo;
//import com.runmit.sweedee.player.data.PlayVideoInfoMgr;
//import com.runmit.sweedee.util.DensityUtil;
//import com.runmit.sweedee.util.Util;
//import com.runmit.sweedee.view.PopupWindow;
//import com.runmit.sweedee.view.PopupWindow.OnDismissListener;
//
//public class SelectEpisodeMenu {
//
//    private Context mContext;
//    private PopupWindow popupWindow;
//    private ListAdapter mListAdapter;
//    private LayoutInflater inflater;
//
//    private static int ROW_HEIGHT_CONTENT = 100;
//    private List<PlayVideoInfo> mList;
//    private OnPlaySelectListener mOnSelectPlayCallback;
//    private OnPlayControlListener mOnPlayControlCallback;
//    private int mType = 0;
//    private DensityUtil mDensityUtil;
//
//    public SelectEpisodeMenu(Context context, OnPlaySelectListener onselectCb, OnPlayControlListener onPlayControlListener) {
//        mContext = context;
//        mOnSelectPlayCallback = onselectCb;
//        this.mOnPlayControlCallback = onPlayControlListener;
//
//
//        mList = PlayVideoInfoMgr.getInstance().getmVideoInfos();
//        mType = PlayVideoInfoMgr.getInstance().getmType();
//        mDensityUtil = new DensityUtil(context);
//
//    }
//
//    public void showSelectEpisodeDialog(View viewParent) {
//        if (popupWindow != null && popupWindow.isShowing() && mListAdapter != null) {
//            mListAdapter.notifyDataSetChanged();
//            return;
//        }
//
//        inflater = (LayoutInflater) mContext.getSystemService(Context.LAYOUT_INFLATER_SERVICE);
//        View view = inflater.inflate(R.layout.layout_select_episode_menu, null);
//
//        ListView listView = (ListView) view.findViewById(R.id.lv_videos);
//        if (mListAdapter == null) {
//            mListAdapter = new ListAdapter();
//        }
//        listView.setAdapter(mListAdapter);
//        listView.setSelection(PlayVideoInfoMgr.getInstance().getmCurPlay());
//        DisplayMetrics dis = mContext.getResources().getDisplayMetrics();
//        ROW_HEIGHT_CONTENT = dis.heightPixels / 8;
//
//        int dialogHeight = ROW_HEIGHT_CONTENT * 6;    //统一调整为4行的高度
//
//        int dialogWidth = 0;
//        int distance = (int) mContext.getResources().getDimension(R.dimen.player_select_esipode_distance);
//
//        if (mType == PlayVideoInfoMgr.TYPE_TV) {
//            dialogWidth = 320;
//            view.setBackgroundResource(R.drawable.bg_player_box);
//            distance = (int) mContext.getResources().getDimension(R.dimen.player_select_esipode_distance_tv);
//        } else {
//            dialogWidth = dis.widthPixels * 3 / 5;
//            view.setBackgroundResource(R.drawable.bg_player_boxone);
//        }
//
//        //打开Dialog回调
//        if (mOnPlayControlCallback != null) {
//            mOnPlayControlCallback.onPlay(OnPlayControlListener.TYPE_PAUSE);
//        }
//
//        popupWindow = new PopupWindow(view, dialogWidth, dialogHeight);
//
//        int[] location = new int[2];
//        viewParent.getLocationOnScreen(location);
//
//        ///popupWindow.showAtLocation(viewParent, Gravity.NO_GRAVITY, location[0]-dialogWidth+distance, location[1]-dialogHeight);
//        Log.i("LJD", "location x = " + location[0] + " y=" + (location[1] - popupWindow.getHeight()) + " height=" + popupWindow.getHeight());
//        popupWindow.showAtLocation(viewParent, Gravity.NO_GRAVITY, location[0] - dialogWidth + distance, location[1] - dialogHeight);
//        //popupWindow.showAsDropDown(viewParent);
//        popupWindow.setOutsideTouchable(true);
//
//        popupWindow.setOnDismissListener(new OnDismissListener() {
//
//            @Override
//            public void onDismiss() {
//                if (mOnPlayControlCallback != null) {
//                    mOnPlayControlCallback.onPlay(OnPlayControlListener.TYPE_REPLAY);
//                }
//            }
//        });
//
//
//    }
//
//    public boolean isShowing() {
//        if (popupWindow != null) {
//            return popupWindow.isShowing();
//        }
//
//        return false;
//    }
//
//    public void dismiss() {
//        if (popupWindow != null) {
//            popupWindow.dismiss();
//        }
//    }
//
//    private class ListAdapter extends BaseAdapter {
//
//        public ListAdapter() {
//
//        }
//
//        @Override
//        public int getCount() {
//            return mList.size();
//        }
//
//        @Override
//        public Object getItem(int position) {
//            return mList.get(position);
//        }
//
//        @Override
//        public long getItemId(int position) {
//            return position;
//        }
//
//        @SuppressLint("NewApi")
//        @Override
//        public View getView(final int position, View convertView, ViewGroup parent) {
//            if (convertView == null) {
//                convertView = inflater.inflate(R.layout.layout_select_episode_video_item, null);
//
//            }
//
//
//            TextView tvVideo = (TextView) convertView
//                    .findViewById(R.id.tv_episose_video);
//            LayoutParams params = (LayoutParams) tvVideo.getLayoutParams();
//
//            PlayVideoInfo info = mList.get(position);
//            if (info != null) {
//                tvVideo.setText(info.getSubFileName());
//            }
//
//            TextView tvSizeOrTime = (TextView) convertView.findViewById(R.id.tv_size_or_time);
//            if (mType == PlayVideoInfoMgr.TYPE_TV) {
//                tvSizeOrTime.setVisibility(View.GONE);
//                params.addRule(RelativeLayout.CENTER_IN_PARENT, R.id.tv_episose_video);
//            } else {
//                params.addRule(RelativeLayout.ALIGN_PARENT_LEFT, R.id.tv_episose_video);
//                String des = "";
//                if (TextUtils.isEmpty(info.getProductTime())) {
//                    des = "未知";
//
//                    String tempFileSizeStr = Util.convertFileSize(
//                            info.getFileSize(), 0);
//                    if (!tempFileSizeStr.equalsIgnoreCase("0B")) {
//                        des = Util.convertFileSize(
//                                info.getFileSize(), 0);
//                    }
//                } else {
//                    des = info.getProductTime() + "期";
//                }
//
//                tvSizeOrTime.setVisibility(View.VISIBLE);
//                tvSizeOrTime.setText(des);
//
//            }
//
//            LayoutParams tvSizeOrTimeParams = (LayoutParams) tvVideo.getLayoutParams();
//            if (PlayVideoInfoMgr.getInstance().getmCurPlay() == position) {
//                convertView.setBackgroundResource(R.drawable.bg_player_box_selected);
//
//            } else {
//                convertView.setBackgroundResource(R.drawable.selector_player_select_movie);
//            }
//
//            convertView.setOnClickListener(new OnClickListener() {
//
//                @Override
//                public void onClick(View arg0) {
//                    // TODO Auto-generated method stub
//                    if (popupWindow != null) {
//                        popupWindow.dismiss();
//                    }
//
//                    if (mOnSelectPlayCallback != null) {
//                        mOnSelectPlayCallback.onPlay(position);
//                    }
//                }
//            });
//
//
//            return convertView;
//        }
//    }
//
//    /**
//     * 播放回调
//     *
//     * @author admin
//     */
//    public interface OnPlaySelectListener {
//        public void onPlay(int index);
//
//    }
//
//    public interface OnPlayControlListener {
//        public final static int TYPE_REPLAY = 1;    //对话框消失后继续播放
//        public final static int TYPE_PAUSE = 2;        //打开对话框暂停
//
//        public void onPlay(int type);
//    }
//
//}
