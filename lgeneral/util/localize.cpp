/*
    Translation utilities.
    Copyright (C) 2005 Leo Savernik  All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions
    are met:
    1. Redistributions of source code must retain the above copyright
       notice, this list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright
       notice, this list of conditions and the following disclaimer in the
       documentation and/or other materials provided with the distribution.

    THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY
    EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
    PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR
    CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
    EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
    PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
    PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
    OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
    OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "localize.h"

#include <assert.h>
#include <locale.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <string>
#include <unordered_set>

#ifdef WIN32
  extern int setenv(const char *name, const char *value, int override);
#endif

/** internal language code. LC_C means default as well as unknown. */
typedef enum { LC_C, LC_DE, LC_EN } LanguageCode;

static LanguageCode lang_code;
/* Only domain presence matters here; the value of the old hashtable was a
 * dummy struct that was never read. */
static std::unordered_set<std::string> loaded_domains;

/* NOTE: Keep in sync with LanguageCode enum.
 * It serves as an index into this table
 */
/** language data for all supported languages */
static const struct {
  const char *lang;	/* iso language-specification */
  const char *charset;	/* charset for output in lgeneral */
}
lang_data[] = {
  { "C",  "ISO-8859-1" },
  { "de", "ISO-8859-1" },
  { "en", "ISO-8859-1" },
};

#if 0
const char *tr(const char *s) {
  const char *r = gettext(s);
  fprintf(stderr, "'%s' -> '%s'\n", s, r);
  return r;
}
#endif

/**
 * Determines from the given language-string a suitable
 * language-code, or LC_C if no match.
 */
static LanguageCode locale_determine_internal_code(const char *str) {
  unsigned len = strlen(str);
  char *buf = (char *)alloca(len + 1);
  char *pos;

  /* make a copy we can manipulate at will */
  strcpy(buf, str);

  /* format can be of ll_TT.ENCODING. Strip .ENCODING */
  pos = strchr(buf, '.');
  len = pos ? pos - buf : len;
  buf[len] = 0;

  do {
    /* In the first pass, try to match ll_TT.
     * If this fails, strip _TT and match again.
     */
    size_t i;
    for (i = 0; i < sizeof lang_data/sizeof lang_data[0]; i++) {
      if (strcmp(lang_data[i].lang, buf) == 0)
        return (LanguageCode)i;
    }

    /* strip _TT */
    pos = strchr(buf, '_');
    len = pos ? pos - buf : len;
    buf[len] = 0;
  } while(pos);

  return LC_C;
}

int locale_load_domain(const char *domain, const char *translations) {
  /* TODO: In the future resources may specify a package containing
   * all translations for a certain domain.
   * Currently, we rely on all translations being preinstalled.
   */
  assert(translations == 0);
  if ( !domain ) return 0;

  /* don't add any domain twice */
  if ( loaded_domains.find( domain ) != loaded_domains.end() ) return 1;

#ifdef ENABLE_NLS
  {
#ifndef LOCALEDIR
#  define LOCALEDIR "."
#endif
    bindtextdomain(domain, LOCALEDIR);
    /* The catalogs are ISO-8859-1 and the game renders text in that charset.
     * Request the conversion explicitly instead of relying on the deprecated
     * OUTPUT_CHARSET environment variable. */
    bind_textdomain_codeset(domain, lang_data[lang_code].charset);
  }
#endif
  loaded_domains.emplace( domain );
  return 1;
}

void locale_write_ordinal_number_lang(char *buf, unsigned n, LanguageCode code, int number) {

  switch (code) {
    case LC_DE:
      /* Ah! Is that easy :-) */
      snprintf( buf, n, "%d.", number);
      break;
    case LC_C:
    case LC_EN:
    default: {
      const int num100 = number % 100;
      switch ( num100 < 10 || num100 > 19 ? number % 10 : 4 ) {
        case 1: snprintf( buf, n, "%dst", number ); break;
        case 2: snprintf( buf, n, "%dnd", number ); break;
        case 3: snprintf( buf, n, "%drd", number ); break;
        default: snprintf( buf, n, "%dth", number ); break;
      }
      break;
    }
  }
}

void locale_write_ordinal_number(char *buf, unsigned n, int number) {
  locale_write_ordinal_number_lang(buf, n, lang_code, number);
}

void locale_init(const char *lang) {
  static const char *vars[] = { "LANGUAGE", "LC_ALL", "LC_MESSAGES", "LANG" };
  size_t i;

  if (lang) {
    setenv("LANGUAGE", lang, 1);
  }

  /* traverse envvars in same order like gettext and use
   * the first one found.
   */
  for (i = 0; i < sizeof vars/sizeof vars[0]; i++) {
    const char *lang = getenv(vars[i]);
    if (lang) {
      lang_code = locale_determine_internal_code(lang);
      break;
    }
  }

#if 0
  fprintf(stderr, "lc: %d, lang: %s\n", lang_code, lang_data[lang_code].str);
#endif

  setlocale(LC_MESSAGES, "");
  
  /* for glibc, force the output charset */
  setenv("OUTPUT_CHARSET", lang_data[lang_code].charset, 1);
  
  locale_load_domain(PACKAGE, 0);
#ifdef ENABLE_NLS
  textdomain(PACKAGE);
#endif
}

/* kate: tab-indents on; space-indent on; replace-tabs off; indent-width 2; dynamic-word-wrap off; inden(t)-mode cstyle */
