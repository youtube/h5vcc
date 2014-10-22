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

#include "lb_heads_up_display.h"

#include <map>

#include "external/chromium/base/logging.h"
#include "external/chromium/base/stringprintf.h"
#include "external/chromium/base/string_tokenizer.h"

#include "lb_shell_console_values_hooks.h"

#if defined(__LB_SHELL__ENABLE_CONSOLE__)

namespace LB {

namespace {
void DCHECK_WITHIN_INCLUSIVE_RANGE(float x, float a, float b) {
  DCHECK_GE(x, a);
  DCHECK_LE(x, b);
}
}  // namespace

HeadsUpDisplay::HeadsUpDisplay(
    TextPrinter* text_printer,
    float left, float top, float right, float bottom) {
  text_printer_ = text_printer;

  DCHECK_WITHIN_INCLUSIVE_RANGE(left, -1.0f, 1.0f);
  DCHECK_WITHIN_INCLUSIVE_RANGE(top, -1.0f, 1.0f);
  DCHECK_WITHIN_INCLUSIVE_RANGE(right, -1.0f, 1.0f);
  DCHECK_WITHIN_INCLUSIVE_RANGE(bottom, -1.0f, 1.0f);
  DCHECK_LE(left, right);
  DCHECK_LE(bottom, top);

  left_ = left;
  top_ = top;
  right_ = right;
  bottom_ = bottom;
}

void HeadsUpDisplay::SetTextPrinter(TextPrinter* text_printer) {
  text_printer_ = text_printer;
}

namespace {  // Helper methods for rendering the console

// Constructs a CVal tree where CVal group names are the interior nodes
// and CVals are the leaf nodes.  Having the CVals in this form will make
// output formatting easier.
// Example input might be "[Sys.Mem.Used, Sys.Mem.Free, Sys.FPS, DOM.NumNodes]"
// In which case a tree will be created that looks like this:
// Root - Sys - Mem - Used
//      |     \ FPS \ Free
//      \ DOM - NumNodes
class CValGroupTree {
 public:
  // Construct the tree out of a list of CVals
  template <class InputIterator>
  CValGroupTree(InputIterator first, InputIterator last);

  // A node in the tree consists of a map from child name to child.  Each
  // child name may refer to a group or CVal.  Keys used in the ChildrenMap
  // cannot have any '.' characters in them, as these are already separated.
  struct Node {
    typedef std::map<std::string, Node*> ChildrenMap;

    ~Node();
    std::string full_name;  // Valid for leaf nodes only, includes '.'s.
    ChildrenMap children;
  };
  const Node* GetRoot() { return &root_; }

 private:
  Node root_;
};

template <class InputIterator>
CValGroupTree::CValGroupTree(InputIterator first, InputIterator last) {
  // Walk through the list of CVal names, adding each one, in turn, to the tree.
  // Create new CVal groups as we see them.
  for (InputIterator iter = first; iter != last; ++iter) {
    const std::string& cur_name = *iter;

    // Split each CVal based on its groups, and then walk through the groups
    // and the tree at the same time, trying to find our current named group
    // in the tree as we go, or creating it if we can't.
    StringTokenizer tokenizer(cur_name, ".");
    Node* cur_node = &root_;
    while (tokenizer.GetNext()) {
      // Try to insert the current group in to the tree, and then check if it
      // was already there.
      std::pair<Node::ChildrenMap::iterator, bool> inserted =
          cur_node->children.insert(
              std::pair<std::string, Node*>(tokenizer.token(), NULL));
      if (inserted.second) {
        // It turns out that this name was not already in the tree, so we must
        // now construct a new tree node to correspond to this current group
        // or CVal.
        Node* new_node = new Node;
        inserted.first->second = new_node;
      }

      // Step to the next node and continue to the next token
      cur_node = inserted.first->second;
    }

    // cur_node is now set to the leaf node for the CVal, so this is a good
    // time to record its full name in the tree.
    cur_node->full_name = cur_name;
  }
}

CValGroupTree::Node::~Node() {
  for (ChildrenMap::iterator iter = children.begin(); iter != children.end();
       ++iter) {
    delete iter->second;
  }
}

struct CValStringFormatOutput {
  std::string output;
  int nodes_printed;  // The number of nodes that were actually printed
};

// Create a nicely formatted string based on a subtree of grouped CVals.
// Each CVal will output as "CVAL_NAME: CVAL_VALUE".
// Each CVal group will output as "CVAL_GROUP_NAME: {CVAL_CHILDREN_OUTPUT}"
// If the CVal group contians only 1 child, it will instead output as
//     "CVAL_GROUP_NAME.CVAL_CHILD_OUTPUT"
// Multiple child outputs will be comma separated.
CValStringFormatOutput CreateStringForCValGroupNode(
    const CValGroupTree::Node* node) {
  CValStringFormatOutput ret;
  ret.nodes_printed = 0;
  // Iterate through each child of the given node, rendering each one to a
  // string.
  std::string next_string_chunk;  // Output for this child node
  for (CValGroupTree::Node::ChildrenMap::const_iterator iter =
           node->children.begin();
       iter != node->children.end(); ++iter) {
    next_string_chunk.clear();
    if (iter->second->children.empty()) {
      // This is a leaf node, it contains a value.  Get the value and output it.
      LB::ConsoleValueManager::ValueQueryResults value_info =
          LB::ConsoleValueManager::GetInstance()->
              GetValueAsPrettyString(iter->second->full_name);
      if (value_info.valid) {
        next_string_chunk = iter->first + ": " + value_info.value;
      }
    } else {
      // This is an internal node, so it is a group.  Recurse on this function
      // to produce output for it.
      CValStringFormatOutput child_ret =
          CreateStringForCValGroupNode(iter->second);
      if (child_ret.nodes_printed == 1) {
        // Compactly print groups with only one element in them
        next_string_chunk = iter->first + "." + child_ret.output;
      } else if (child_ret.nodes_printed > 0) {
        next_string_chunk = iter->first + ": {" + child_ret.output + "}";
      }
    }

    // We may not have produced output if the CVal was not valid or a
    // group was empty.  In that case, don't create any string.
    if (!next_string_chunk.empty()) {
      // Comma separate our values
      if (ret.nodes_printed > 0) {
        ret.output += ", ";
      }
      ret.output += next_string_chunk;
      ++ret.nodes_printed;
    }
  }
  return ret;
}

}  // namespace

void HeadsUpDisplay::InsertLineBreaks(std::string* line) {
  float line_width = right_ - left_;

  // This variable keeps track of nice places to line break as we traverse
  // through the string
  int last_good_break_point = 0;
  int last_line_break = 0;  // The last position we actually inserted a new line
  float cur_line_size = 0.0f;  // The current output width of the current line
  for (int i = 0; i < line->length(); ++i) {
    // Add the width of the next character to our total current line width
    cur_line_size += text_printer_->GetStringWidth(
        base::StringPiece(line->data()+i, 1));

    if (cur_line_size > line_width) {
      // We've gone over the line limit, lookup the last found good break
      // point and if it exists, put a newline there.
      if (last_good_break_point <= last_line_break) {
        // We have not been able to locate a good spot for a line break
        // since the last one, but we need one, so insert one anyway before
        // this current character.  This will shuffle the rest of the string
        // characters, but we'll accept that as this is an unlikely scenario
        // and only occurs on non-release builds.
        line->insert(i, "\n");
        last_good_break_point = i;
        ++i;  // Increment i as we've inserted a new character
      }
      last_line_break = last_good_break_point;
      (*line)[last_line_break] = '\n';
      // Reset the current line size to the new value after the line break
      cur_line_size = text_printer_->GetStringWidth(
          base::StringPiece(line->data()+last_line_break+1,
                            i-last_line_break));
    }

    // If we see ", ", mark the space as a good position for a line break
    if (i > 0 && (*line)[i-1] == ',' && (*line)[i] == ' ') {
      last_good_break_point = i;
    }
  }
}

void HeadsUpDisplay::Render() {
  if (!text_printer_)
    return;

  // Print the current URL as a special case, since it prints out to changing
  // lengths which cause distractions by changing the positions of the other
  // console values.
  const char* kCurrentURLCValName = "Nav.CurrentURL";
  LB::ConsoleValueManager::ValueQueryResults cur_url_info =
      LB::ConsoleValueManager::GetInstance()->
          GetValueAsPrettyString(kCurrentURLCValName);
  if (cur_url_info.valid) {
    text_printer_->Printf(
        left_, top_, "%s: %s", kCurrentURLCValName, cur_url_info.value.c_str());
  }

  // Get the set of CVals to print out
  std::set<std::string> cval_names =
      ShellConsoleValueHooks::GetInstance()->GetWhitelist()->GetOrderedValues();

  // Get a tree representation of the CVals and their groups
  CValGroupTree cval_group_tree(cval_names.begin(), cval_names.end());

  // Convert the CVal group tree in to a formatted string
  CValStringFormatOutput cval_output_string =
      CreateStringForCValGroupNode(cval_group_tree.GetRoot());

  // Add line breaks to the returned string so that it doesn't go off the edge
  // of the screen
  InsertLineBreaks(&cval_output_string.output);

  // Print the output to the screen
  text_printer_->Print(left_, top_ - text_printer_->GetLineHeight(),
                       cval_output_string.output.c_str());
}

}  // namespace LB

#endif  // defined(__LB_SHELL__ENABLE_CONSOLE__)
