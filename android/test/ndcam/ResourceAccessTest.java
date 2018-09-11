package ndcam;

import android.content.Context;
import android.content.res.AssetManager;
import android.support.test.InstrumentationRegistry;
import android.support.test.runner.AndroidJUnit4;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

import static org.junit.Assert.*;

/**
 * @author luncliff@gmail.com
 */
@RunWith(AndroidJUnit4.class)
public class ResourceAccessTest {

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
    public void PackageName() {
        assertEquals("dev.ndcam.test", context.getPackageName());
    }

}