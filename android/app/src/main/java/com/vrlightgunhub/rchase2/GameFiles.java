// SPDX-License-Identifier: GPL-2.0-or-later
package com.vrlightgunhub.rchase2;
import java.io.*;
import java.security.*;
import java.util.*;
import java.util.zip.*;

/** Validate original packed ROMs in place; never unpack game data into app state. */
public final class GameFiles {
    static final class Rom {
        final String name, sha1; final long size, crc;
        Rom(String name, long size, long crc, String sha1) {
            this.name=name; this.size=size; this.crc=crc; this.sha1=sha1;
        }
    }
    private static final Rom[] REQUIRED = {
        new Rom("epr-18045a.15", 524288, 0xbfca0314L, "9eb0f2cdab8c10fda9edc0ddc439263af3903cdc"),
        new Rom("epr-18046a.16", 524288, 0x0b8d3074L, "fee8436399fb97ad5b8357b81e69bd5c27af1dde"),
        new Rom("epr-18074a.13", 524288, 0xca4b58dfL, "d41cb8efd9fd65eea9e7aefadebfd0a27ef145fb"),
        new Rom("epr-18075a.14", 524288, 0xb82672e4L, "519fdb5a978b6e82989b9841c6b59819f0d417cb"),
        new Rom("mpr-18037.11", 2097152, 0xdea8f896L, "8eb45e46bd14a2ffbdaac47d381a1ea9b9a03ca2"),
        new Rom("mpr-18038.12", 2097152, 0x441f7709L, "cbfa687839b6cad6a5ace45b44b95c45e4cfab0d"),
        new Rom("mpr-18039.9", 2097152, 0xb98c6f06L, "dd1ff9c682778de1c6c09e7a5cbc95a8149488c4"),
        new Rom("mpr-18040.10", 2097152, 0x0d872667L, "33e56486ec6b953341552b6bc21dc66f6f8aaf74"),
        new Rom("mpr-18041.7", 2097152, 0xe511ab0aL, "c6ea14b3bdefdc59603bd2fc152ac0421fae4d6f"),
        new Rom("mpr-18042.8", 2097152, 0xe9a04159L, "0204ba86af2707bc9e277cac68dd9ef759189c23"),
        new Rom("mpr-18043.5", 2097152, 0xff84dfd6L, "82833bf4cb1f367aea5fec6cffb7023cbbd3c8cb"),
        new Rom("mpr-18044.6", 2097152, 0xab9b406dL, "62e95ceea6f71eedbebae59e188aac03e6129e62"),
        new Rom("mpr-18031.17", 2097152, 0x25d0deaeL, "2d0339dd7eeb2625f78e2fbe4ebdc976967175a4"),
        new Rom("mpr-18032.21", 2097152, 0xdbae35c2L, "9510104975192a0ef1750251636daff7f089feb9"),
        new Rom("mpr-18033.18", 2097152, 0x1e75946cL, "7dee991f0c43de9bfe17ae44767f65f12e83c811"),
        new Rom("mpr-18034.22", 2097152, 0x215235adL, "48227544209412fca3035e85a00d33ea654dc7b5"),
        new Rom("mpr-18035.27", 2097152, 0x4423f66eL, "c1f8dda4781dea00bd97dbf9ecfbb626dadd2c35"),
        new Rom("mpr-18036.25", 2097152, 0x69221cf5L, "e39644a08aa631dbdcfc7c0dc356e73f6a4412a9"),
        new Rom("epr-18047.31", 524288, 0x4c31d459L, "424d5e5a7787d0d4c68aa919ba7d575babfd1ce0"),
        new Rom("mpr-18029.32", 2097152, 0xf6804150L, "ef40c11008c75d04159772ad30f02cdb8c5464f3"),
        new Rom("mpr-18030.34", 2097152, 0x1167615dL, "bae0060aec3c15f08342f11df665c05c5703523d"),
        new Rom("epr-17895.ic8", 32768, 0x8fd7003dL, "b8b16e20e3ed07326330ba335ea1e701cc0bec17"),
        new Rom("epr-18022.ic2", 65536, 0x0ca70f80L, "edf5ade72d9fa2f4d5f83f9f89e6cecfadd77f56"),
    };
    private static final long MAX_ARCHIVE_BYTES=128L*1024*1024;
    private static final int MAX_MEMBERS=4096;
    private GameFiles() {}

    public static void validate(File game, File billboard) throws IOException {
        if(game==null || !game.isFile()) throw new IOException("Missing game archive: "+(game==null?"rchase2.zip":game.getAbsolutePath()));
        File[] paths=billboard!=null && billboard.isFile()?new File[]{game,billboard}:new File[]{game};
        validateArchives(paths, REQUIRED);
    }

    static void validateArchives(File[] paths, Rom[] requirements) throws IOException {
        Map<String,Rom> wanted=new HashMap<>();
        for(Rom rom:requirements) wanted.put(rom.name,rom);
        Set<String> found=new HashSet<>();
        for(File path:paths) {
            if(!path.isFile() || !path.canRead()) throw new IOException("Cannot read "+path.getName()+". Allow game file access and retry.");
            if(path.length()>MAX_ARCHIVE_BYTES) throw new IOException("ROM archive is too large: "+path.getName());
            try(ZipFile zip=new ZipFile(path)) {
                if(zip.size()>MAX_MEMBERS) throw new IOException("Too many ZIP members in "+path.getName());
                Set<String> seen=new HashSet<>();
                Enumeration<? extends ZipEntry> entries=zip.entries();
                while(entries.hasMoreElements()) {
                    ZipEntry entry=entries.nextElement(); String name=entry.getName();
                    if(name.isEmpty() || name.equals(".") || name.equals("..") || name.indexOf('/')>=0 || name.indexOf('\\')>=0 || name.indexOf(':')>=0 || name.indexOf('\0')>=0)
                        throw new IOException("Unsafe ZIP member: "+name);
                    String key=name.toLowerCase(Locale.ROOT);
                    if(!seen.add(key)) throw new IOException("Duplicate ZIP member: "+name);
                    Rom rom=wanted.get(key);
                    if(rom==null) continue;
                    if(entry.isDirectory() || entry.getSize()!=rom.size) throw new IOException("Wrong ROM size: "+rom.name);
                    MessageDigest sha;
                    try { sha=MessageDigest.getInstance("SHA-1"); }
                    catch(NoSuchAlgorithmException error) { throw new IOException("SHA-1 validation is unavailable",error); }
                    CRC32 crc=new CRC32(); byte[] buffer=new byte[16384]; long count=0;
                    try(InputStream in=zip.getInputStream(entry)) {
                        int bytes;
                        while((bytes=in.read(buffer,0,(int)Math.min(buffer.length,rom.size-count+1)))!=-1) {
                            if(bytes==0) continue;
                            count+=bytes;
                            if(count>rom.size) throw new IOException("Oversized ROM payload: "+rom.name);
                            sha.update(buffer,0,bytes); crc.update(buffer,0,bytes);
                        }
                    }
                    if(count!=rom.size || crc.getValue()!=rom.crc || !hex(sha.digest()).equals(rom.sha1)) throw new IOException("ROM checksum mismatch: "+rom.name+". Copy a complete, unmodified archive.");
                    found.add(key);
                }
            } catch(ZipException error) {
                throw new IOException("Cannot read "+path.getName()+": invalid, corrupt or encrypted ZIP ("+error.getMessage()+").",error);
            }
        }
        for(Rom rom:requirements) if(!found.contains(rom.name)) {
            String source=rom.name.equals("epr-18022.ic2")?"segabill.zip (billboard device firmware)":"rchase2.zip (including Z80 I/O board firmware)";
            throw new IOException("Missing ROM: "+rom.name+". Required in "+source+" or a complete 23-payload rchase2.zip.");
        }
    }
    private static String hex(byte[] bytes) {
        char[] symbols="0123456789abcdef".toCharArray(); char[] result=new char[bytes.length*2];
        for(int i=0;i<bytes.length;i++) { result[i*2]=symbols[(bytes[i]&255)>>>4]; result[i*2+1]=symbols[bytes[i]&15]; }
        return new String(result);
    }
}
