package ndcam;

import android.media.Image;

/**
 * @author luncliff@gmail.com
 */
public class CameraModel {
    static {
        System.loadLibrary("c++_shared");
        System.loadLibrary("ndk_camera");
    }

    private static Device[] devices = null;

    public static native void Init();

    public static native int GetDeviceCount();

    private static native void SetDeviceData(Device[] devices);

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