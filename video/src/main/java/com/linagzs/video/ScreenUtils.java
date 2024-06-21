package com.linagzs.video;

import android.app.Activity;
import android.content.Context;
import android.content.res.Configuration;
import android.graphics.Bitmap;
import android.graphics.Rect;
import android.util.DisplayMetrics;
import android.view.View;
import android.view.WindowManager;

/**
 * Created by DELL on 2019/4/25.
 */
public class ScreenUtils {
	private static int mStatusHeight;
	
	private ScreenUtils() {
		throw new UnsupportedOperationException("cannot be instantiated");
	}
	
	public static int getScreenWidth(Context context) {
		WindowManager wm = (WindowManager)context.getSystemService(Context.WINDOW_SERVICE);
		DisplayMetrics outMetrics = new DisplayMetrics();
		wm.getDefaultDisplay().getMetrics(outMetrics);
		return outMetrics.widthPixels;
	}
	
	public static float getScreenSize(Context context) {
		WindowManager wm = (WindowManager)context.getSystemService(Context.WINDOW_SERVICE);
		DisplayMetrics outMetrics = new DisplayMetrics();
		wm.getDefaultDisplay().getMetrics(outMetrics);
		return (float)(Math.sqrt(Math.pow((double)outMetrics.widthPixels, 2.0D) + Math.pow((double)outMetrics.heightPixels, 2.0D)) / (double)outMetrics.density);
	}
	
	public static int getScaleWidthOfScreen(Context context, float ratio) {
		WindowManager wm = (WindowManager)context.getSystemService(Context.WINDOW_SERVICE);
		DisplayMetrics outMetrics = new DisplayMetrics();
		wm.getDefaultDisplay().getMetrics(outMetrics);
		float density = (float)(outMetrics.widthPixels / outMetrics.densityDpi);
		int width = outMetrics.widthPixels;
		boolean isLand = context.getResources().getConfiguration().orientation == 2;
		float r;
		if (density <= 2.0F) {
			r = isLand ? 0.7F : 0.9F;
		} else if (density <= 2.25F) {
			r = isLand ? 0.68F : 0.88F;
		} else if (density <= 3.75F) {
			r = isLand ? 0.62F : 0.8F;
		} else if (density <= 4.8F) {
			r = isLand ? 0.6F : 0.7F;
		} else {
			r = isLand ? 0.5F : 0.6F;
		}
		
		r = ratio / 0.9F * r;
		return (int)((float)width * r);
	}
	
	public static int getScreenHeight(Context context) {
		WindowManager wm = (WindowManager)context.getSystemService(Context.WINDOW_SERVICE);
		DisplayMetrics outMetrics = new DisplayMetrics();
		wm.getDefaultDisplay().getMetrics(outMetrics);
		return outMetrics.heightPixels;
	}
	
	public static int getScreenMaxHeight(Context context) {
		WindowManager wm = (WindowManager)context.getSystemService(Context.WINDOW_SERVICE);
		DisplayMetrics outMetrics = new DisplayMetrics();
		wm.getDefaultDisplay().getMetrics(outMetrics);
		return Math.max(outMetrics.heightPixels, outMetrics.widthPixels);
	}
	
	public static int getScreenMinWidth(Context context) {
		WindowManager wm = (WindowManager)context.getSystemService(Context.WINDOW_SERVICE);
		DisplayMetrics outMetrics = new DisplayMetrics();
		wm.getDefaultDisplay().getMetrics(outMetrics);
		return Math.min(outMetrics.heightPixels, outMetrics.widthPixels);
	}
	
	public static DisplayMetrics getDisplayMetrics(Context context) {
		WindowManager wm = (WindowManager)context.getSystemService(Context.WINDOW_SERVICE);
		DisplayMetrics outMetrics = new DisplayMetrics();
		wm.getDefaultDisplay().getMetrics(outMetrics);
		return outMetrics;
	}
	
	public static float getScreenRatio(Context context) {
		WindowManager wm = (WindowManager)context.getSystemService(Context.WINDOW_SERVICE);
		DisplayMetrics outMetrics = new DisplayMetrics();
		wm.getDefaultDisplay().getMetrics(outMetrics);
		return (float)outMetrics.heightPixels / (float)outMetrics.widthPixels;
	}
	

	public static Bitmap snapShotWithStatusBar(Activity activity) {
		View view = activity.getWindow().getDecorView();
		view.setDrawingCacheEnabled(true);
		view.buildDrawingCache();
		Bitmap bmp = view.getDrawingCache();
		int width = getScreenWidth(activity);
		int height = getScreenHeight(activity);
		Bitmap bp = Bitmap.createBitmap(bmp, 0, 0, width, height);
		view.destroyDrawingCache();
		return bp;
	}
	
	public static Bitmap snapShotWithoutStatusBar(Activity activity) {
		View view = activity.getWindow().getDecorView();
		view.setDrawingCacheEnabled(true);
		view.buildDrawingCache();
		Bitmap bmp = view.getDrawingCache();
		Rect frame = new Rect();
		activity.getWindow().getDecorView().getWindowVisibleDisplayFrame(frame);
		int statusBarHeight = frame.top;
		int width = getScreenWidth(activity);
		int height = getScreenHeight(activity);
		Bitmap bp = Bitmap.createBitmap(bmp, 0, statusBarHeight, width, height - statusBarHeight);
		view.destroyDrawingCache();
		return bp;
	}
	
	public static double getScreenDimension(Context context) {
		WindowManager wm = (WindowManager)context.getSystemService(Context.WINDOW_SERVICE);
		DisplayMetrics dm = new DisplayMetrics();
		wm.getDefaultDisplay().getMetrics(dm);
		double x = Math.pow((double)((float)dm.widthPixels / dm.xdpi), 2.0D);
		double y = Math.pow((double)((float)dm.heightPixels / dm.ydpi), 2.0D);
		return Math.sqrt(x + y);
	}
	
	public static int getScreenRotation(Context context) {
		WindowManager wm = (WindowManager)context.getSystemService(Context.WINDOW_SERVICE);
		int rotation = wm.getDefaultDisplay().getRotation();
		if (rotation == 0) {
			return 0;
		} else if (rotation == 1) {
			return 90;
		} else if (rotation == 2) {
			return 180;
		} else {
			return rotation == 3 ? 270 : 0;
		}
	}
	
	public static boolean isLandscape(Context context) {
		int ortation = context.getResources().getConfiguration().orientation;
		return ortation == 2;
	}
	
	public static boolean isPortrait(Context context) {
		int ortation = context.getResources().getConfiguration().orientation;
		return ortation == 1;
	}
	
	public static boolean isTablet(Context context) {
		DisplayMetrics metrics = getDisplayMetrics(context);
		return (int)((float)Math.min(metrics.widthPixels, metrics.heightPixels) / metrics.density) >= 600;
	}

	/**
	 * 谷歌的方法
	 * @param context
	 * @return
	 */
	public static boolean isTabletGoogle(Context context) {
		return (context.getResources().getConfiguration().screenLayout & Configuration.SCREENLAYOUT_SIZE_MASK) >= Configuration.SCREENLAYOUT_SIZE_LARGE;
	}
	public static int getScreenSW(Context context) {
		DisplayMetrics metrics = getDisplayMetrics(context);
		return (int)((float)Math.min(metrics.widthPixels, metrics.heightPixels) / metrics.density);
	}
}
