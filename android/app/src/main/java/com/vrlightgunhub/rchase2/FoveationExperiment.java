// SPDX-License-Identifier: GPL-2.0-or-later
package com.vrlightgunhub.rchase2;
import android.content.Intent;
import java.util.function.BiConsumer;

/** Medium Quest default plus launch-only developer controls; never saved. */
final class FoveationExperiment {
    private static final String EXTRA="rchase2_ffr";
    private FoveationExperiment() {}
    static String mode(Intent intent) {
        String value=null;
        try {
            if(intent==null||!intent.hasExtra(EXTRA))return "medium";
            value=intent.getStringExtra(EXTRA);
        }
        catch(ClassCastException malformed) { return "raw"; }
        // Android returns null for a present non-String extra as well as null.
        // Both remain conservative Raw; only an absent extra selects Medium.
        return "none".equals(value)||"medium".equals(value)||"high".equals(value)||"cycle".equals(value)||"cycle-high".equals(value)?value:"raw";
    }
    static void forward(Intent incoming,Intent outgoing) { outgoing.putExtra(EXTRA,mode(incoming)); }
    static void configure(Intent intent,BiConsumer<String,String> environment) {
        // Always set the ordinary Medium default too: never inherit a prior cycle.
        environment.accept("RCHASE2_FFR_EXPERIMENT",mode(intent));
    }
}
