package hcisocket;

/**
 * 
 * Java fork of node-bluetooth-hci-socket 0.4.2
 * 
 * @author Martin Petzold
 *
 */
public class BluetoothHciSocket {
	
	public native void newInstance();
	public native int bindRaw(int pDevId);
	public native int bindUser(int pDevId);
	public native void bindControl();
	public native boolean isDevUp();
	public native void setFilter(byte[] data);
	public native void start();
	public native void stop();
	public native void write(byte[] data);
	
	private DataHandler dataHandler = null;
	private ErrorHandler errorHandler = null;
	
	public BluetoothHciSocket(int deviceId) {

		try { System.loadLibrary("uv"); } catch(UnsatisfiedLinkError e) { /* Just "pretended" to preload this library! */ }
		System.loadLibrary("BluetoothHciSocket");
		
		this.newInstance();
		
		this.bindRaw(deviceId);
		
	}
	
	public void emit(byte[] data) {
		if(this.dataHandler != null) {
			this.dataHandler.handleData(new Buffer(data));
		}
	}
	
	public void error(String message) {
		if(this.errorHandler != null) {
			this.errorHandler.handleError(message);
		}
	}
	
	public void onData(DataHandler dataHandler) {
		this.dataHandler = dataHandler;
	}
	
	public void onError(ErrorHandler errorHandler) {
		this.errorHandler = errorHandler;
	}
	
}
