package com.aliunde.tts;

import android.app.Activity;
import android.content.Context;
import android.os.Bundle;
import android.speech.tts.TextToSpeech;
import android.speech.tts.Voice;
import android.view.View;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.ListView;
import android.widget.Toast;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.List;
import java.util.Set;
import java.util.UUID;

public final class VoiceChooserActivity extends Activity {
    private TextToSpeech tts;
    private final List<Voice> voices = new ArrayList<>();
    private String lastPreview = "";

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        ListView list = new ListView(this);
        list.setChoiceMode(ListView.CHOICE_MODE_SINGLE);
        list.setFocusable(true);
        setContentView(list);

        tts = new TextToSpeech(getApplicationContext(), status -> {
            if (status != TextToSpeech.SUCCESS) {
                Toast.makeText(this, "Android text to speech is unavailable", Toast.LENGTH_LONG).show();
                finish();
                return;
            }

            Set<Voice> installed = tts.getVoices();
            if (installed != null) {
                for (Voice voice : installed) {
                    if (TtsReceiver.isEnglishVoice(voice)) voices.add(voice);
                }
            }

            Collections.sort(voices, Comparator
                    .comparing((Voice v) -> v.getLocale().toLanguageTag())
                    .thenComparing(Voice::getName));

            if (voices.isEmpty()) {
                Toast.makeText(this, "No installed English voices found", Toast.LENGTH_LONG).show();
                finish();
                return;
            }

            List<String> labels = new ArrayList<>();
            for (Voice voice : voices) labels.add(labelFor(voice));
            ArrayAdapter<String> adapter = new ArrayAdapter<>(
                    this, android.R.layout.simple_list_item_single_choice, labels);
            list.setAdapter(adapter);

            String saved = getSharedPreferences("aliunde.tts", Context.MODE_PRIVATE)
                    .getString("voice", "");
            int selected = findVoice(saved);
            if (selected < 0) selected = 0;
            list.setItemChecked(selected, true);
            list.setSelection(selected);

            list.setOnItemSelectedListener(new AdapterView.OnItemSelectedListener() {
                @Override
                public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {
                    preview(position);
                }

                @Override
                public void onNothingSelected(AdapterView<?> parent) {
                }
            });

            list.setOnItemClickListener((parent, view, position, id) -> {
                Voice voice = voices.get(position);
                getSharedPreferences("aliunde.tts", Context.MODE_PRIVATE)
                        .edit().putString("voice", voice.getName()).apply();
                tts.setVoice(voice);
                tts.speak("Voice selected", TextToSpeech.QUEUE_FLUSH, null,
                        "aliunde-selected-" + UUID.randomUUID());
                list.postDelayed(this::finish, 700);
            });

            list.requestFocus();
            preview(selected);
        });
    }

    private void preview(int position) {
        if (tts == null || position < 0 || position >= voices.size()) return;
        Voice voice = voices.get(position);
        if (voice.getName().equals(lastPreview)) return;
        lastPreview = voice.getName();
        tts.stop();
        tts.setVoice(voice);
        tts.speak("Aliunde Screen Reader. " + labelFor(voice),
                TextToSpeech.QUEUE_FLUSH, null, "aliunde-preview-" + UUID.randomUUID());
    }

    private int findVoice(String name) {
        for (int i = 0; i < voices.size(); i++) {
            if (voices.get(i).getName().equals(name)) return i;
        }
        return -1;
    }

    private static String labelFor(Voice voice) {
        String locale = voice.getLocale() == null ? "English" : voice.getLocale().toLanguageTag();
        return locale + " - " + voice.getName();
    }

    @Override
    protected void onDestroy() {
        if (tts != null) {
            tts.stop();
            tts.shutdown();
            tts = null;
        }
        super.onDestroy();
    }
}
