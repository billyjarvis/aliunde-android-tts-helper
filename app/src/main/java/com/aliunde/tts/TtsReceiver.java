package com.aliunde.tts;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;

public final class TtsReceiver extends BroadcastReceiver {
    @Override
    public void onReceive(Context context, Intent intent) {
        if (intent == null || intent.getAction() == null) return;

        if ("com.aliunde.tts.CHOOSE_VOICE".equals(intent.getAction())) {
            Intent chooser = new Intent(context, VoiceChooserActivity.class);
            chooser.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
            context.startActivity(chooser);
            return;
        }

        Intent service = new Intent(context, AliundeTtsService.class);
        service.setAction(intent.getAction());
        if (intent.getExtras() != null) service.putExtras(intent.getExtras());
        context.startService(service);
    }

    static boolean isEnglishVoice(android.speech.tts.Voice voice) {
        return voice != null && voice.getLocale() != null
                && java.util.Locale.ENGLISH.getLanguage().equalsIgnoreCase(
                        voice.getLocale().getLanguage());
    }
}
