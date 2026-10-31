package com.vrlightgunhub.rchase2;
import java.io.File;
import java.nio.file.Files;

public final class GameStorageHarness {
    private static void check(boolean value) { if(!value) throw new AssertionError(); }
    public static void main(String[] args) throws Exception {
        File owned=new File(args[0],"owned"), downloads=new File(args[0],"Downloads");
        GameStorage.prepare(owned); downloads.mkdirs();
        File ownRoms=new File(owned,"roms");
        check(ownRoms.isDirectory());
        check(GameStorage.romDirectory(owned,downloads,false).equals(ownRoms));
        Files.write(new File(downloads,"rchase2.zip").toPath(),new byte[]{1});
        check(GameStorage.romDirectory(owned,downloads,false).equals(ownRoms));
        check(GameStorage.romDirectory(owned,downloads,true).equals(downloads));
        Files.write(new File(ownRoms,"rchase2.zip").toPath(),new byte[]{2});
        check(GameStorage.romDirectory(owned,downloads,true).equals(ownRoms));
        check(GameStorage.romDirectory(owned,downloads,false).equals(ownRoms));
        File ownGun=new File(owned,"models/guns/custom.glb");
        File downloadGun=new File(downloads,"rail-chase-2-vr/models/guns/custom.glb");
        downloadGun.getParentFile().mkdirs(); Files.write(downloadGun.toPath(),new byte[]{3});
        check(GameStorage.customGun(owned,downloads,false).equals(ownGun));
        check(GameStorage.customGun(owned,downloads,true).equals(downloadGun));
        Files.write(ownGun.toPath(),new byte[]{4});
        check(GameStorage.customGun(owned,downloads,true).equals(ownGun));
        check(GameStorage.romDirectory(owned,null,true).equals(ownRoms));
        System.out.println("App-owned media works without broad access; Downloads requires a grant; owned files win.");
    }
}
