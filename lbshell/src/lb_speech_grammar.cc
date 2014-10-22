/*
 * Copyright 2014 Google Inc. All Rights Reserved.
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

#include "lb_speech_grammar.h"

#include <libxml/tree.h>
#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>

#include "base/debug/trace_event.h"
#include "base/logging.h"
#include "base/string_number_conversions.h"
#include "base/string_util.h"
#include "base/utf_string_conversions.h"
#include "net/base/escape.h"

#include "config.h"
#if ENABLE(SCRIPTED_SPEECH)

namespace {
const char* kXmlNamespace = "http://www.w3.org/2001/06/grammar";
const char* kXmlDataMimePrefix = "data:application/xml,";
const size_t kXmlDataMimePrefixSize = strlen(kXmlDataMimePrefix);

const char* XmlCharToChar(const xmlChar* str) {
  return reinterpret_cast<const char*>(str);
}

const xmlChar* CharToXmlChar(const char* str) {
  return reinterpret_cast<const xmlChar*>(str);
}

const char* GetNodeValue(xmlNode* node) {
  return XmlCharToChar(xmlNodeListGetString(node->doc, node->children, 1));
}

xmlXPathObjectPtr EvaluateXPath(xmlDocPtr doc) {
  TRACE_EVENT0("lb_shell", "LB::SpeechGrammar::EvaluateXPath");

  xmlXPathContextPtr context = xmlXPathNewContext(doc);
  if (context == NULL) {
    DLOG(WARNING) << "Error in xmlXPathNewContext";
    return NULL;
  }
  // Register a namespace with prefix. We need to associate the URIs xml
  // namespace with a prefix and use the prefix as stand-in for the URIs in
  // xpath.
  if (xmlXPathRegisterNs(context, CharToXmlChar("grammar"),
      CharToXmlChar(kXmlNamespace))) {
    DLOG(WARNING) << "Error: unable to register NS with prefix";
    xmlXPathFreeContext(context);
    return NULL;
  }

  xmlXPathObjectPtr result = xmlXPathEvalExpression(
      CharToXmlChar("//grammar:rule"), context);
  xmlXPathFreeContext(context);

  if (result == NULL) {
    DLOG(WARNING) << "Error in xmlXPathEvalExpression";
    return NULL;
  }
  if (xmlXPathNodeSetIsEmpty(result->nodesetval)) {
    xmlXPathFreeObject(result);
    DLOG(INFO) << "No result";
    return NULL;
  }
  return result;
}

// Parse a xml Node of type:
// <rule id="foo">
//   <phrase>Play</phrase>
//   <action>Play Video</phrase>
//   <pronounciation>peh lay</pronounciation>
//   <confidence>0.75</confidence>
// </rule>
// Into a LB::SpeechRule.
scoped_ptr<LB::SpeechRule> LoadPhrase(xmlNode* node) {
  DCHECK(node);
  scoped_ptr<LB::SpeechRule> result(new LB::SpeechRule);

  const char* id = XmlCharToChar(xmlGetProp(node, CharToXmlChar("id")));
  DCHECK(id);
  if (id != NULL && *id != '\0') {
    result->id = id;
  }

  xmlNode* child = node->children;
  while (child) {
    if (xmlStrEqual(child->name, CharToXmlChar("phrase"))) {
      result->phrase = GetNodeValue(child);
    } else if (xmlStrEqual(child->name, CharToXmlChar("pronunciation"))) {
      result->pronunciation = GetNodeValue(child);
    } else if (xmlStrEqual(child->name, CharToXmlChar("action"))) {
      result->action = GetNodeValue(child);
    } else if (xmlStrEqual(child->name, CharToXmlChar("confidence"))) {
      double out_confidence = 1.0f;
      if (base::StringToDouble(GetNodeValue(child), &out_confidence)) {
        result->confidence = static_cast<float>(out_confidence);
      }
    }
    child = child->next;
  }
  return result.Pass();
}

std::string DecodeURLEscapeSequences(const WebCore::KURL& url) {
  const WTF::CString& utf8 = url.utf8String();
  if (utf8.isNull())
    return std::string();
  return net::UnescapeURLComponent(std::string(utf8.data(), utf8.length()),
      net::UnescapeRule::SPACES | net::UnescapeRule::URL_SPECIAL_CHARS);
}

}  // namespace

namespace LB {

// static
scoped_ptr<SpeechGrammar> SpeechGrammar::LoadFromXml(const std::string& src) {
  TRACE_EVENT0("lb_shell", "LB::SpeechGrammar::LoadFromXml");

  scoped_ptr<SpeechGrammar> grammar(new SpeechGrammar);
  // The data type uri starts with "data:application/xml,", and we only want to
  // parse the part after that, so we find the position of "," and only parse
  // the substring after it.
  if (!StartsWithASCII(src, kXmlDataMimePrefix, true /* case sensitive */)) {
    grammar->failure_reason_ = "Grammar must start with data:application/xml.";
    return grammar.Pass();
  }

  xmlDoc *doc = xmlReadDoc(
      CharToXmlChar(src.c_str() + kXmlDataMimePrefixSize),
      NULL,  /* url */
      "utf-8",
      0  /* parser options */);
  if (!doc) {
    DLOG(WARNING) << "Unable to parse grammar xml " << src;
    grammar->failure_reason_ = "Unable to parse grammar xml.";
    return grammar.Pass();
  }
  xmlXPathObjectPtr result = EvaluateXPath(doc);
  if (result) {
    xmlNodeSetPtr nodeset = result->nodesetval;
    for (int i = 0; i < nodeset->nodeNr; ++i) {
      grammar->rules_.push_back(LoadPhrase(nodeset->nodeTab[i]).release());
    }
    xmlXPathFreeObject(result);
  } else {
    grammar->failure_reason_ = "Bad grammar XML.";
  }
  xmlFreeDoc(doc);

  grammar->valid_ = grammar->failure_reason_.isNull();
  return grammar.Pass();
}

// static
scoped_ptr<SpeechGrammar> SpeechGrammar::LoadFromGrammar(
    const WebCore::SpeechGrammar* grammar) {
  DCHECK(grammar);


  if (!grammar->src().protocolIsData()) {
    scoped_ptr<SpeechGrammar> ret(new SpeechGrammar);
    ret->failure_reason_ = "Grammar protocol other than data is not supported.";
    return ret.Pass();
  }

  std::string xml_data = DecodeURLEscapeSequences(grammar->src());
  scoped_ptr<SpeechGrammar> ret(
      LB::SpeechGrammar::LoadFromXml(xml_data).Pass());
  ret->weight_ = abs(grammar->weight());

  return ret.Pass();
}

}  // namespace LB

#endif  // ENABLE(SCRIPTED_SPEECH)