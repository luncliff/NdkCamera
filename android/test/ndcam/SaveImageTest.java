package ndcam;

import android.Manifest;
import android.content.Context;
import android.content.res.AssetManager;
import android.graphics.ImageFormat;
import android.hardware.camera2.CameraCharacteristics;
import android.media.Image;
import android.media.ImageReader;
import android.os.Environment;
import android.provider.ContactsContract;
import android.support.test.InstrumentationRegistry;
import android.support.test.rule.GrantPermissionRule;
import android.support.test.runner.AndroidJUnit4;
import android.util.Log;

import junit.framework.Assert;

import org.junit.After;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

import java.io.DataOutputStream;
import java.io.File;
import java.io.FileOutputStream;
import java.io.FileWriter;
import java.io.OutputStream;
import java.io.OutputStreamWriter;
import java.io.Writer;
import java.nio.ByteBuffer;
import java.util.concurrent.Callable;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.Future;
import java.util.concurrent.TimeUnit;

import static org.junit.Assert.*;

/**
 * @author luncliff@gmail.com
 */
@RunWith(AndroidJUnit4.class)
public class SaveImageTest extends TestBackbone {

    File publicDir;

    @Before
    public void GetPublicDirectory()
     {
        // Get the directory to save captured images
        publicDir = Environment.getExternalStoragePublicDirectory(
                Environment.DIRECTORY_DOWNLOADS);
        Assert.assertNotNull(publicDir);

        publicDir = new File(publicDir, "NdCamTestDir");
         Assert.assertNotNull(publicDir);

        if(publicDir.exists() == false)
            publicDir.mkdir();
     }

    @Before
    public void CheckStorageWritable()
    {
        String state = Environment.getExternalStorageState();
        Assert.assertTrue( Environment.MEDIA_MOUNTED.equals(state) );
    }

    @Test
    public void CreateFileToSave() throws Exception
    {
        File file = new File(publicDir, "hell_world.txt");
        if(file.exists())
            Assert.assertTrue(file.delete());

        Assert.assertTrue(file.createNewFile());
        Assert.assertTrue(file.canWrite());

        Log.i("CreateFileToSave", file.getPath());

        OutputStream stream = new FileOutputStream(file, false);
        Assert.assertNotNull(stream);

        Writer writer = new OutputStreamWriter(stream);
        Assert.assertNotNull(writer);
        writer.write("Hell-World!");
        writer.flush();

        writer.close();
        stream.close();
    }

    @Test
    public void SameImageToFile() throws Exception
    {
        File file = new File(publicDir, "image.ycc");
        if(file.exists())
            Assert.assertTrue(file.delete());

        Assert.assertTrue(file.createNewFile());
        Assert.assertTrue(file.canWrite());

        Log.i("SameImageToFile", file.getPath());

        // camera image reader
        ImageReader imageReader =ImageReader.newInstance(
                640 , 480, ImageFormat.YUV_420_888, 2);
        Assert.assertNotNull(imageReader);

        // Acquire camera
        CameraModel.Init();
        Device camera = null;
        for (Device device : CameraModel.GetDevices()) {
            if(device.facing() == CameraCharacteristics.LENS_FACING_BACK)
                camera = device;
        }
        Assert.assertNotNull(camera);

        // start capture operation
        camera.capture(imageReader.getSurface());
        Thread.sleep(100);

        Image image = WaitForImage(executorService, imageReader).get(2, TimeUnit.SECONDS); // expect image in 2 sec
        Assert.assertNotNull(image);
        // stop after capture
        camera.stopCapture();

        Image.Plane[] planes = image.getPlanes();
        Assert.assertNotNull(planes);
        Assert.assertTrue(planes.length == 3);

        int planeSize = image.getWidth() * image.getHeight();
        int channel = planes.length;

        Assert.assertTrue(
                image.getFormat() == ImageFormat.YUV_420_888);

        ByteBuffer yView = planes[0].getBuffer();
        ByteBuffer ccView1 = planes[1].getBuffer();
        ByteBuffer ccView2 = planes[2].getBuffer();

        byte[] yccData= new byte[planeSize * channel / 2];
        yView.get(yccData, 0, planeSize);
        ccView1 .get(yccData, planeSize, 1);
        ccView2.get(yccData,
                planeSize+1, ccView2.remaining());

        image.close();

        DataOutputStream stream = new DataOutputStream(new FileOutputStream(file));
        Assert.assertNotNull(stream);

        stream.write(yccData);
        stream.close();
        Thread.sleep(100);

    }

}