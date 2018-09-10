package ndcam;

import android.graphics.ImageFormat;
import android.media.Image;
import android.media.ImageReader;
import android.support.test.runner.AndroidJUnit4;
import android.util.Log;

import junit.framework.Assert;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

import java.util.concurrent.Future;

@RunWith(AndroidJUnit4.class)
public class DeviceOperationTest extends CameraModelTest {

    ImageReader reader;

    @Before
    public void CreateImageReader() {
        // 1280 * 720
        // 1920 * 1080
        reader = ImageReader.newInstance(
                1920, 1080, ImageFormat.YUV_420_888,
                30);

        Assert.assertNotNull(reader);
    }

    @Test
    public void CloseWithoutStop() {
        Device[] devices = CameraModel.GetDevices();
        Assert.assertNotNull(devices);
        Device camera = null;
        for (Device device : devices)
            if (device.isFront())
                camera = device;

        // front camera
        Assert.assertNotNull(camera);
        camera.repeat(reader);
        // camera.stop();

        // !!! stop is missing !!!
    }
//
//    @Test
//    public void CaptureNone() {
//        Device[] devices = CameraModel.GetDevices();
//        Assert.assertNotNull(devices);
//        Device camera = null;
//        for (Device device : devices)
//            if (device.isFront())
//                camera = device;
//        // front camera
//        Assert.assertNotNull(camera);
//
//        camera.capture(reader);
//        camera.stop(); // stop just after capture request
//        {
//            Image image = reader.acquireLatestImage();
//            Assert.assertNull(image);
//        }
//    }
//
//    @Test
//    public void CaptureOnce() {
//        Device[] devices = CameraModel.GetDevices();
//        Assert.assertNotNull(devices);
//        Device camera = null;
//        for (Device device : devices)
//            if (device.isFront())
//                camera = device;
//        // front camera
//        Assert.assertNotNull(camera);
//
//        // start capture operation
//        camera.capture(reader);
//        {
//            Image image = reader.acquireNextImage();
//            Assert.assertNotNull(image);
//        }
//        camera.stop(); // stop after capture
//    }

    @Test
    public void CaptureRepeat() throws Exception{
        Device[] devices = CameraModel.GetDevices();
        Image i1 = null, i2 = null;
        Device camera = null;
        for (Device device : devices)
            if (device.isBack())
                camera = device;
        // rear camera
        Assert.assertNotNull(camera);

        // start repeating capture operation
        camera.repeat(reader);
        Image image = null;
        int i = 300;
        while(i-- > 0)
        {
            Thread.sleep(30);

            image = reader.acquireNextImage();
            if(image != null)
            {
                Log.i("ndk_camera", "format " + image.getFormat() +
                        " " + image.getWidth() +
                        "/" + image.getHeight());
                image.close();
            }
        }
        Assert.assertNotNull(image);

        camera.stop(); // stop after capture

        // Assert.assertTrue(i1.getTimestamp() < i2.getTimestamp());

    }
}
