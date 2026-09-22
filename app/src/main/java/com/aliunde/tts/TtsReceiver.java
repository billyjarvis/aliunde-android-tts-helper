package com.aliunde.tts;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.os.Bundle;
import android.speech.tts.TextToSpeech;
import android.speech.tts.Voice;
import java.util.Locale;
import java.util.Set;
import java.util.UUID;

public final class TtsReceiver extends BroadcastReceiver {
    private static TextToSpeech tts;
    private static boolean ready;
    private static String pendingText;
    private static String pendingVoice;
    private static float pendingRate = 1.0f;
    private static float pendingVolume = 1.0f;
    private static Context appContext;

    @Override
    public void onReceive(Context context, Intent intent) {
        if (intent == null || intent.getAction() == null) return;
        appContext = context.getApplicationContext();
        String action = intent.getAction();

        if ("com.aliunde.tts.STOP".equals(action)) {
            pendingText = null;
            if (tts != null) tts.stop();
            return;
        }

        if ("com.aliunde.tts.CHOOSE_VOICE".equals(action)) {
            Intent chooser = new Intent(appContext, VoiceChooserActivity.class);
            chooser.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
            appContext.startActivity(chooser);
            return;
        }

        if (!"com.aliunde.tts.SPEAK".equals(action)) return;

        pendingText = intent.getStringExtra("text");
        pendingVoice = intent.getStringExtra("voice");
        pendingRate = clamp(intent.getFloatExtra("rate", 1.0f), 0.5f, 2.0f);
        pendingVolume = clamp(intent.getFloatExtra("volume", 1.0f), 0.0f, 1.0f);
        if (pendingText == null || pendingText.trim().isEmpty()) return;

        ensureTts();
    }

    private static synchronized void ensureTts() {
        if (tts == null) {
            ready = false;
            tts = new TextToSpeech(appContext, status -> {
                ready = status == TextToSpeech.SUCCESS;
                if (ready) speakPending();
            });
        } else if (ready) {
            speakPending();
        }
    }

    private static synchronized void speakPending() {
        if (!ready || tts == null || pendingText == null || appContext == null) return;
        tts.stop();

        String selected = pendingVoice;
        if (selected == null || selected.trim().isEmpty()) {
            selected = appContext.getSharedPreferences("aliunde.tts", Context.MODE_PRIVATE)
                    .getString("voice", "");
        }
        if (!selected.isEmpty()) selectVoice(selected);

        tts.setSpeechRate(pendingRate);
        Bundle params = new Bundle();
        params.putFloat(TextToSpeech.Engine.KEY_PARAM_VOLUME, pendingVolume);
        String text = pendingText;
        pendingText = null;
        pendingVoice = null;
        tts.speak(text, TextToSpeech.QUEUE_FLUSH, params, "aliunde-" + UUID.randomUUID());
    }

    private static void selectVoice(String requested) {
        Set<Voice> voices = tts.getVoices();
        if (voices == null) return;
        for (Voice voice : voices) {
            if (requested.equals(voice.getName())) {
                tts.setVoice(voice);
                return;
            }
        }
    }

    static boolean isEnglishVoice(Voice voice) {
        if (voice == null || voice.getLocale() == null) return false;
        return Locale.ENGLISH.getLanguage().equalsIgnoreCase(voice.getLocale().getLanguage());
    }

    private static float clamp(float value, float min, float max) {
        return Math.max(min, Math.min(max, value));
    }
}
