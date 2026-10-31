package com.vrlightgunhub.rchase2;
import java.io.File;
public final class GameFilesHarness {
    public static void main(String[] args) throws Exception {
        if(args.length==2 && args[0].equals("--missing")) {
            GameFiles.validate(new File(args[1]),null);return;
        }
        File[] paths = new File[args.length];
        for (int i=0; i<args.length; i++) paths[i] = new File(args[i]);
        GameFiles.validateArchives(paths, new GameFiles.Rom[] {
            new GameFiles.Rom("game.rom",3,0x352441c2L,"a9993e364706816aba3e25717850c26c9cd0d89d"),
            new GameFiles.Rom("board.rom",3,0x352441c2L,"a9993e364706816aba3e25717850c26c9cd0d89d")
        });
    }
}
