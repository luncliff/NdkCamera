package ndcam;

import android.hardware.camera2.CameraCharacteristics;
import android.media.ImageReader;
import android.view.Surface;

import java.util.concurrent.Future;

/**
 * @author luncliff@gmail.com
 */
public class Device {
    /** library internal identifier */
    public short id = -1;

    Device() {}

    /**
     * @return
     *      {@link CameraCharacteristics#LENS_FACING_FRONT } ||
     *      {@link CameraCharacteristics#LENS_FACING_BACK } ||
     *      {@link CameraCharacteristics#LENS_FACING_EXTERNAL }
     */
    public native byte facing();

    native void open() throws RuntimeException;
    public native void close();

    /**
     * User of the Camera 2 API must provide valid Surface.
     * @param surface output surface for Camera 2 API
     */
    public void repeat(Surface surface) throws RuntimeException{
        this.open();
        startRepeat(surface);
    }
    private native void startRepeat(Surface surface);
    public native void stopRepeat();

    public void capture(Surface surface) throws RuntimeException {
        this.open();
        // camera will provide image to given surface
        startCapture(surface);
    }
    private native void startCapture(Surface surface);
    public native void stopCapture();
}
