package ndcam;

import android.graphics.ImageFormat;
import android.hardware.camera2.CameraCharacteristics;
import android.media.Image;
import android.media.ImageReader;
import android.support.test.runner.AndroidJUnit4;
import android.util.Log;

import junit.framework.Assert;

import org.junit.After;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.rules.Timeout;
import org.junit.runner.RunWith;

import java.nio.ByteBuffer;
import java.util.concurrent.Callable;
import java.util.concurrent.CompletableFuture;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.Future;
import java.util.concurrent.FutureTask;
import java.util.concurrent.TimeUnit;

/**
 * @author luncliff@gmail.com
 */
@RunWith(AndroidJUnit4.class)
public class DeviceOperationTest extends CameraModelTest {
    // Image reader doesn't have timeout. So we have to rule it for this test
    @Rule
    public Timeout timeout = new Timeout(60, TimeUnit.SECONDS);

    ImageReader reader;
    Device camera;

    @Before
    public void CreateImageReader() {
        // 1920 * 1080, 30 FPS, YCbCr 4:2:0(YUV_420_888)
        reader = ImageReader.newInstance( // create a new one
                1920, 1080, // width, height
                ImageFormat.YUV_420_888, // YCbCr
                30 // reserve some images
        );
        Assert.assertNotNull(reader);
    }

    @Before
    public void AcquireDevice() {
        CameraModel.Init();
        Device[] devices = CameraModel.GetDevices();
        Assert.assertNotNull(devices);
        camera = null;
        // get rear camera for test
        for (Device device : devices)
            if (device.facing() == CameraCharacteristics.LENS_FACING_BACK)
                camera = device;

        Assert.assertNotNull(camera);
    }

    @After
    public void CloseReaderAndDevice() throws Exception {
        Assert.assertNotNull(reader);
        reader.close();

        Thread.sleep(500);

        Assert.assertNotNull(camera);
        camera.close();

        // wait for camera framework to stop completely
        Thread.sleep(500);
    }

    /**
     * This scenario will generate error logs like the following one queueBuffer:
     * BufferQueue has been abandoned
     */
    @Test
    public void StopRepeatWithoutConsume() throws Exception {
        // start repeating capture operation
        camera.repeat(reader.getSurface());

        // Android Camera 2 API uses background thread.
        // Give some time to the framework.
        Thread.sleep(100);

        camera.stopRepeat();
    }

    @Test
    public void TryRepeatCapture() throws Exception {
        // start repeating capture operation
        camera.repeat(reader.getSurface());

        Thread.sleep(100);

        Image image = null;
        int i = 0, count = 0;
        while (i++ < 100) // try 100 capture (repeat mode)
        {
            // expect 30 FPS...
            Thread.sleep(30);

            // Fetch image 1 by 1.
            image = reader.acquireNextImage();
            if (image == null)
                continue;

            Log.v("ndk_camera", String.format("format %d width %d height %d timestamp %d", image.getFormat(),
                    image.getWidth(), image.getHeight(), image.getTimestamp()));
            image.close();
            count += 1;
        }
        camera.stopRepeat(); // stop after iteration

        Assert.assertNotNull(image); // ensure at least 1 image was acquired
        Assert.assertTrue(count > 1); // Repeating capture leads to multiple images
        Assert.assertTrue(i > count); // !!! some images might be dropped !!!
    }

    /**
     * This scenario will generate error logs like the following one queueBuffer:
     * BufferQueue has been abandoned
     */
    @Test
    public void StopCaptureWithoutConsume() throws Exception {
        // start repeating capture operation
        camera.capture(reader.getSurface());

        // Android Camera 2 API uses background thread.
        // Give some time to the framework.
        Thread.sleep(100);

        camera.stopCapture();
    }

    @Test
    public void TryCapture() throws Exception {
        // start capture operation
        camera.capture(reader.getSurface());
        Thread.sleep(100);

        // expect image in 2 sec
        Image image = WaitForImage(executorService, reader).get(2, TimeUnit.SECONDS);
        Assert.assertNotNull(image);

        camera.stopCapture(); // stop after capture

        Log.d("ndk_camera", String.format("format %d width %d height %d timestamp %d", image.getFormat(),
                image.getWidth(), image.getHeight(), image.getTimestamp()));

        image.close();
    }

    @Test
    public void ReadImageDataToByteArray() throws Exception {
        // start capture operation
        camera.capture(reader.getSurface());
        Thread.sleep(100);

        // expect image in 2 sec
        Image image = WaitForImage(executorService, reader).get(2, TimeUnit.SECONDS);
        Assert.assertNotNull(image);
        // stop after capture
        camera.stopCapture();

        Image.Plane[] planes = image.getPlanes();
        Assert.assertNotNull(planes);
        Assert.assertTrue(planes.length == 3);

        int planeSize = image.getWidth() * image.getHeight();
        int channel = planes.length;

        Assert.assertTrue(image.getFormat() == ImageFormat.YUV_420_888);
        //
        // Checked with monospace fonts.
        // D2Coding, Source Code Pro, Ubuntu Mono
        //
        // ◯ : U+25EF // y
        // △ : U+25B3 // cb
        // ▢ : U+25A2 // cr
        //

        // y ◯ ◯ ◯ ◯
        // ◯ ◯ ◯ ◯
        // ◯ ◯ ◯ ◯
        // ◯ ◯ ◯ ◯
        ByteBuffer yView = planes[0].getBuffer();
        // `ccView1` & `ccView2` shared *same memory* but their range is different...
        // cb/cr △ ▢ △ ▢
        // △ ▢ △
        ByteBuffer ccView1 = planes[1].getBuffer();
        // cb/cr ▢ △ ▢
        // △ ▢ △ ▢
        ByteBuffer ccView2 = planes[2].getBuffer();

        //
        // ◯ : U+25EF // empty
        // △ : U+25B3 // filling
        // ▢ : U+25A2 // filled
        //
        byte[] yccData = new byte[planeSize * channel / 2];

        // y △ △ △ △
        // △ △ △ △
        // △ △ △ △
        // △ △ △ △
        // cb/cr ◯ ◯ ◯ ◯
        // ◯ ◯ ◯ ◯
        yView.get(yccData, 0, planeSize);

        // y ▢ ▢ ▢ ▢
        // ▢ ▢ ▢ ▢
        // ▢ ▢ ▢ ▢
        // ▢ ▢ ▢ ▢
        // cb/cr △ ◯ ◯ ◯
        // ◯ ◯ ◯ ◯
        ccView1.get(yccData, planeSize, 1);

        // y ▢ ▢ ▢ ▢
        // ▢ ▢ ▢ ▢
        // ▢ ▢ ▢ ▢
        // ▢ ▢ ▢ ▢
        // cb/cr ▢ △ △ △
        // △ △ △ △
        ccView2.get(yccData, planeSize + 1, ccView2.remaining());

        image.close();
    }
}
