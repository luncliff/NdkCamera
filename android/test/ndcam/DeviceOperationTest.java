package ndcam;

import android.graphics.Camera;
import android.graphics.ImageFormat;
import android.hardware.camera2.CameraCharacteristics;
import android.media.CameraProfile;
import android.media.Image;
import android.media.ImageReader;
import android.support.test.runner.AndroidJUnit4;
import android.util.Log;
import android.view.Surface;

import junit.framework.Assert;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

import java.util.concurrent.Future;

@RunWith(AndroidJUnit4.class)
public class DeviceOperationTest extends CameraModelTest {

    ImageReader reader;
    Device camera;

    @Before
    public void CreateImageReader() {
        // TODO: default value?
        // 1280 * 720
        // 1920 * 1080, 30 FPS, YCbCr 4:2:0(YUV_420_888)
        reader = ImageReader.newInstance(
                1920, 1080, ImageFormat.YUV_420_888,
                30);

        Assert.assertNotNull(reader);
    }

    @Before
    public void AcquireDevice()
    {
        CameraModel.Init();
        Device[] devices = CameraModel.GetDevices();
        Assert.assertNotNull(devices);
        camera = null;
        // get rear camera
        for (Device device : devices)
            if (device.facing() == CameraCharacteristics.LENS_FACING_BACK)
                camera = device;

        Assert.assertNotNull(camera);
    }

    @After
    public void CloseDevice(){
        Assert.assertNotNull(camera);
        camera.close();
    }

    @Test
    public void CloseWithoutStopRepeat() throws Exception {
        // start repeating capture operation
        camera.repeat(reader.getSurface());

        // Android Camera 2 API uses background thread.
        // Give some time to the framework.
        Thread.sleep(100);

        // camera.stop(); // !!! stop is missing !!!
    }

    @Test
    public void TryRepeatCapture() throws Exception{
        // start repeating capture operation
        camera.repeat(reader.getSurface());

        Thread.sleep(100);

        Image image = null;
        int i = 0, count = 0;
        while(i++ < 100) // try 100 capture (repeat mode)
        {
            // expect 30 FPS...
            Thread.sleep(30);

            // Fetch image 1 by 1.
            image = reader.acquireNextImage();
            if(image == null)
                continue;

            Log.v("ndk_camera",
                    String.format("format %d width %d height %d timestamp %d",
                            image.getFormat(), image.getWidth(), image.getHeight(),
                            image.getTimestamp())
            );
            image.close();
            count += 1;
        }
        camera.stopRepeat(); // stop after iteration

        Assert.assertNotNull(image);    // ensure at least 1 image was acquired
        Assert.assertTrue(i > count);   // !!! some images might be dropped !!!
    }

    @Test
    public void CloseWithoutStopCapture() throws Exception {
        // start repeating capture operation
        camera.repeat(reader.getSurface());

        // Android Camera 2 API uses background thread.
        // Give some time to the framework.
        Thread.sleep(100);

        // camera.stop(); // !!! stop is missing !!!
    }

    @Test
    public void TryCapture() {
        // start capture operation
        camera.capture(reader.getSurface());

        Image image = null;
        do
        {
            image = reader.acquireLatestImage();
        }while(image == null);

        Assert.assertNotNull(image);

        Log.v("ndk_camera",
                String.format("format %d width %d height %d timestamp %d",
                        image.getFormat(), image.getWidth(), image.getHeight(),
                        image.getTimestamp())
        );
        image.close();

        camera.stopCapture(); // stop after capture
    }
}
