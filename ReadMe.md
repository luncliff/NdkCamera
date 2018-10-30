# NDK Camera

> If there is an issue with this library, please mail to luncliff@gmail.com

Simplified [Android Camera 2 API](https://www.youtube.com/watch?v=Bi4QjMfSOE0).  
Available for both Java/JNI.

  - API level: 24+
  - NDK

### Reference

 - Personal Template Project: https://github.com/luncliff/Muffin
 - [API Reference](https://developer.android.com/ndk/reference/group/camera)
 - [Android Camera Overview](https://source.android.com/devices/camera)
    - Camera HAL3: https://source.android.com/devices/camera/camera3
    - HAL Subsystem: https://source.android.com/devices/camera/camera3_requests_hal
    - Multi-Camera Support: https://source.android.com/devices/camera/multi-camera
    - Version Support: https://source.android.com/devices/camera/versioning
 - Android Media
    - https://source.android.com/devices/media/framework-hardening

## How to

### Build

For **Windows** environment, latest [Android Studio](https://developer.android.com/studio/) is recommended.  
For **Linux/MacOS**, [Gradle 4.10.2](https://gradle.org/) will be enough.

```console
$ git clone https://github.com/luncliff/NdkCamera
$ cd ./NdkCamera
$ gradle assemble               # Build: libndk_camera.so & NdkCamera.aar
```

### Test

Connect your device and run the test with Gradle.
Please reference the [test codes](./android/test/ndcam/).

```console
$ gradle connectedAndroidTest   # Run test
```

### Use

The following code shows working with `SurfaceView` class.

```java
package dev.example;

// ...
import dev.ndcam.*;

// Expect we already have a camera permission
public class SurfaceDisplay
    implements  SurfaceHolder.Callback
{
    SurfaceView surfaceView;
    Surface surface;
    ndcam.Device camera;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        // ...

        // Load/Init library
        ndcam.CameraModel.Init();

        surfaceView = findViewById(R.id.surfaceView);
        SurfaceHolder holder = surfaceView.getHolder();
        holder.setFixedSize(1920, 1080);
        holder.setFormat(ImageFormat.YUV_420_888);
        holder.addCallback(this);
    }
    @Override
    public void surfaceCreated(SurfaceHolder surfaceHolder) {
        // Get all devices in array form
        for(ndcam.Device device : ndcam.CameraModel.GetDevices())
            if(device.facing() == CameraCharacteristics.LENS_FACING_BACK)
                camera = device;
    }
    @Override
    public void surfaceChanged(SurfaceHolder surfaceHolder,
                               int format, int width, int height) {
        // Make a repeating caputre request with surface
        surface = surfaceHolder.getSurface();
        camera.repeat(surface);
    }
    @Override
    public void surfaceDestroyed(SurfaceHolder surfaceHolder) {
        // No more capture
        if(camera != null)
            camera.stopRepeat();
    }
}
```
