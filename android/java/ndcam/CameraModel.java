package ndcam;

/**
 * @author luncliff@gmail.com
 */
public class CameraModel {
    static {
        System.loadLibrary("c++_shared");
        System.loadLibrary("ndk_camera");
    }

    private static Device[] devices = null;

    /**
     * Multiple init is safe. Only *first* invocation will take effect
     */
    public static synchronized native void Init();

    /**
     * Get the number of current camera devices
     * 
     * @return 0 if initialization failed or no device confirmed
     */
    public static native int GetDeviceCount();

    /**
     * @param devices SetDeviceData will provide appropriate internal library id
     */
    private static native void SetDeviceData(Device[] devices);

    /**
     * @return array of available devices.
     * @see Device
     */
    public static synchronized Device[] GetDevices() {
        if (devices == null) // allocate java objects
        {
            int count = GetDeviceCount();

            devices = new Device[count];
            for (int i = 0; i < count; ++i)
                devices[i] = new Device();

            SetDeviceData(devices);
        }
        return devices;
    }
}