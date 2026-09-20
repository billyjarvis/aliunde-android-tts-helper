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

    @Override
    public void onReceive(Context context, Intent intent) {
        if (intent == null || intent.getAction() == null) return;
        if ("com.aliunde.tts.STOP".equals(intent.getAction())) {
            pendingText = null;
            if (tts != null) tts.stop();
            return;
        }
        if (!"com.aliunde.tts.SPEAK".equals(intent.getAction())) return;

        pendingText = intent.getStringExtra("text");
        pendingVoice = intent.getStringExtra("voice");
        pendingRate = clamp(intent.getFloatExtra("rate", 1.0f), 0.5f, 2.0f);
        pendingVolume = clamp(intent.getFloatExtra("volume", 1.0f), 0.0f, 1.0f);
        if (pendingText == null || pendingText.trim().isEmpty()) return;

        if (tts == null) {
            Context app = context.getApplicationContext();
            tts = new TextToSpeech(app, status -> {
                ready = status == TextToSpeech.SUCCESS;
                if (ready) speakPending();
            });
        } else if (ready) {
            speakPending();
        }
    }

    private static synchronized void speakPending() {
        if (!ready || tts == null || pendingText == null) return;
        tts.stop();
        selectVoice(pendingVoice);
        tts.setSpeechRate(pendingRate);
        Bundle params = new Bundle();
        params.putFloat(TextToSpeech.Engine.KEY_PARAM_VOLUME, pendingVolume);
        String text = pendingText;
        pendingText = null;
        tts.speak(text, TextToSpeech.QUEUE_FLUSH, params, "aliunde-" + UUID.randomUUID());
    }

    private static void selectVoice(String requested) {
        if (requested == null || requested.trim().isEmpty()) return;
        Set<Voice> voices = tts.getVoices();
        if (voices == null) return;
        for (Voice voice : voices) {
            if (requested.equals(voice.getName())) {
                tts.setVoice(voice);
                return;
            }
        }
        Locale locale = Locale.forLanguageTag(requested);
        if (!locale.getLanguage().isEmpty()) tts.setLanguage(locale);
    }

    private static float clamp(float value, float min, float max) {
        return Math.max(min, Math.min(max, value));
    }
}
