package ndcam;

/**
 * @author luncliff@gmail.com
 */
public class CameraModel {
    static {
        System.loadLibrary("c++_shared");
        System.loadLibrary("ndk_camera");
    }

    public static native
    void Init();
}