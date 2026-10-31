// SPDX-License-Identifier: GPL-2.0-or-later
package com.vrlightgunhub.rchase2;
import org.libsdl.app.SDLActivity;
import android.os.Environment;
import android.content.Intent;
import android.os.Build;
import android.Manifest;
import android.content.pm.PackageManager;
import java.io.File;

public final class GameActivity extends SDLActivity {
    @Override protected void onNativeMainReturned() {
        Intent setup=new Intent(Intent.ACTION_MAIN).setClass(this,SetupActivity.class).putExtra("returned",true);
        setup.addCategory("com.oculus.intent.category.2D");
        setup.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK|Intent.FLAG_ACTIVITY_CLEAR_TASK);
        startActivity(setup); finishAndRemoveTask();
    }
    @Override protected String[] getLibraries() { return new String[]{"SDL2","rchase2_audio","main"}; }
    @Override protected String[] getArguments() {
        FoveationExperiment.configure(getIntent(),(name,value)->nativeSetenv(name,value));
        String root=getFilesDir().getAbsolutePath();
        File download=Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_DOWNLOADS);
        File external=getExternalFilesDir(null);
        File owned=external!=null?external:getFilesDir();
        boolean downloadsAllowed=Build.VERSION.SDK_INT>=30?Environment.isExternalStorageManager():
            checkSelfPermission(Manifest.permission.READ_EXTERNAL_STORAGE)==PackageManager.PERMISSION_GRANTED;
        File games=GameStorage.romDirectory(owned,download,downloadsAllowed);
        nativeSetenv("RCHASE2_VR","1");
        nativeSetenv("RCHASE2_FAST_RASTER","1");
        nativeSetenv("RCHASE2_UART_COALESCE","1");
        nativeSetenv("RCHASE2_AUDIO_STRETCH","1");
        nativeSetenv("RCHASE2_AUDIO_LIBRARY",getApplicationInfo().nativeLibraryDir+"/librchase2_audio.so");
        nativeSetenv("RCHASE2_XR_LIBRARY",getApplicationInfo().nativeLibraryDir+"/librchase2_xr.so");
        nativeSetenv("RCHASE2_ASSETS",root+"/assets");
        nativeSetenv("RCHASE2_SETTINGS",root+"/settings.ini");
        File logs=getExternalFilesDir(null);
        nativeSetenv("RCHASE2_XR_LOG",new File(logs!=null?logs:getFilesDir(),"rchase2-xr.log").getAbsolutePath());
        nativeSetenv("RCHASE2_AUDIO_LOG",new File(logs!=null?logs:getFilesDir(),"rchase2-audio.log").getAbsolutePath());
        nativeSetenv("RCHASE2_CUSTOM_GUN",GameStorage.customGun(owned,download,downloadsAllowed).getAbsolutePath());
        return new String[]{"rchase2","-rompath",games.getAbsolutePath(),
            "-nvram_directory",root+"/nvram","-cfg_directory",root+"/cfg","-state_directory",root+"/state","-snapshot_directory",root+"/snap",
            "-video","none","-sound","sdl","-skip_gameinfo","-noreadconfig","-nomouse","-numprocessors","2","-throttle"};
    }
}
