package com.runmit.libsdk.util;

import java.math.BigDecimal;

import android.app.ProgressDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.pm.PackageManager.NameNotFoundException;
import android.os.Environment;
import android.text.Html;
import android.view.KeyEvent;


public class SdkUtil {
	
	public static enum DialogTextColor {
        BLUE, WHITE,
    }
	
    public static boolean isSDCardExist() {
        return Environment.MEDIA_MOUNTED.equalsIgnoreCase(Environment.getExternalStorageState());
    }
	
    public static void dismissDialog(ProgressDialog dialog) {
        if (null != dialog) {
            try {
                dialog.dismiss();
            } catch (IllegalArgumentException e) {
                e.printStackTrace();
            }
        }
    }
    
    public static boolean showDialog(ProgressDialog dialog, String msg) {
        return showDialog(dialog, msg, false);
    }

    public static boolean showDialog(ProgressDialog dialog, String msg, boolean flag) {
        if (dialog == null) {
            return false;
        }
        dismissDialog(dialog);

        dialog.setMessage(msg);
        dialog.setCanceledOnTouchOutside(false);
        dialog.setCancelable(true);
        if (flag) {
            dialog.setOnKeyListener(new DialogInterface.OnKeyListener() {
                @Override
                public boolean onKey(DialogInterface dialog, int keyCode, KeyEvent event) {
                    return true;
                }
            });
        }
        dialog.getWindow().setBackgroundDrawableResource(android.R.color.transparent);
        try {
            dialog.show();
        } catch (Exception e) {
            e.printStackTrace();
        }
        return true;
    }
    
	/**
     * 利用html代码让dialog字体变蓝
     */
    public static android.text.Spanned setColourText(Context context, int resId, DialogTextColor color) {
    	return setColourText(context.getString(resId), color);
    }
    
    public static android.text.Spanned setColourText(String str, DialogTextColor color) {
        try {
            switch (color) {
                case BLUE:
                    return Html.fromHtml("<font color='#1e91eb'>" + str + "</font>");
                case WHITE:
                    return Html.fromHtml("<font color='#FFFFFF'>" + str + "</font>");
                default:
                    return Html.fromHtml("<font color='#FFFFFF'>" + str + "</font>");
            }
        } catch (Exception e) {
            return null;
        }
    }
    
    public static int getVerCode(Context context) {
		int verCode = -1;
		try {
			verCode = context.getPackageManager().getPackageInfo(context.getPackageName(), 0).versionCode;
		} catch (NameNotFoundException e) {
			e.printStackTrace();
		} catch (NullPointerException e) {
			e.printStackTrace();
		}
		return verCode;
	}

	public static String getVerName(Context context) {
		String verName = "";
		try {
			verName = context.getPackageManager().getPackageInfo(context.getPackageName(), 0).versionName;
		} catch (NameNotFoundException e) {
			e.printStackTrace();
		} catch (NullPointerException e) {
			e.printStackTrace();
		}
		return verName;

	}

	public static String getMainVerName(Context context) {
		return getVerName(context).substring(0, 3);
	}
	
	
    private static final long BASE_B = 1L;
    private static final long BASE_KB = 1024L;
    private static final long BASE_MB = 1024L * 1024L;
    private static final long BASE_GB = 1024L * 1024L * 1024L;
    private static final long BASE_TB = 1024L * 1024L * 1024L * 1024L;
    private static final String UNIT_BIT = "B";
    private static final String UNIT_KB = "K";
    private static final String UNIT_MB = "M";
    private static final String UNIT_GB = "G";
    private static final String UNIT_TB = "T";
    private static final String UNIT_PB = "P";
	public static String convertFileSize(long file_size, int precision, boolean isForceInt) {
        long int_part = 0;
        double fileSize = file_size;
        double floatSize = 0L;
        long temp = file_size;
        int i = 0;
        double base = 1;
        String baseUnit = "M";
        String fileSizeStr = null;
        int indexMid = 0;

        while (temp / 1024 > 0) {
            int_part = temp / 1024;
            temp = int_part;
            i++;
        }
        switch (i) {
            case 0:
                // B
                base = BASE_B;
                floatSize = fileSize / base;
                baseUnit = UNIT_BIT;
                break;

            case 1:
                // KB
                base = BASE_KB;
                floatSize = fileSize / base;
                baseUnit = UNIT_KB;
                break;

            case 2:
                // MB
                base = BASE_MB;
                floatSize = fileSize / base;
                baseUnit = UNIT_MB;
                break;

            case 3:
                // GB
                base = BASE_GB;
                floatSize = fileSize / base;
                baseUnit = UNIT_GB;
                break;

            case 4:
                // TB
                // base = BASE_TB;
                floatSize = (fileSize / BASE_GB) / BASE_KB;
                baseUnit = UNIT_TB;
                break;
            case 5:
                // PB
                // base = BASE_PB;
                floatSize = (fileSize / BASE_GB) / BASE_MB;
                baseUnit = UNIT_PB;
                break;
            default:
                break;
        }
        fileSizeStr = Double.toString(floatSize);
        if (precision == 0 || (isForceInt && i < 3)) {
            indexMid = fileSizeStr.indexOf('.');
            if (-1 == indexMid) {
                return fileSizeStr + baseUnit;
            }
            return fileSizeStr.substring(0, indexMid) + baseUnit;
        }

        try {
            BigDecimal b = new BigDecimal(Double.toString(floatSize));
            BigDecimal one = new BigDecimal("1");
            double dou = b.divide(one, precision, BigDecimal.ROUND_HALF_UP).doubleValue();
            return dou + baseUnit;
        } catch (Exception e) {
            indexMid = fileSizeStr.indexOf('.');
            if (-1 == indexMid) {
                return fileSizeStr + baseUnit;
            }
            if (fileSizeStr.length() <= indexMid + precision + 1) {
                return fileSizeStr + baseUnit;
            }
            if (indexMid < 3) {
                indexMid += 1;
            }
            if (indexMid + precision < 6) {
                indexMid = indexMid + precision;
            }
            return fileSizeStr.substring(0, indexMid) + baseUnit;
        }
    }
}
