#include "webkit/media/crypto/key_systems.h"

#include "base/string_util.h"
#include "media/crypto/shell_decryptor_factory.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/platform/WebString.h"

// Convert a WebString to ASCII, falling back on an empty string in the case
// of a non-ASCII string.
static std::string ToASCIIOrEmpty(const WebKit::WebString& string) {
  return IsStringASCII(string) ? UTF16ToASCII(string) : std::string();
}

namespace webkit_media {

bool IsSupportedKeySystem(const WebKit::WebString& key_system) {
  return media::ShellDecryptorFactory::Supports(ToASCIIOrEmpty(key_system));
}

bool IsSupportedKeySystemWithMediaMimeType(
    const std::string& mime_type,
    const std::vector<std::string>& /* codecs */,
    const std::string& key_system) {
  if (media::ShellDecryptorFactory::Supports(key_system)) {
    return mime_type == "video/mp4" || mime_type == "audio/mp4";
  }
  return false;
}

// Used for logging only.
std::string KeySystemNameForUMA(const std::string& key_system) {
  return key_system;
}

// Used for logging only.
std::string KeySystemNameForUMA(const WebKit::WebString& key_system) {
  return UTF16ToASCII(key_system);
}

bool CanUseAesDecryptor(const std::string& /* key_system */) {
  return false;
}

std::string GetPluginType(const std::string& /* key_system */) {
  return std::string();
}

}  // namespace webkit_media
