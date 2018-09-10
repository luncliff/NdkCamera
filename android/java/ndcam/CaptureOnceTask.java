package ndcam;

import android.media.Image;

import java.util.concurrent.ExecutionException;
import java.util.concurrent.Future;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.TimeoutException;

class CaptureOnceTask implements Future<Image> {
    int i = 0;

    void setResultImage(Image image) {

    }

    @Override
    public boolean cancel(boolean b) {
        return false;
    }

    @Override
    public boolean isCancelled() {
        return false;
    }

    @Override
    public boolean isDone() {
        i += 1;
        return i > 100;
    }

    @Override
    public Image get() throws InterruptedException, ExecutionException {
        return null;
    }

    @Override
    public Image get(long l, TimeUnit timeUnit) throws InterruptedException, ExecutionException, TimeoutException {
        return null;
    }
}
