/**
 * 
 */
package com.runmit.sweedee.manager;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.ObjectInputStream;
import java.io.ObjectOutputStream;
import java.io.Serializable;

import android.util.LruCache;

import com.runmit.sweedee.util.EnvironmentTool;

/**
 * @author Sven.Zhan * 
 * 
 * 提供一套简易的对象缓存方案，支持RAM缓存和DISK缓存
 */
public class ObjectCacheManager extends AbstractManager{
	
	/**RAM缓存的大小*/
	private static final int DEFAULT_MEM_CACHE_SIZE = 1024 * 8; // 8MB
	
	/**默认缓存有效期*/
	private static final long DEFAULT_DISKCACHE_AVAILABLE_TIME = 2*24*3600*1000L;//默认有效期2天
	
	/**RAM LRU*/
	LruCache<CacheKey, CacheValue> mMemCache;
	
	private static ObjectCacheManager instance;
	
	public synchronized static ObjectCacheManager getInstance(){
		if(instance == null){
			instance = new ObjectCacheManager();
		}
		return instance;
	}
	
	private ObjectCacheManager(){
		TAG = ObjectCacheManager.class.getSimpleName();
		mMemCache = new LruCache<CacheKey, CacheValue>(DEFAULT_MEM_CACHE_SIZE);
		
		/**清除过期磁盘缓存*/
		mAsyncHandler.post(new Runnable() {			
			@Override
			public void run() {
				try{
					clearOverdueDiskCache();
				}catch(Exception e){
					e.printStackTrace();
				}
			}
		});
	}
	
	
	public static void init(){
		getInstance();
	}
	
	/**
	 * 缓存到内存
	 * @param key 缓存的键值，为任意非空对象
	 * @param cacheObj 需要缓存的对象，为任意非空对象
	 */
	private void pushIntoRam(Object key,Object cacheObj){
		if(key == null || cacheObj == null){
			return;
		}		
		CacheKey cacheKey = new CacheKey(key, cacheObj.getClass());
		CacheValue cacheValue = new CacheValue(cacheObj);
		mMemCache.put(cacheKey, cacheValue);
	}
	
	/**
	 * 缓存到内存 和 磁盘
	 * @param option 磁盘缓存的参数,为空则不会进行磁盘缓存 
	 * @param key 缓存的键值，为任意非空对象
	 * @param cacheObj 需要缓存的对象，为任意非空对象
	 */
	public void pushIntoCache(Object key,Object cacheObj,DiskCacheOption option){
		pushIntoRam(key,cacheObj);		
		if(option != null && cacheObj instanceof Serializable){
			CacheKey cacheKey = new CacheKey(key, cacheObj.getClass());			
			String path = cacheKey.getDiskPathName(option);
			CacheValue cacheValue = new CacheValue(option.avialableTime,cacheObj);
			saveObjectToDisk(path, cacheValue);
		}else{
		}
	}
	
	/**
	 * 从缓存中获取对象
	 * @param key 缓存的键值，为任意非空对象
	 * @param objType 缓存对象的Class
	 * @param option  磁盘缓存的选项参数，若为空，则不会查找磁盘缓存
	 * @return key 和 objType 对应的缓存对象
	 */
	public Object pullFromCache(Object key,Class<?> objType,DiskCacheOption option){
		
		/** step1 从RAM中查找*/
		CacheKey cacheKey = new CacheKey(key, objType);
		CacheValue cachedValue = mMemCache.get(cacheKey);
		
		if(checkCacheValue(cachedValue, objType)){
			return cachedValue.realObject;
		}
		
		/** step2  如果DiskCacheOption不为空  从DISK中查找*/
		if(option != null){
			String path = cacheKey.getDiskPathName(option);
			cachedValue = loadObjectFromDisk(path);
			if(cachedValue != null && System.currentTimeMillis() - cachedValue.cacheTime >= cachedValue.availableTime){
				new File(path).delete();
				cachedValue = null;
			}
		}
		
		if(checkCacheValue(cachedValue, objType)){			
			return cachedValue.realObject;
		}
		return null;
	}
	
	/**
	 * 清除过期的磁盘缓存对象
	 */
	public synchronized void clearOverdueDiskCache(){
		long now = System.currentTimeMillis();
		
		/**先删外置磁盘数据*/
		DiskCacheOption external = new DiskCacheOption().useExternal(true);
		String[] efileStrs = new File(external.cachePath).list();
		if(efileStrs != null){
			for(String efile : efileStrs){
				String epath = external.cachePath+efile;
				CacheValue value = loadObjectFromDisk(epath);
				if(value != null && now - value.cacheTime >= value.availableTime){
					new File(epath).delete();
				}
			}
		}
		
		/**再删内置磁盘数据*/
		DiskCacheOption internal = new DiskCacheOption().useExternal(true);
		String[] ifileStrs = new File(internal.cachePath).list();
		if(ifileStrs != null){
			for(String ifile : ifileStrs){
				String ipath = internal.cachePath+ifile;
				CacheValue value = loadObjectFromDisk(ipath);
				if(value != null && now - value.cacheTime >= value.availableTime){
					new File(ipath).delete();
				}
			}
		}
	}
	
	/**
	 * 清除所有的磁盘缓存对象
	 */
	public synchronized void clearAllDiskCache(){
		
		mAsyncHandler.post(new Runnable() {			
			@Override
			public void run() {
				/**先删外置磁盘数据*/
				DiskCacheOption external = new DiskCacheOption().useExternal(true);
				String[] efileStrs = new File(external.cachePath).list();
				if(efileStrs != null){
					for(String efile : efileStrs){
						new File(external.cachePath+efile).delete();
					}
				}
				
				/**再删内置磁盘数据*/
				DiskCacheOption internal = new DiskCacheOption().useExternal(false);
				String[] ifileStrs = new File(internal.cachePath).list();
				if(ifileStrs != null){
					for(String ifile : ifileStrs){
						new File(internal.cachePath+ifile).delete();
					}
				}
			}
		});
	}

	private boolean checkCacheValue(CacheValue cachedValue,Class<?> objType){
		if(cachedValue != null && objType.isInstance(cachedValue.realObject)){			
			return true;
		}else{
			return false;
		}			
	}
	
	private synchronized void saveObjectToDisk(String filepath,CacheValue cacheValue){
		ObjectOutputStream out = null;
		try {
			File file = new File(filepath);
			if(!file.exists()){
				file.createNewFile();
			}
			FileOutputStream outstream = new FileOutputStream(file);
			out = new ObjectOutputStream(outstream);
			out.writeObject(cacheValue);//将对象写入流
		} catch (Exception e) {			
			e.printStackTrace();
		}finally{
			try {
				if(out != null){
					out.close(); // 清空并关闭流
				}				
			} catch (IOException e) {				
				e.printStackTrace();
			}
		}
	}
	
	private synchronized CacheValue loadObjectFromDisk(String filepath){
		FileInputStream in = null;
		ObjectInputStream s = null;
		CacheValue cacheValue = null;
		try{
			in = new FileInputStream(filepath); 
			s = new ObjectInputStream(in); 
			cacheValue = (CacheValue) s.readObject(); 
		}catch(Exception e){
			e.printStackTrace();
		}finally{			
			try {
				if(in != null) in.close();
				if(s != null) s.close();
			} catch (Exception e) {				
				e.printStackTrace();
			}
		}
		return cacheValue;
	}
	
	private class CacheKey{
		
		private Object key;
		
		private Class<?> classType;

		public CacheKey(Object key, Class<?> classType) {
			this.key = key;
			this.classType = classType;
		}
		
		public String getKeyName(){
			return classType.getName().hashCode()+"_"+key.hashCode()+"_"+key.hashCode();
		}
		
		public String getDiskPathName(DiskCacheOption option){
			return option.cachePath+getKeyName();
		}

		@Override
		public int hashCode() {
			final int prime = 31;
			int result = 1;
			result = prime * result + ((classType == null) ? 0 : classType.hashCode());
			result = prime * result + ((key == null) ? 0 : key.hashCode());
			return result;
		}

		@Override
		public boolean equals(Object obj) {
			if (this == obj)
				return true;
			if (obj == null)
				return false;
			if (getClass() != obj.getClass())
				return false;
			CacheKey other = (CacheKey) obj;
			if (classType == null) {
				if (other.classType != null)
					return false;
			} else if (!classType.equals(other.classType))
				return false;
			if (key == null) {
				if (other.key != null)
					return false;
			} else if (!key.equals(other.key))
				return false;
			return true;
		}
	}
	
	private static class CacheValue implements Serializable{
		
		private static final long serialVersionUID = 4486627140673183469L;

		public long cacheTime;
		
		public long availableTime = DEFAULT_DISKCACHE_AVAILABLE_TIME;
		
		public Object realObject;

		public CacheValue(Object realObject) {
			this.cacheTime = System.currentTimeMillis();
			this.realObject = realObject;
		}

		public CacheValue(long availableTime, Object realObject) {
			this(realObject);
			this.availableTime = availableTime;
		}
	}
	
	public static class DiskCacheOption{
		
		/**默认子目录*/
		private static final String CACHE_SUBFOLDER = "OCache"+File.separator;
		
		/**默认磁盘类型*/
		private static final boolean USE_EXTERNAL_DISK = true;
		
		/**是否缓存到设备外置存储*/
		private boolean useExternal = USE_EXTERNAL_DISK;
		
		/**缓存的有效期*/
		private long avialableTime = DEFAULT_DISKCACHE_AVAILABLE_TIME;
		
		/**缓存路径*/
		private String cachePath ;
		
		/**默认对象*/
		public static DiskCacheOption DEFAULT = new DiskCacheOption();
		
		public static DiskCacheOption NULL = null;
		
		public DiskCacheOption() {
			cachePath = generateDiskCachePath(useExternal);
		}
		
		private String generateDiskCachePath(boolean useExt){
			String parent = useExt ? EnvironmentTool.getExternalDataPath(CACHE_SUBFOLDER) : EnvironmentTool.getInternalDataPath(CACHE_SUBFOLDER);
			File f = new File(parent);
			if(!f.exists() || !f.isDirectory()){
				f.mkdirs();
			}
			return parent;
		}

		public DiskCacheOption useExternal(boolean useExternalDisk) {
			useExternal = useExternalDisk;
			cachePath = generateDiskCachePath(useExternal);
			return this;
		}
		
		public DiskCacheOption setAvialableTime(long avialableTime) {
			this.avialableTime = avialableTime;
			return this;
		}		
	}
}
