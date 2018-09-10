package ndcam;

import android.media.Image;

public class Device
{
    /**
     * library internal identifier
     */
    public short id = -1;

    public native boolean isFront();
    public native boolean isBack();
    public native boolean isExternal();

    public native void capture() throws RuntimeException;
}
