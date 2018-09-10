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
    public void CaptureRepeat() {
        Image i1 = null, i2 = null;
        Device camera = null;
        for (Device device : devices)
            if (device.isBack())
                camera = device;
        // rear camera
        Assert.assertNotNull(camera);

        // start repeating capture operation
        camera.repeat(reader);
        {
            i1 = reader.acquireNextImage();
            Assert.assertNotNull(i1);

            i2 = reader.acquireNextImage();
            Assert.assertNotNull(i2);
        }
        camera.stop(); // stop after capture
        Assert.assertTrue(i1.getTimestamp() < i2.getTimestamp());

    }
}
