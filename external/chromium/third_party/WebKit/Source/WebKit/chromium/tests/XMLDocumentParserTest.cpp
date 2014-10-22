/*
 * Copyright (C) 2013 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"

#include "base/file_path.h"
#include "base/path_service.h"
#include "Document.h"
#include "DocumentType.h"
#include "Element.h"
#include "FrameView.h"
#include "gtest/gtest.h"
#include "HTMLNames.h"
#include "MathMLNames.h"
#include "MediaFeatureNames.h"
#include "NamedNodeMap.h"
#include "SVGNames.h"
#include "third_party/WebKit/Source/Platform/chromium/public/Platform.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/WebKit.h"
#include "XLinkNames.h"
#include "XMLDocumentParser.h"
#include "XMLNames.h"
#include "XMLNSNames.h"
#include "WebKitFontFamilyNames.h"

using namespace WebCore;

namespace {

struct XMLDocumentParserTestData
{
    // Test data file name
    const char* m_fileName;
    // Information to verify
    bool m_good;
    String m_encoding;
    String m_version;
    String m_docType;
    bool m_declaration;
    int m_nodeCount;
    String m_rootNode;
    // Number of $(kTargetNodeName) node in the entire tree
    int m_testTargetCount;
    String m_flatten;
};

static const String kTargetNodeName = "test_target";
static XMLDocumentParserTestData s_paserTestData[] = {
    // parsing empty string to make sure parser can handle this case
    { NULL,                 false, "",      "1.0", "",      false,  0,  "",      0, "" },
    // Good XML samples
    { "xml/xml1.xml",       true,  "",      "1.0", "",      false,  3,  "node",  0, "<node:<type:><content:>>" },
    { "xml/xml2.xml",       true,  "",      "1.0", "",      false,  5,  "node",  0, "<node:<name:multiple_content><type:<attribute1:1><attribute2:2>><content:><content:><content:>>" },
    { "xml/xml3.xml",       true,  "",      "1.0", "",      false,  10, "node",  2, "<node:<name:with_test_target><type:><content:><level1:<level2:<level3:<level4:<test_target:><test_target:>>><level3:>>>>" },
    { "xml/xml4.xml",       true,  "utf-8", "1.0", "node",  true,   4,  "node",  0, "<node:><node:<type:><content:>>" },
    // Bad XML samples
    { "xml/xml_bad1.xml",   false, "",      "1.0", "",      false,  0,  "",      0, "" },
    { "xml/xml_bad2.xml",   false, "",      "1.0", "",      false,  0,  "",      0, "" },
    { "xml/xml_bad3.xml",   false, "",      "1.0", "",      false,  0,  "",      0, "" },
    { "xml/xml_bad4.xml",   false, "",      "1.0", "",      false,  0,  "",      0, "" },
};

class MockPlatform : public WebKit::Platform
{
public:
    virtual void cryptographicallyRandomValues(unsigned char* buffer,
                                               size_t length) {}
};
static MockPlatform s_platform;  // Mock platform

class MockDocument : public Document
{
public:
    virtual ~MockDocument() {}

    static PassRefPtr<Document> create()
    {
        return adoptRef(new MockDocument());
    }

private:
    MockDocument() : Document(NULL, KURL(), true, false) {}
};

class XMLDocumentParserTest
    : public ::testing::TestWithParam<XMLDocumentParserTestData>
{
public:
    virtual void SetUp()
    {
        InitEnvironment();
    }

    virtual void TearDown()
    {
        m_mockDocument = NULL;
        WebKit::Platform::shutdown();
    }

    static void InitEnvironment()
    {
        // Global init
        WebKit::Platform::initialize(&s_platform);
        WTF::initializeThreading();
        WTF::initializeMainThread();
        // Init global objects. This list is the same as the list
        // in Frame.cpp. It is not convenient to create a Frame object
        // here because of the dependencies, so the whole list is copied
        // here to have the same environment. They are global objects
        // and should be destroyed automatically after app ends, hence
        // no need to call deinit in TearDown.
        AtomicString::init();
        HTMLNames::init();
        QualifiedName::init();
        MediaFeatureNames::init();
        SVGNames::init();
        XLinkNames::init();
        MathMLNames::init();
        XMLNSNames::init();
        XMLNames::init();
        WebKitFontFamilyNames::init();
    }

    void Parse()
    {
        // Create document and parser
        m_mockDocument = MockDocument::create();
        m_XMLParser = XMLDocumentParser::create(m_mockDocument.get(), NULL);
        m_parser = m_XMLParser;
        // Append content
        m_parser->append(m_content);
        // Finish
        m_parser->finish();
        m_parser->detach();
    }

    bool WellFormed()
    {
        return m_XMLParser->wellFormed();
    }

    Document* GetDocument() { return m_mockDocument.get(); }

    bool ReadFile(const char* filePath)
    {
        struct stat statBuf;
        FilePath dataPath;
        PathService::Get(base::DIR_SOURCE_ROOT, &dataPath);
        dataPath = dataPath.Append(FILE_PATH_LITERAL(filePath));

        const char* path = dataPath.value().c_str();

        if (stat(path, &statBuf))
        {
            return false;
        }

        WTF::Vector<char> contents;
        contents.resize(statBuf.st_size);
        FILE* fin(fopen(path, "rb"));
        if (!fin)
        {
            return false;
        }
        size_t bytes_read(fread(contents.data(), 1, contents.size(), fin));
        fclose(fin);

        if (bytes_read != contents.size())
        {
            return false;
        }
        // Set to content string
        m_content = String::fromUTF8(contents.data(), contents.size());
        return true;
    }

    // Helper function to print out names of all non-builtin nodes.
    // This function is not called currently but it is very useful
    // for debugging.
    void OutputTree(Node* node, int level)
    {
        if (!node) return;

        String name = node->nodeName();
        // Print out tree node if it is not built-in node
        if (!name.startsWith("#")) {  // Not a builtin node
            for (int space = 0; space < level; ++space)
                printf("  ");
            String value = node->nodeValue();
            printf("%s:%s\n", name.utf8().data(), value.utf8().data());
            // Output attributes if available
            const NamedNodeMap* attributes = node->attributes();
            if (attributes) {
                for (int i = 0; i < attributes->length(); ++i ) {
                    RefPtr<Node> attribute = attributes->item(i);
                    OutputTree(attribute.get(), level + 1);
                }
            }
        }

        for (int i = 0; i < node->childNodeCount(); ++i) {
            OutputTree(node->childNode(i), level + 1);
        }

        return;
    }

    int TotalNodeCount(Node* node)
    {
        if (!node) return 0;

        int count = 0;  // Current node
        String name = node->nodeName();
        if (!name.startsWith("#")) {  // Not a builtin node
            count++;
        }

        for (int i = 0; i < node->childNodeCount(); ++i) {
            count += TotalNodeCount(node->childNode(i));
        }

        return count;
    }

    int TargetNodeCount(Node* node)
    {
        if (!node) return 0;

        int count = 0;  // Current node
        String name = node->nodeName();
        if (name == kTargetNodeName) {
            count++;
        }

        for (int i = 0; i < node->childNodeCount(); ++i) {
            count += TargetNodeCount(node->childNode(i));
        }

        return count;
    }

    Node* FindRootNode(Node* document)
    {
        // Find a non-builtin node in document's children
        Node* root = NULL;
        for (int i = 0; i < document->childNodeCount(); ++i) {
            Node* currentNode = document->childNode(i);
            String name = currentNode->nodeName();
            if (!name.startsWith("#")) {  // Not a builtin node
                if (root)
                    EXPECT_EQ(root->nodeName(), currentNode->nodeName());
                root = currentNode;
            }
        }
        return root;
    }

    String FlattenNodes(Node* node)
    {
        if (!node) return "";

        String str;  // Current node
        String name = node->nodeName();
        bool valid = false;
        if (!name.startsWith("#")) {
            valid = true;
            // Append node name
            str.append('<');
            str.append(name);
            // Append node value
            String value = node->nodeValue();
            str.append(':');
            str.append(value);
            // Flatten attributes if available
            const NamedNodeMap* attributes = node->attributes();
            if (attributes) {
                for (int i = 0; i < attributes->length(); ++i ) {
                    RefPtr<Node> attribute = attributes->item(i);
                    str.append(FlattenNodes(attribute.get()));
                }
            }
        }
        // Flatten child nodes
        for (int i = 0; i < node->childNodeCount(); ++i) {
            str.append(FlattenNodes(node->childNode(i)));
        }
        // End of node
        if (valid) {
            str.append('>');
        }
        return str;
    }

private:
    String m_content;  // XML content
    RefPtr<Document> m_mockDocument;
    // These two parser pointers point to the same object.
    // They are used to access different member functions.
    RefPtr<DocumentParser> m_parser;
    RefPtr<XMLDocumentParser> m_XMLParser;
};

TEST_P(XMLDocumentParserTest, ParseFile)
{
    XMLDocumentParserTestData data = GetParam();

    if (data.m_fileName) {
        fprintf(stdout, "Running XMLDocumentParser test with file %s\n",
                data.m_fileName);
        bool read = ReadFile(data.m_fileName);
        ASSERT_TRUE(read);
    }
    // Parse
    Parse();

    ASSERT_TRUE(GetDocument());
    // Now check results
    bool good = WellFormed();
    // Encoding
    String encoding = GetDocument()->xmlEncoding();
    // Version
    String version = GetDocument()->xmlVersion();
    // Declaration
    bool declaration = GetDocument()->hasXMLDeclaration();
    // Document type
    DocumentType* type = GetDocument()->doctype();
    String typeName("");
    if (type)
        typeName = type->name();

    EXPECT_EQ(data.m_good, good);
    EXPECT_EQ(data.m_encoding.isEmpty(), encoding.isEmpty());
    if (!data.m_encoding.isEmpty())
        EXPECT_EQ(data.m_encoding, encoding);
    EXPECT_EQ(data.m_version.isEmpty(), version.isEmpty());
    if (!data.m_version.isEmpty())
        EXPECT_EQ(data.m_version, version);
    EXPECT_EQ(data.m_docType, typeName);
    EXPECT_EQ(data.m_declaration, declaration);

    if (good) {
        // Check Node related results
        int nodeCount = TotalNodeCount(GetDocument());
        int targetCount = TargetNodeCount(GetDocument());

        EXPECT_EQ(data.m_nodeCount, nodeCount);
        EXPECT_EQ(data.m_testTargetCount, targetCount);

        // Check root node
        Node* root = FindRootNode(GetDocument());
        EXPECT_NE(static_cast<Node*>(NULL), root);
        if (root)
            EXPECT_EQ(data.m_rootNode, root->nodeName());

        // Flatten node names and values to string
        String flatten = FlattenNodes(GetDocument());
        EXPECT_EQ(data.m_flatten, flatten);
    }
}

// This test is for WebCore::parseAttributes which parses attributes
// directly without creating an XMLDocumentParser object.
TEST_F(XMLDocumentParserTest, ParseAttribute)
{
    String goodStr = "attribute1='value1' attribute2='value2'";
    // Change 'value1' to 'value1 to create a bad attribute string
    String badStr = "attribute1='value1 attribute2='value2'";
    bool ok;
    HashMap<String, String> attributes;
    attributes = parseAttributes(goodStr, ok);
    EXPECT_TRUE(ok);
    EXPECT_FALSE(attributes.isEmpty());
    EXPECT_EQ(2, attributes.size());
    EXPECT_EQ("value1", attributes.get("attribute1"));
    EXPECT_EQ("value2", attributes.get("attribute2"));

    attributes = parseAttributes(badStr, ok);
    EXPECT_FALSE(ok);
    EXPECT_TRUE(attributes.isEmpty());
}

}  // namespace

// Use INSTANTIATE_TEST_CASE_P to instantiate the test cases.
INSTANTIATE_TEST_CASE_P(XMLDocumentParserTestInstance,
                        XMLDocumentParserTest,
                        ::testing::ValuesIn(s_paserTestData));
