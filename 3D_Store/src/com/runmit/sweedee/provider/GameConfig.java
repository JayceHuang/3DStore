package com.runmit.sweedee.provider;

public class GameConfig {

	
	private String packageName;// 游戏包名 
	private int versionCode;//游戏版本号VersionCode
	private float x;//x参数
	private float y;//y参数
	private float z;//z参数
	private float w;//w参数
	private boolean status;//设置的可用状态
	private int dataVersion;//该配置数据的版本
	private long updateAt;

	public GameConfig(String packageName, int versionCode, float x, float y, float z, float w, boolean status, int dataVersion,long updateAt) {
		super();
		this.packageName = packageName;
		this.versionCode = versionCode;
		this.x = x;
		this.y = y;
		this.z = z;
		this.w = w;
		this.status = status;
		this.dataVersion = dataVersion;
		this.updateAt = updateAt;
	}

	public String getPackageName() {
		return packageName;
	}

	public void setPackageName(String packageName) {
		this.packageName = packageName;
	}

	public int getVersionCode() {
		return versionCode;
	}

	public void setVersionCode(int versionCode) {
		this.versionCode = versionCode;
	}

	public float getX() {
		return x;
	}

	public void setX(float x) {
		this.x = x;
	}

	public float getY() {
		return y;
	}

	public void setY(float y) {
		this.y = y;
	}

	public float getZ() {
		return z;
	}

	public void setZ(float z) {
		this.z = z;
	}

	public float getW() {
		return w;
	}

	public void setW(float w) {
		this.w = w;
	}

	public boolean isStatus() {
		return status;
	}

	public void setStatus(boolean status) {
		this.status = status;
	}

	public int getDataVersion() {
		return dataVersion;
	}

	public void setDataVersion(int dataVersion) {
		this.dataVersion = dataVersion;
	}
	
	public long getUpdateAt() {
		return updateAt;
	}

	public void setUpdateAt(long updateAt) {
		this.updateAt = updateAt;
	}

	@Override
	public String toString() {
		return "GameConfig [packageName=" + packageName + ", versionCode=" + versionCode + ", x=" + x + ", y=" + y + ", z=" + z + ", w=" + w + ", status=" + status + ", dataVersion=" + dataVersion
				+ ", updateAt=" + updateAt + "]";
	}
	
}
