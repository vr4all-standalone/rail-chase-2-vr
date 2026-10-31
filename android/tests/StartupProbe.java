package com.vrlightgunhub.rchase2.startupcheck;

import android.app.Activity;
import android.app.Instrumentation;
import android.content.Context;
import android.content.Intent;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.os.Bundle;
import android.os.Environment;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.TextView;
import java.io.File;
import java.io.FileOutputStream;
import java.lang.reflect.InvocationTargetException;

/** Diagnostic instrumentation: use the installed app's UID and real validator. */
public final class StartupProbe extends Instrumentation {
    private boolean render;
    @Override public void onCreate(Bundle arguments) {super.onCreate(arguments);render=arguments!=null&&"true".equals(arguments.getString("render"));start();}
    private boolean playEnabled(View view) {
        if(view instanceof Button && ((Button)view).getText().toString().contains("Play"))return view.isEnabled();
        if(view instanceof ViewGroup)for(int i=0;i<((ViewGroup)view).getChildCount();i++)if(playEnabled(((ViewGroup)view).getChildAt(i)))return true;
        return false;
    }
    @Override public void onStart() {
        Bundle result=new Bundle();boolean ready=false;
        try {
            Context context=getTargetContext();File owned=context.getExternalFilesDir(null);
            if(owned==null)owned=context.getFilesDir();
            File directory=new File(owned,"roms"),game=new File(directory,"rchase2.zip");
            result.putString("owned",owned.getAbsolutePath());
            result.putString("directoryAccess","exists="+directory.exists()+", read="+directory.canRead()+", traverse="+directory.canExecute());
            result.putString("archiveAccess","exists="+game.exists()+", file="+game.isFile()+", read="+game.canRead()+", bytes="+game.length());
            result.putBoolean("downloadsPermission",Environment.isExternalStorageManager());
            Class<?> validator=Class.forName("com.vrlightgunhub.rchase2.GameFiles",true,context.getClassLoader());
            validator.getMethod("validate",File.class,File.class).invoke(null,game,new File(directory,"segabill.zip"));
            ready=true;
            if(render) {
                Intent intent=new Intent(Intent.ACTION_MAIN).setClassName(context.getPackageName(),"com.vrlightgunhub.rchase2.SetupActivity");
                intent.addCategory("com.oculus.intent.category.2D");
                intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK|Intent.FLAG_ACTIVITY_CLEAR_TASK);
                intent.putExtra("returned",true);intent.putExtra("vr4all.suppressAutoStart",true);
                ActivityMonitor monitor=addMonitor("com.vrlightgunhub.rchase2.SetupActivity",null,false);
                context.startActivity(intent);
                Activity activity=monitor.waitForActivityWithTimeout(5000);
                removeMonitor(monitor);
                if(activity==null)throw new IllegalStateException("Preparation Activity was not created within five seconds");
                // Exercise the production foreground-check callback without
                // requiring headset presence or launching immersive gameplay.
                runOnMainSync(()->callActivityOnResume(activity));
                final boolean[] enabled={false};
                for(int i=0;i<100&&!enabled[0];i++) {
                    runOnMainSync(()->enabled[0]=playEnabled(activity.getWindow().getDecorView()));
                    if(!enabled[0])Thread.sleep(100);
                }
                result.putBoolean("playEnabled",enabled[0]);
                if(!enabled[0])throw new IllegalStateException("Production preparation did not enable Play");
                final File image=new File(owned,"startup-panel.png");
                runOnMainSync(()->{
                    View content=((ViewGroup)activity.findViewById(android.R.id.content)).getChildAt(0);
                    float density=activity.getResources().getDisplayMetrics().density;
                    int width=Math.round(860*density),height=Math.round(640*density);
                    content.measure(View.MeasureSpec.makeMeasureSpec(width,View.MeasureSpec.EXACTLY),View.MeasureSpec.makeMeasureSpec(height,View.MeasureSpec.EXACTLY));
                    content.layout(0,0,width,height);
                    Bitmap bitmap=Bitmap.createBitmap(width,height,Bitmap.Config.ARGB_8888);content.draw(new Canvas(bitmap));
                    try(FileOutputStream file=new FileOutputStream(image)){bitmap.compress(Bitmap.CompressFormat.PNG,100,file);}
                    catch(Exception error){throw new RuntimeException(error);}
                    bitmap.recycle();
                    callActivityOnPause(activity);
                });
                result.putString("panelImage",image.getAbsolutePath());
            }
        } catch(Exception error) {
            ready=false;
            Throwable cause=error instanceof InvocationTargetException?error.getCause():error;
            result.putString("error",cause.toString());
        }
        result.putBoolean("ready",ready);finish(ready?Activity.RESULT_OK:Activity.RESULT_CANCELED,result);
    }
}
