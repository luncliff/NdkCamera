package ndcam;

import android.graphics.ImageFormat;
import android.media.Image;
import android.media.ImageReader;
import android.support.test.runner.AndroidJUnit4;

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
        reader = ImageReader.newInstance(1280, 720, ImageFormat.YUV_420_888, 1);

        Assert.assertNotNull(reader);
    }

    @After
    public void CloseImageReader() {
        reader.close();
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
        camera.capture(reader);
        // camera.stop();

        // !!! stop is missing !!!
    }

    @Test
    public void CaptureNone() {
        Device[] devices = CameraModel.GetDevices();
        Assert.assertNotNull(devices);
        Device camera = null;
        for (Device device : devices)
            if (device.isFront())
                camera = device;
        // front camera
        Assert.assertNotNull(camera);

        camera.capture(reader);
        camera.stop(); // stop just after capture request
        {
            Image image = reader.acquireLatestImage();
            Assert.assertNull(image);
        }
    }

    @Test
    public void CaptureOnce() {
        Device[] devices = CameraModel.GetDevices();
        Assert.assertNotNull(devices);
        Device camera = null;
        for (Device device : devices)
            if (device.isFront())
                camera = device;
        // front camera
        Assert.assertNotNull(camera);

        // start capture operation
        camera.capture(reader);
        {
            Image image = reader.acquireNextImage();
            Assert.assertNotNull(image);
        }
        camera.stop(); // stop after capture
    }

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
        while (i1 == null)
            i1 = reader.acquireNextImage();

        Thread.sleep(10000);
        // int count = 1000;
        // while(count-- > 0)
        // {
        // i1 = reader.acquireNextImage();
        //
        // i2 = reader.acquireNextImage();
        //
        // }
        camera.stop(); // stop after capture

        Assert.assertNotNull(i1);
        Assert.assertNotNull(i2);

        Assert.assertTrue(i1.getTimestamp() < i2.getTimestamp());

    }
}
