// SPDX-License-Identifier: GPL-2.0-or-later
package com.vrlightgunhub.rchase2;
import java.io.File;
import java.io.IOException;

/** File selection only; the existing GameFiles validator checks every payload. */
public final class GameStorage {
    private GameStorage() {}
    public static File ownedRoms(File owned) { return new File(owned,"roms"); }
    public static void prepare(File owned) throws IOException {
        for(File directory:new File[]{ownedRoms(owned),new File(owned,"models/guns")}) {
            if(!directory.isDirectory() && !directory.mkdirs())
                throw new IOException("Cannot prepare app folder: "+directory);
        }
    }
    public static File romDirectory(File owned,File downloads,boolean downloadsAllowed) {
        File local=ownedRoms(owned);
        if(new File(local,"rchase2.zip").isFile() || !downloadsAllowed || downloads==null) return local;
        return downloads;
    }
    public static File customGun(File owned,File downloads,boolean downloadsAllowed) {
        File local=new File(owned,"models/guns/custom.glb");
        if(local.isFile() || !downloadsAllowed || downloads==null) return local;
        return new File(downloads,"rail-chase-2-vr/models/guns/custom.glb");
    }
}
