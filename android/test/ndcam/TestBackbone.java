package ndcam;

import android.Manifest;
import android.content.Context;
import android.content.res.AssetManager;
import android.hardware.camera2.CameraCharacteristics;
import android.media.Image;
import android.media.ImageReader;
import android.os.Environment;
import android.support.test.InstrumentationRegistry;
import android.support.test.rule.GrantPermissionRule;
import android.support.test.runner.AndroidJUnit4;

import junit.framework.Assert;

import org.junit.After;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

import java.util.concurrent.Callable;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.Future;

import static org.junit.Assert.assertNotNull;

/**
 * @author luncliff@gmail.com
 */
@RunWith(AndroidJUnit4.class)
public class TestBackbone {
    @Rule
    public GrantPermissionRule useCamera = GrantPermissionRule.grant(Manifest.permission.CAMERA);
    @Rule
    public GrantPermissionRule writeStorage = GrantPermissionRule.grant(Manifest.permission.WRITE_EXTERNAL_STORAGE);
    @Rule
    public GrantPermissionRule readStorage = GrantPermissionRule.grant(Manifest.permission.READ_EXTERNAL_STORAGE);

    protected ExecutorService executorService;
    protected Context context;

    @Before
    public void CreateExecutorService() {
        executorService = Executors.newFixedThreadPool(1);
        Assert.assertNotNull(executorService);
    }

    @Before
    public void GetContext() {
        context = InstrumentationRegistry.getTargetContext();
        assertNotNull(context);
    }

    @Test
    public void AcquireAssetManager() {
        AssetManager assets = context.getAssets();
        assertNotNull(assets);
        assets.close();
    }

    protected static Future<Image> WaitForImage(ExecutorService service, final ImageReader imageReader) {
        return service.submit(new Callable<Image>() {
            @Override
            public Image call() throws Exception {
                Image image = null;

                // try 50 times
                int repeatCount = 0;
                while (repeatCount < 50) {
                    // Give some time to Camera Framework in background
                    Thread.sleep(30);

                    // acquire image...
                    image = imageReader.acquireNextImage();
                    if (image != null)
                        break; // acquired !

                    repeatCount += 1;
                }
                return image;
            }
        });
    }

}
