//package com.runmit.sweedee.manager;
//
//import java.util.ArrayList;
//import java.util.Arrays;
//import java.util.HashMap;
//import java.util.concurrent.ExecutorService;
//import java.util.concurrent.Executors;
//
//import android.content.Context;
//
//import com.runmit.sweedee.StoreApplication;
//import com.runmit.sweedee.opengl.utils.AnimFrameData;
//import com.runmit.sweedee.opengl.utils.GzipUtil;
//import com.runmit.sweedee.opengl.utils.ObjectInfo;
//import com.runmit.sweedee.opengl.utils.ResLoaderMethod;
//import com.runmit.sweedee.util.XL_Log;
//
//public class OpenGLResourceManager {
//	
//	XL_Log log = new XL_Log(OpenGLResourceManager.class);
//	
//	/**各场景与数据的映射*/
//	private HashMap<ResourceType,Object> mSceneAnimMap = new HashMap<ResourceType, Object>();
//	
//	/**各数据加载对应的线程锁*/
//	private HashMap<Object,Object> mObjSyncmMap = new HashMap<Object, Object>();
//	
//	private ArrayList<ResourceType> mResourceNeedToLoadList = new ArrayList<ResourceType>();
//	
//	/**加载所有资源的线程池*/
//	private ExecutorService mExecutor = Executors.newCachedThreadPool();
//	
//	private static OpenGLResourceManager instance;
//	
//	private Context mContext;
//	
//    public static OpenGLResourceManager getInstance(){
//        if (instance == null){
//        	instance = new OpenGLResourceManager();
//        }
//        return instance;
//    }
//    
//    private OpenGLResourceManager(){
//    	//将需要预加载的资源类型放到此处,我们将所有的都加进来，不需要预加载的remove掉即可
//    	mResourceNeedToLoadList.addAll(Arrays.asList(ResourceType.values()));
//    	mContext = StoreApplication.INSTANCE;
//    }
//    
//    
//    public void loadAllData(){
//    	for(final ResourceType resType:mResourceNeedToLoadList){
////    		mExecutor.execute(new Runnable() {				
////				@Override
////				public void run() {	
////					long starttime = System.currentTimeMillis();
////					Object data = loadDataImpl(mContext, resType);				
////					if(data != null){
////						mSceneAnimMap.put(resType,data);	
////					}
////					log.debug("load file duration="+(System.currentTimeMillis() - starttime));
////					Object lock = mObjSyncmMap.get(resType);
////					if(lock != null){
////						synchronized (lock) {
////							lock.notifyAll();
////						}
////					}
////					
////				}
////			});
//    	}
//
//    }
//    
//    /**获取场景动画数据*/
//    public AnimFrameData getSceneAnimData(ResourceType resType){
//    	AnimFrameData  animData = null;
//    	if(resType.resourceType == ResourceType.TYPE_ANIM){
//    		Object data = getDataImpl(resType);
//        	if(data != null && data instanceof AnimFrameData){
//        		animData = (AnimFrameData) data;	
//        	}
//    	}
//    	return animData;
//    }
//    
//    /**获取顶点信息数据*/
//    public ObjectInfo getVertexObjData(ResourceType resType){
//    	ObjectInfo objData = null;
//    	if(resType.resourceType == ResourceType.TYPE_OBJ || resType.resourceType == ResourceType.TYPE_MESH){
//    		Object data = getDataImpl(resType);
//    		if(data != null && data instanceof ObjectInfo){
//    			objData = (ObjectInfo) data;	
//        	}
//    	}
//    	return objData;
//    }
//    
//    /**获取着色器数据*/
//    public String getShaderString(ResourceType resType){
//    	String shader = null;
//    	if(resType.resourceType == ResourceType.TYPE_SHADER){
//    		Object data = getDataImpl(resType);
//    		if(data != null && data instanceof String){
//    			shader = (String) data;	
//        	}
//    	}
//    	return shader;
//    }
//    
//    /**获取大的背景图*/
//    /**
//    public Bitmap getLargeBitmap(ResourceType resType){
//    	Bitmap bitmap = null;
//    	if(resType.resourceType == ResourceType.TYPE_BITMAP){
//    		Object data = getDataImpl(resType);
//    		if(data != null && data instanceof Bitmap){
//    			bitmap = (Bitmap) data;	
//        	}
//    	}
//    	return bitmap;
//    }*/
//    
//    /**我们需要根据不同的资源类型调用不同的方法*/
//    private Object loadDataImpl(Context ctx,ResourceType resType){
//    	Object data = null;
//    	switch (resType.resourceType) {
//		case ResourceType.TYPE_ANIM:	
//		case ResourceType.TYPE_MESH:
//		case ResourceType.TYPE_OBJ:		
//			data = GzipUtil.loadGZipObject(ctx,resType.resourceFileName);
//			break;
//		case ResourceType.TYPE_SHADER:	
//			data = ResLoaderMethod.loadFromAssetsFile(ctx,resType.resourceFileName);
//			break;
//		default:
//			break;
//		}
//    	if(data == null){
//    		data = loadDataOriginImpl(ctx, resType);
//    	}
//    	log.debug("loadDataImpl resType = "+resType+" logData = "+data);
//    	return data;
//    }  
//    
//    /**
//     * 加载原始文件
//     * @param ctx
//     * @param resType
//     * @return
//     */
//    private Object loadDataOriginImpl(Context ctx,ResourceType resType){
//    	Object data = null;
//    	switch (resType.resourceType) {
//		case ResourceType.TYPE_ANIM:	
//			data = ResLoaderMethod.loadAnimDataFromFile(ctx, resType.resourceFileName);
//			break;
//		case ResourceType.TYPE_MESH:
//			data = ResLoaderMethod.loadObjFromResource(ctx, resType.resourceFileName);
//			break;
//		case ResourceType.TYPE_OBJ:		
//			data = ResLoaderMethod.loadFromFile(ctx,resType.resourceFileName);
//			break;
//		case ResourceType.TYPE_SHADER:	
//			data = ResLoaderMethod.loadFromAssetsFile(ctx,resType.resourceFileName);
//			break;
//		default:
//			break;
//		}
//    	Object logData = data;
//    	if(data instanceof String){
//    		logData = "data is a String";
//    	}
//    	log.debug("loadDataImpl resType = "+resType+" logData = "+logData);
//    	return data;
//    }
//    private Object getDataImpl(ResourceType resType){
//    	Object data = mSceneAnimMap.get(resType);
////    	if(data == null){  //等待加载完成，将会引起阻塞
////    		Object lock = mObjSyncmMap.get(resType);
////    		if(lock != null){
////    			synchronized (lock) {
////    				try {
////						lock.wait();
////					} catch (InterruptedException e) {						
////						e.printStackTrace();
////					}
////    			}	
////    		}
////    		//待完成后，再取一次,为空则无数据
////    		data = mSceneAnimMap.get(resType);
////    	}
//    	
//    	if(data == null){//无注册预加载，直接加载数据，将会引起阻塞
//    		data = loadDataImpl(mContext, resType);
//    	}  
//    	log.debug("getDataImpl resType = "+resType+" data = "+data);
//    	return data;
//    }
//    
// 
//
//	
//	
//	public static enum ResourceType{		
//		//场景的动画文件
//		AnimHomeScene("models/home/HomeScene.xml",ResourceType.TYPE_ANIM),//首页场景动画文件
//		AnimVedioScene("models/video/video_scene.xml",ResourceType.TYPE_ANIM),//电影页动画文件)
//		AnimBoxScene("models/video/box5.xml",ResourceType.TYPE_ANIM),//电影页动画文件)
//		
//		//顶点数据OBJ文件
//		ObjBackground("models/background/background.obj",ResourceType.TYPE_OBJ),//背景的顶点信息文件
//		ObjVedio("models/video/video.obj",ResourceType.TYPE_OBJ),//电影列表电影模型的顶点信息文件
//		ObjMenu("models/video/menu.obj",ResourceType.TYPE_OBJ),//电影场景中菜单开关的顶点信息文件
//		
//		//顶点数据Mesh文件
//		MeshHomeItem("models/home/homeObj.xml",ResourceType.TYPE_MESH),//首页模型的Mesh文件
//		MeshVedioItem("models/video/object7.xml",ResourceType.TYPE_MESH),//电影列表电影模型的Mesh文件
//		
//		MeshHomeGameItem("models/home/objs/game.mesh.xml", ResourceType.TYPE_MESH),
//        MeshHomeBookItem("models/home/objs/book.mesh.xml", ResourceType.TYPE_MESH),
//        MeshHomeMarketItem("models/home/objs/market.mesh.xml", ResourceType.TYPE_MESH),
//        MeshHomeMapItem("models/home/objs/map.mesh.xml", ResourceType.TYPE_MESH),
//        MeshHomeModelItem("models/home/objs/model.mesh.xml", ResourceType.TYPE_MESH),
//        MeshHomeMovielItem("models/home/objs/movie.mesh.xml", ResourceType.TYPE_MESH),
//        MeshHomeMusicItem("models/home/objs/music.mesh.xml", ResourceType.TYPE_MESH),
//        MeshHomeRadioItem("models/home/objs/radio.mesh.xml", ResourceType.TYPE_MESH),
//
//		Mesh11Item("models/background/11.mesh.xml",ResourceType.TYPE_MESH),
//		MeshPlaneItem("models/background/plane001.mesh.xml",ResourceType.TYPE_MESH),
//		MeshSphere1Item("models/background/Sphere001.mesh.xml",ResourceType.TYPE_MESH),
//		MeshSphere2Item("models/background/Sphere002.mesh.xml",ResourceType.TYPE_MESH),
//		
//		MeshItemMap("models/home/app/map.obj",ResourceType.TYPE_OBJ),
//		
//		MeshItem3D("models/home/app/3D.mesh.xml",ResourceType.TYPE_MESH),
//		Anim3DScene("models/home/app/3D.scene.xml",ResourceType.TYPE_ANIM),
//		
//		MeshItemCamera("models/home/app/camera.mesh.xml",ResourceType.TYPE_MESH),
//		AnimCameraScene("models/home/app/camera.scene.xml",ResourceType.TYPE_ANIM),
//		
//		MeshItemEye("models/home/app/eye.mesh.xml",ResourceType.TYPE_MESH),
//		AnimEyeScene("models/home/app/eye.scene.xml",ResourceType.TYPE_ANIM),
//		
//		MeshItemArrows("models/home/app/arrows.mesh.xml",ResourceType.TYPE_MESH),
//		AnimRentouScene("models/home/app/arrows.scene.xml",ResourceType.TYPE_ANIM),
//		
//		MeshItemCircle("models/home/app/circle.mesh.xml",ResourceType.TYPE_MESH),
//		AnimCircleScene("models/home/app/circle.scene.xml",ResourceType.TYPE_ANIM),
//		
//		MeshItemPLANK("models/home/app/plank.mesh.xml",ResourceType.TYPE_MESH),
//		AnimPLANKScene("models/home/app/plank.scene.xml",ResourceType.TYPE_ANIM),
//
//		//着色器shader文件
//		ShaderWaterFrag("shaders/frag_water.sh",ResourceType.TYPE_SHADER),
//		ShaderWaterVertex("shaders/vertex_water.sh",ResourceType.TYPE_SHADER),
//        ShaderBlurVertex("shaders/normal_blur_vertex.sh",ResourceType.TYPE_SHADER),//模糊所用的片元着色器文件
//        ShaderBlurFrag("shaders/normal_blur_frag.sh",ResourceType.TYPE_SHADER);//模糊所用的片元着色器文件
//		
//		//加载比较耗时的大图
//		//BitmapBackground(R.drawable.background+"",ResourceType.TYPE_BITMAP);//背景图
//		
//		public String resourceFileName;
//		
//		public int resourceType; 
//		
//		private ResourceType(String resourceFileName,int resourceType) {
//			this.resourceFileName = resourceFileName;
//			this.resourceType = resourceType;
//		}
//		
//		/**资源的类型集合*/
//		public static final int TYPE_ANIM = 0;
//		public static final int TYPE_OBJ = 1;
//		public static final int TYPE_MESH = 2;
//		//public static final int TYPE_BITMAP = 3;
//		public static final int TYPE_SHADER = 4;
//	}
//}
