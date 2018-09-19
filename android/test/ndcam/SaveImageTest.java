package ndcam;

import android.Manifest;
import android.content.Context;
import android.content.res.AssetManager;
import android.media.Image;
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

import java.io.File;
import java.io.FileOutputStream;
import java.io.OutputStream;
import java.io.OutputStreamWriter;
import java.io.Writer;

import static org.junit.Assert.*;

/**
 * @author luncliff@gmail.com
 */
@RunWith(AndroidJUnit4.class)
public class SaveImageTest {

    @Rule
    public GrantPermissionRule writeStorage = GrantPermissionRule.grant(
            Manifest.permission.WRITE_EXTERNAL_STORAGE);
    @Rule
    public GrantPermissionRule readStorage = GrantPermissionRule.grant(
            Manifest.permission.READ_EXTERNAL_STORAGE);


    Context context;
    AssetManager assetManager;

    @Before
    public void GetContext() {
        context = InstrumentationRegistry.getTargetContext();
        assertNotNull(context);

        assetManager = context.getAssets();
        assertNotNull(assetManager);
    }

    @After
    public void CloseManager() {
        assetManager.close();
    }

    @Test
    public void CheckStorageWritable()
    {
        String state = Environment.getExternalStorageState();

        Assert.assertTrue( Environment.MEDIA_MOUNTED.equals(state) );
    }

    @Test
    public void GetPublicDirectory() throws Exception
    {
        // Get the directory to save captured images
        File dir = Environment.getExternalStoragePublicDirectory(
                Environment.DIRECTORY_DOWNLOADS);
        Assert.assertNotNull(dir);
        Log.i("SaveImageTest", dir.getPath());
    }

    @Test
    public void CreateFileToSave() throws Exception
    {
        // Get the directory to save captured images
        File dir = Environment.getExternalStoragePublicDirectory(
                Environment.DIRECTORY_DCIM);
        Assert.assertNotNull(dir);

        // Make sub-directory for safety
        dir = new File(dir, "unknown_files");
        // make directory if not exists
        if(dir.exists() == false)
            Assert.assertTrue(dir.mkdir());

        File file = new File(dir, "some_file.txt");
        if(file.exists())
            Assert.assertTrue(file.delete());

        Assert.assertTrue(file.createNewFile());
        Assert.assertTrue(file.canWrite());

        Log.i("SaveImageTest", file.getPath());

        OutputStream stream = new FileOutputStream(file, false);
        Assert.assertNotNull(stream);

        Writer writer = new OutputStreamWriter(stream);
        Assert.assertNotNull(writer);
        writer.write("HellWorld!");
        writer.flush();

        writer.close();
        stream.close();
    }

//    @Test
//    public void SameImageToFile() throws Exception
//    {
//        Image image = null;
//        Assert.assertNotNull(image);
//
//    }
}