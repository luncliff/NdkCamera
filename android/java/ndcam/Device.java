package ndcam;

import android.hardware.camera2.CameraCaptureSession;
import android.hardware.camera2.CameraCharacteristics;
import android.hardware.camera2.CameraDevice;
import android.os.Handler;
import android.view.Surface;

/**
 * In short, this is combination of
 * {@link android.hardware.camera2.CameraDevice} and
 * {@link android.hardware.camera2.CameraCaptureSession}
 *
 * TODO - capture request with configuration
 *
 * @author luncliff@gmail.com
 */
public class Device {
    /** library internal identifier */
    public short id = -1;

    /**
     * Only {@link CameraModel} will access to this
     */
    Device() {
    }

    /**
     * @return {@link CameraCharacteristics#LENS_FACING_FRONT } ||
     *         {@link CameraCharacteristics#LENS_FACING_BACK } ||
     *         {@link CameraCharacteristics#LENS_FACING_EXTERNAL }
     */
    public native byte facing();

    /**
     * Open a camera device
     * 
     * @see android.hardware.camera2.CameraManager#openCamera(String,
     *      CameraDevice.StateCallback, Handler)
     */
    private native void open() throws RuntimeException;

    /**
     * Close both internal session and device handle so that other device can be
     * opened This method is named to support {@link AutoCloseable} in future update
     *
     * @see CameraCaptureSession#close()
     * @see CameraDevice#close()
     */
    public native void close();

    /**
     * User of the Camera 2 API must provide valid Surface.
     *
     * @see "https://developer.android.com/reference/android/view/Surface"
     * @param surface output surface for Camera 2 API
     */
    public void repeat(Surface surface) throws RuntimeException {
        // ensure the camera is opened
        this.open();
        // Create a session with repeating capture request
        startRepeat(surface);
    }

    /**
     * Open a camera and create a session with repeating capture request
     * 
     * @param surface camera will provide image to given surface
     */
    private native void startRepeat(Surface surface);

    /**
     * Stop the repeating request
     * 
     * @see Device#startRepeat(Surface)
     */
    public native void stopRepeat();

    /**
     * User of the Camera 2 API must provide valid Surface.
     *
     * @see "https://developer.android.com/reference/android/view/Surface"
     * @param surface output surface for Camera 2 API
     */
    public void capture(Surface surface) throws RuntimeException {
        this.open();
        startCapture(surface);
    }

    /**
     * Open a camera and create a session with capture request
     * 
     * @param surface camera will provide image to given surface
     */
    private native void startCapture(Surface surface);

    /**
     * Stop(abort) the capture request
     * 
     * @see Device#startCapture(Surface)
     */
    public native void stopCapture();
}
