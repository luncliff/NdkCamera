package dev.ndcam.testapp;

import androidx.appcompat.app.AppCompatActivity;

import android.graphics.ImageFormat;
import android.hardware.camera2.CameraCharacteristics;
import android.os.Bundle;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;

/**
 * we are in the same package!
 */
import dev.ndcam.*;

public class FullscreenActivity extends AppCompatActivity
        implements SurfaceHolder.Callback {

    SurfaceView surfaceView;
    Surface surface;
    ndcam.Device camera;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        ndcam.CameraModel.Init();

        setContentView(R.layout.activity_fullscreen);
        surfaceView = findViewById(R.id.fullscreen_surface);
        surfaceView.setSystemUiVisibility(
                View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN | View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION);

        SurfaceHolder holder = surfaceView.getHolder();
        holder.setFixedSize(1920, 1080);
        holder.setFormat(ImageFormat.YUV_420_888);
        holder.addCallback(this);
    }

    @Override
    public void surfaceCreated(SurfaceHolder holder) {
        // Get all devices in array form
        for(ndcam.Device device : ndcam.CameraModel.GetDevices())
            if(device.facing() == CameraCharacteristics.LENS_FACING_BACK)
                camera = device;
    }

    @Override
    public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
        // Make a repeating capture request with surface
        surface = holder.getSurface();
        camera.repeat(surface);
}

    @Override
    public void surfaceDestroyed(SurfaceHolder holder) {
        if(camera == null)
            return;
        camera.stopRepeat(); // No more capture ...
    }
}
