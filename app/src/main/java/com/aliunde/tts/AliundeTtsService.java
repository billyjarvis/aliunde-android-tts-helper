package com.aliunde.tts;

import android.app.Service;
import android.content.Intent;
import android.os.Bundle;
import android.os.IBinder;
import android.speech.tts.TextToSpeech;
import android.speech.tts.Voice;
import java.util.Set;
import java.util.UUID;

public final class AliundeTtsService extends Service {
    private TextToSpeech tts;
    private boolean ready;
    private String pendingText;
    private String pendingVoice;
    private float pendingRate = 1.0f;
    private float pendingVolume = 1.0f;

    @Override public void onCreate() {
        super.onCreate();
        tts = new TextToSpeech(getApplicationContext(), status -> {
            ready = status == TextToSpeech.SUCCESS;
            if (ready) speakPending();
        });
    }

    @Override public int onStartCommand(Intent intent, int flags, int startId) {
        if (intent == null || intent.getAction() == null) return START_NOT_STICKY;
        String action = intent.getAction();
        if ("com.aliunde.tts.STOP".equals(action)) {
            pendingText = null;
            if (tts != null) tts.stop();
            return START_NOT_STICKY;
        }
        if ("com.aliunde.tts.SPEAK".equals(action)) {
            pendingText = intent.getStringExtra("text");
            pendingVoice = intent.getStringExtra("voice");
            pendingRate = clamp(intent.getFloatExtra("rate", 1.0f), 0.5f, 2.0f);
            pendingVolume = clamp(intent.getFloatExtra("volume", 1.0f), 0.0f, 1.0f);
            if (ready) speakPending();
        }
        return START_NOT_STICKY;
    }

    private synchronized void speakPending() {
        if (!ready || tts == null || pendingText == null || pendingText.trim().isEmpty()) return;
        tts.stop();
        String selected = pendingVoice;
        if (selected == null || selected.trim().isEmpty())
            selected = getSharedPreferences("aliunde.tts", MODE_PRIVATE).getString("voice", "");
        if (!selected.isEmpty()) selectVoice(selected);
        tts.setSpeechRate(pendingRate);
        Bundle params = new Bundle();
        params.putFloat(TextToSpeech.Engine.KEY_PARAM_VOLUME, pendingVolume);
        String text = pendingText;
        pendingText = null;
        pendingVoice = null;
        tts.speak(text, TextToSpeech.QUEUE_FLUSH, params, "aliunde-" + UUID.randomUUID());
    }

    private void selectVoice(String requested) {
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

    @Override public void onDestroy() {
        if (tts != null) {
            tts.stop();
            tts.shutdown();
            tts = null;
        }
        ready = false;
        super.onDestroy();
    }

    @Override public IBinder onBind(Intent intent) { return null; }
}
