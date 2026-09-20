package com.aliunde.tts;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.os.Bundle;
import android.speech.tts.TextToSpeech;
import android.speech.tts.Voice;
import java.util.Set;
import java.util.UUID;

public final class TtsReceiver extends BroadcastReceiver {
    private static TextToSpeech tts;
    private static boolean ready;
    private static String pendingText;
    private static float pendingRate = 1.0f;
    private static float pendingVolume = 1.0f;
    private static Context appContext;

    @Override
    public void onReceive(Context context, Intent intent) {
        if (intent == null || intent.getAction() == null) return;
        appContext = context.getApplicationContext();
        if ("com.aliunde.tts.STOP".equals(intent.getAction())) {
            pendingText = null;
            if (tts != null) tts.stop();
            return;
        }
        if (!"com.aliunde.tts.SPEAK".equals(intent.getAction())) return;

        pendingText = intent.getStringExtra("text");
        pendingRate = clamp(intent.getFloatExtra("rate", 1.0f), 0.5f, 2.0f);
        pendingVolume = clamp(intent.getFloatExtra("volume", 1.0f), 0.0f, 1.0f);
        if (pendingText == null || pendingText.trim().isEmpty()) return;

        if (tts == null) {
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
        String selected = appContext.getSharedPreferences("aliunde.tts", Context.MODE_PRIVATE)
                .getString("voice", "");
        if (!selected.isEmpty()) selectVoice(selected);
        tts.setSpeechRate(pendingRate);
        Bundle params = new Bundle();
        params.putFloat(TextToSpeech.Engine.KEY_PARAM_VOLUME, pendingVolume);
        String text = pendingText;
        pendingText = null;
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

    private static float clamp(float value, float min, float max) {
        return Math.max(min, Math.min(max, value));
    }
}
