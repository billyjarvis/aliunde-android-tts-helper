ALIUNDE ANDROID TTS HELPER 0.0.1

Purpose
-------
A minimal Android helper for Aliunde Screen Reader for Kodi.
It sends text directly to Android TextToSpeech.speak().
It does NOT create MP3, MP4 or WAV files and does NOT use Kodi's player.

Build
-----
1. Create an empty GitHub repository.
2. Upload the CONTENTS of this folder to the repository root.
3. Open Actions -> Build Aliunde TTS Helper APK -> Run workflow.
4. Open the completed workflow and download artifact: Aliunde.TTS.Helper.APK
5. Inside the artifact is app-debug.apk.

Do not install/test yet if this is being built as part of the Aliunde Screen Reader repair.
Return the APK to ChatGPT first so the Kodi-side integration can be completed and checked.

Broadcast interface
-------------------
SPEAK action: com.aliunde.tts.SPEAK
Extras:
  text   String, required
  voice  String, Android voice name or language tag such as en-GB
  rate   float, 0.5 to 2.0
  volume float, 0.0 to 1.0

STOP action: com.aliunde.tts.STOP
