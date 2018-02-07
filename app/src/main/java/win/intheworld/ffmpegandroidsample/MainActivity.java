package win.intheworld.ffmpegandroidsample;

import android.app.Activity;
import android.os.Environment;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.text.method.ScrollingMovementMethod;
import android.util.Log;
import android.view.Surface;
import android.view.SurfaceView;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;
import android.widget.TextView;


public class MainActivity extends Activity {

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        final TextView infoText = (TextView) findViewById(R.id.text_libinfo);
        infoText.setMovementMethod(ScrollingMovementMethod.getInstance());
        Button button = (Button) this.findViewById(R.id.button_avcodec);
        button.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                infoText.setText(avcodecinfo());
            }
        });

        final EditText et_fileName = (EditText) findViewById(R.id.et_videoName);

        Button buttonDecode = (Button) this.findViewById(R.id.button_decode);
        buttonDecode.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                String folderurl= Environment.getExternalStorageDirectory().getPath();

                String urltext_input= et_fileName.getText().toString();
                final String inputurl=folderurl+"/"+urltext_input;

                String urltext_output= urltext_input.concat(".YUV");
                final String outputurl=folderurl+"/"+urltext_output;

                Log.i("inputurl",inputurl);
                Log.i("outputurl",outputurl);
                new Thread(new Runnable() {
                    @Override
                    public void run() {
                        decode(inputurl,outputurl);
                    }
                }).run();

            }
        });

        final SurfaceView surfaceView = (SurfaceView) findViewById(R.id.surfaceView);
        Button btn_play = (Button) findViewById(R.id.btn_play);
        btn_play.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                String folderurl= Environment.getExternalStorageDirectory().getPath();
                String urltext_input= et_fileName.getText().toString();
                final String inputurl=folderurl+"/"+urltext_input;

                new Thread(new Runnable() {
                    @Override
                    public void run() {
                        render(inputurl, surfaceView.getHolder().getSurface());
                    }
                }).run();

            }
        });
    }


    //JNI
    public native String avcodecinfo();

    public native int decode(String inputUrl, String outputUrl);

    public native void render(String inputUrl, Surface surface);

    static{
        System.loadLibrary("avutil-55");
        System.loadLibrary("swresample-2");
        System.loadLibrary("avcodec-57");
        System.loadLibrary("avformat-57");
        System.loadLibrary("swscale-4");
        System.loadLibrary("avfilter-6");
        System.loadLibrary("native-lib");
    }

}
