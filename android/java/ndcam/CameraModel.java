package ndcam;

public class CameraModel {
    static {
        System.loadLibrary("c++_shared");
        System.loadLibrary("ndk_camera");
    }

    public static void Init() {
        // ...
    }
}