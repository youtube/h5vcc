/*
 * Copyright 2012 Google Inc. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "config.h"
#include "lb_platform.h"
#include "WebFontInfo.h"
#include "WebFontRenderStyle.h"

#include <ft2build.h>  // must come before freetype.h
#include <freetype/freetype.h>
#include <string.h>
#include <unicode/uscript.h>
#include <unicode/utf16.h>

#include <string>

#if defined(NDEBUG)
#define debug_printf(...)
#else
#define debug_printf printf
#endif


// This font mapping code is designed to save memory while still providing quick
// decisions about the best font for a given character.  "Best" here is defined
// as first preferring the font we have assigned to the character's script,
// and failing that, any font that actually has the glyph in question.
// In particular, I'm trying to avoid a bitmap of the entire unicode range
// per font, which would cost us 136kB per font.  This way costs us about 1.1MB
// total and supports up to 256 fonts.

typedef enum {
  // Droid Sans fonts:
  DroidArabicNaskh,
  DroidSans,
  DroidSansArmenian,
  DroidSansEthiopic,
  DroidSansFallback,
  DroidSansGeorgian,
  DroidSansHebrew,
  DroidSansJapanese,
  DroidSansThai,

  // Lohit fonts:
  LohitBengali,
  LohitDevanagari,
  LohitGujarati,
  LohitKannada,
  LohitMalayalam,
  LohitOriya,
  LohitPunjabi,
  LohitTamil,
  LohitTelugu,

  // MUST COME LAST!
  kNumBuiltInFonts
} ShellFont;
static const ShellFont kNoSuchFont = kNumBuiltInFonts;

// Used to compute the font name for familyForChars's return value.
// All fonts must be in this list.
static inline const char *fontFamilyByEnum(ShellFont font) {
  switch (font) {
    case DroidArabicNaskh:
      return "Droid Arabic Naskh";
    case DroidSans:
      return "Droid Sans";
    case DroidSansArmenian:
      return "Droid Sans Armenian";
    case DroidSansEthiopic:
      return "Droid Sans Ethiopic";
    case DroidSansFallback:
      return "Droid Sans Fallback";
    case DroidSansGeorgian:
      return "Droid Sans Georgian";
    case DroidSansHebrew:
      return "Droid Sans Hebrew";
    case DroidSansJapanese:
      return "Droid Sans Japanese";
    case DroidSansThai:
      return "Droid Sans Thai";
    case LohitBengali:
      return "Lohit Bengali";
    case LohitDevanagari:
      return "Lohit Devanagari";
    case LohitGujarati:
      return "Lohit Gujarati";
    case LohitKannada:
      return "Lohit Kannada";
    case LohitMalayalam:
      return "Lohit Malayalam";
    case LohitOriya:
      return "Lohit Oriya";
    case LohitPunjabi:
      return "Lohit Punjabi";
    case LohitTamil:
      return "Lohit Tamil";
    case LohitTelugu:
      return "Lohit Telugu";
    case kNoSuchFont:
      // this means we are missing a glyph we need!
      return "";
  }

  ASSERT(false);  // this should not happen.
  return "";
}

// Used by buildFontMap to index all fonts.
// All fonts must be in this list.
static inline const char *fontFileByEnum(ShellFont font) {
  switch (font) {
    case DroidArabicNaskh:
      return "DroidNaskh-Regular";
    case DroidSans:
      return "DroidSans";
    case DroidSansArmenian:
      return "DroidSansArmenian";
    case DroidSansEthiopic:
      return "DroidSansEthiopic-Regular";
    case DroidSansFallback:
      return "DroidSansFallbackFull";
    case DroidSansGeorgian:
      return "DroidSansGeorgian";
    case DroidSansHebrew:
      return "DroidSansHebrew-Regular";
    case DroidSansJapanese:
      return "DroidSansJapanese";
    case DroidSansThai:
      return "DroidSansThai";
    case LohitBengali:
      return "Lohit-Bengali";
    case LohitDevanagari:
      return "Lohit-Devanagari";
    case LohitGujarati:
      return "Lohit-Gujarati";
    case LohitKannada:
      return "Lohit-Kannada";
    case LohitMalayalam:
      return "Lohit-Malayalam";
    case LohitOriya:
      return "Lohit-Oriya";
    case LohitPunjabi:
      return "Lohit-Punjabi";
    case LohitTamil:
      return "Lohit-Tamil";
    case LohitTelugu:
      return "Lohit-Telugu";
    default:
      break;
  }

  ASSERT(false);  // this should not happen.
  return "";
}

// Used by buildFontMap to decide conflicts between fonts that support the
// same glyph.  Only has an effect when none of the fonts that support a glyph
// are requested by name.
static inline ShellFont getPreferredFont(UScriptCode script_code,
                                         bool japanese_mode) {
  switch (script_code) {
    case USCRIPT_COMMON:  // non-breaking space, etc.
    case USCRIPT_INHERITED:
      return DroidSansFallback;

    case USCRIPT_BOPOMOFO:
    case USCRIPT_SIMPLIFIED_HAN:
    case USCRIPT_TRADITIONAL_HAN:
      return DroidSansFallback;

    case USCRIPT_HANGUL:
    case USCRIPT_KOREAN:
      return DroidSansFallback;

    case USCRIPT_HIRAGANA:
    case USCRIPT_KATAKANA:
    case USCRIPT_KATAKANA_OR_HIRAGANA:
    case USCRIPT_JAPANESE:
      return DroidSansJapanese;

    case USCRIPT_HAN:
      return japanese_mode ? DroidSansJapanese : DroidSansFallback;

    case USCRIPT_CYRILLIC:
    case USCRIPT_GREEK:
    case USCRIPT_LATIN:
      return DroidSans;

    case USCRIPT_ARABIC:
      return DroidArabicNaskh;
    case USCRIPT_ARMENIAN:
      return DroidSansArmenian;
    case USCRIPT_ETHIOPIC:
      return DroidSansEthiopic;
    case USCRIPT_GEORGIAN:
      return DroidSansGeorgian;
    case USCRIPT_HEBREW:
      return DroidSansHebrew;
    case USCRIPT_THAI:
      return DroidSansThai;

    case USCRIPT_BENGALI:
      return LohitBengali;
    case USCRIPT_DEVANAGARI:
      return LohitDevanagari;
    case USCRIPT_GUJARATI:
      return LohitGujarati;
    case USCRIPT_KANNADA:
      return LohitKannada;
    case USCRIPT_MALAYALAM:
      return LohitMalayalam;
    case USCRIPT_ORIYA:
      return LohitOriya;
    case USCRIPT_GURMUKHI:
      return LohitPunjabi;
    case USCRIPT_TAMIL:
      return LohitTamil;
    case USCRIPT_TELUGU:
      return LohitTelugu;

    case USCRIPT_UNKNOWN:
      return DroidSansFallback;
    default:
      break;
  }

  return kNoSuchFont;
}

static bool font_map_is_built = false;
static const int kNumUnicodeCodePoints = 0x110000;
// actually a ShellFont value, but char saves us memory:
static char best_font_for_char[kNumUnicodeCodePoints];
// FT_STYLE_FLAG_ITALIC == 1, FT_STYLE_FLAG_BOLD == 2
static char flags_for_font[kNumBuiltInFonts];

extern std::string *global_game_content_path;

// This function runs in about 1 second on the PS3.
static void buildFontMap(const char *preferredLocale) {
  bool japanese_mode = (strcmp(preferredLocale, "ja") == 0);
  debug_printf("Building font map.  Locale = %s.\n", preferredLocale);

  // clear the map.
  memset(best_font_for_char, kNoSuchFont, sizeof(best_font_for_char));

  // initialize FreeType.
  FT_Library freetype_lib;
  if (FT_Init_FreeType(&freetype_lib) != 0) {
    debug_printf("Failed to initialize FreeType!\n");
    ASSERT(false);  // this should not happen.
    return;
  }

  // iterate through the fonts.
  for (int font_num = 0; font_num < kNumBuiltInFonts; ++font_num) {
    ShellFont font = static_cast<ShellFont>(font_num);
    flags_for_font[font_num] = 0;

    std::string filename = *global_game_content_path;
    filename.append("/fonts/");
    filename.append(fontFileByEnum(font));
    filename.append(".ttf");

    // load this font.
    FT_Face face;
    FT_Error err = FT_New_Face(freetype_lib, filename.c_str(), 0, &face);
    if (err) {
      debug_printf("Failed to load font %s\n", filename.c_str());
      continue;
    }

    flags_for_font[font_num] = face->style_flags;

    // map out this font's characters.
    for (long code_point = 0; code_point < kNumUnicodeCodePoints; code_point++) {
      if (FT_Get_Char_Index(face, code_point) != 0) {
        // the font has this character.
        // what script does this character belong to?
        UErrorCode error_code = U_ZERO_ERROR;
        UScriptCode script_code = uscript_getScript(code_point, &error_code);
        if (error_code) {
          debug_printf("Unable to determine script for code point U+%lx\n", (long)code_point);
          ASSERT(false);  // this should not happen.
          continue;
        }

        // overwrite what's in the map if:
        //    1) this font is the preferred font for this script
        // or 2) there's no font support for this character yet.
        if (font == getPreferredFont(script_code, japanese_mode) ||
            best_font_for_char[code_point] == kNoSuchFont) {
          best_font_for_char[code_point] = font;
        }
      }
    }

    // release this font.
    FT_Done_Face(face);
  }

  // shut down FreeType.
  FT_Done_FreeType(freetype_lib);

  // mark the map as built.
  font_map_is_built = true;
}

namespace WebKit {

void WebFontInfo::familyForChars(const WebUChar* characters, size_t numCharacters, const char* preferredLocale, WebKit::WebFontFamily *family)
{
  if (!font_map_is_built)
    buildFontMap(preferredLocale);

  // If this function is even called, it means that the font already being used
  // is missing some required characters.

  // extract the code point from the UTF-16 input:
  uint32_t code_point;
  if (numCharacters > 1
      && U16_IS_SURROGATE(characters[0])
      && U16_IS_SURROGATE_LEAD(characters[0])
      && U16_IS_TRAIL(characters[1])) {
    code_point = U16_GET_SUPPLEMENTARY(characters[0], characters[1]);
  } else {
    code_point = characters[0];
  }

  ASSERT(code_point < kNumUnicodeCodePoints);

  ShellFont best_font = static_cast<ShellFont>(best_font_for_char[code_point]);
  std::string fallbackFamily = fontFamilyByEnum(best_font);

  // fill the family structure as output:
  family->name = WebCString(fallbackFamily.data(), fallbackFamily.length());
  family->isBold = flags_for_font[best_font] & FT_STYLE_FLAG_BOLD;
  family->isItalic = flags_for_font[best_font] & FT_STYLE_FLAG_ITALIC;
}

void WebFontInfo::renderStyleForStrike(const char* family, int sizeAndStyle, WebFontRenderStyle* out)
{
  out->useBitmaps = 0;
  out->useAutoHint = 0;
  out->useHinting = 0;
  out->hintStyle = 0;
  out->useAntiAlias = 1;
  out->useSubpixelRendering = 1;
}

} // namespace WebKit
