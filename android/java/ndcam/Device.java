package ndcam;

import android.media.ImageReader;
import android.view.Surface;

import java.util.concurrent.Future;

public class Device {
    /**
     * library internal identifier
     */
    public short id = -1;

    Device() {
    }

    public native boolean isFront();

    public native boolean isBack();

    public native boolean isExternal();

    private native void startRepeat(Surface surface);

    private native void startCapture(Surface surface);

    private native void open() throws RuntimeException;

    public void repeat(ImageReader reader) {
        this.open();

        // TODO: default value?
        // 512,512, ImageFormat.YUV_420_888, 30;

        // camera will provide image to given surface
        Surface surface = reader.getSurface();
        startRepeat(surface);
    }

    public void capture(ImageReader reader) {
        this.open();

        // camera will provide image to given surface
        Surface surface = reader.getSurface();
        startCapture(surface);
    }

    public native void stop();
}
