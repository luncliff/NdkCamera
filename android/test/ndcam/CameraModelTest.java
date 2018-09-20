package ndcam;

import android.Manifest;
import android.hardware.camera2.CameraCharacteristics;
import android.support.test.rule.GrantPermissionRule;
import android.support.test.runner.AndroidJUnit4;

import junit.framework.Assert;

import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

/**
 * @author luncliff@gmail.com
 */
@RunWith(AndroidJUnit4.class)
public class CameraModelTest extends TestBackbone {

    @Before
    public void TryInit() {
        CameraModel.Init();

        Device[] devices = CameraModel.GetDevices();
        Assert.assertNotNull(devices);
        Assert.assertTrue(devices.length > 0);
    }

    @Test
    public void AcquireDevices() {
        for (Device device : CameraModel.GetDevices()) {
            Assert.assertTrue(device.id != -1);
            Assert.assertTrue(device.facing() == CameraCharacteristics.LENS_FACING_FRONT
                    || device.facing() == CameraCharacteristics.LENS_FACING_BACK
                    || device.facing() == CameraCharacteristics.LENS_FACING_EXTERNAL);
        }
    }

}
