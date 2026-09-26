#include <kodi/AddonBase.h>
#include <kodi/addon-instance/Peripheral.h>
#if defined(ANDROID)
#include <kodi/platform/android/System.h>
#endif

class CBridgeProof final : public kodi::addon::CAddonBase,
                           public kodi::addon::CInstancePeripheral
{
public:
  CBridgeProof()
  {
    kodi::Log(ADDON_LOG_INFO, "[Aliunde Binary Proof] GATE 1 PASS: Kodi loaded binary addon");
#if defined(ANDROID)
    kodi::platform::CInterfaceAndroidSystem system;
    const int sdk = system.GetSDKVersion();
    const std::string className = system.GetClassName();
    if (sdk > 0)
      kodi::Log(ADDON_LOG_INFO, "[Aliunde Binary Proof] GATE 2 PASS: ANDROID_SYSTEM sdk=%d class=%s", sdk, className.c_str());
    else
      kodi::Log(ADDON_LOG_ERROR, "[Aliunde Binary Proof] GATE 2 FAIL: ANDROID_SYSTEM unavailable");

    void* env = system.GetJNIEnv();
    if (env)
      kodi::Log(ADDON_LOG_INFO, "[Aliunde Binary Proof] GATE 3 PASS: Kodi supplied JNIEnv=%p", env);
    else
      kodi::Log(ADDON_LOG_ERROR, "[Aliunde Binary Proof] GATE 3 FAIL: GetJNIEnv returned null");
#else
    kodi::Log(ADDON_LOG_ERROR, "[Aliunde Binary Proof] WRONG PLATFORM: not Android");
#endif
  }

  void GetCapabilities(kodi::addon::PeripheralCapabilities& capabilities) override
  {
    capabilities.SetProvidesJoysticks(false);
    capabilities.SetProvidesButtonmaps(false);
  }
};

ADDONCREATOR(CBridgeProof)
