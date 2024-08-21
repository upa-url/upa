#!/usr/bin/env python3
#
# Copyright 2023-2024 Rimas Misevičius
# Distributed under the BSD-style license that can be
# found in the LICENSE file.
from enum import auto, IntEnum
import os
import sys

INDENT_TEXT = "    "
COMMON_CPP_HEADER = """
#include \"upa/url.h\"
#include <iostream>
#include <string>
"""

def has_main(cpp_text):
    return "int main(" in cpp_text

class CppExamples:
    class State(IntEnum):
        Outside = auto()
        EnterCpp = auto()
        InCpp = auto()
        InCppSnipet = auto()

    def __init__(self, out_path):
        self._cpp_file_num = 0
        self._cpp_example_num = 0
        self._cpp_examples_text = COMMON_CPP_HEADER
        self._out_path = out_path

    def extract_cpp_from_md_file(self, md_path):
        state = self.State.Outside
        cpp_text = ""

        # Open the MD file
        print("Processing Markdown file:", md_path)
        with open(md_path, "r") as file:
            for line_nl in file:
                # remove ending newline character
                line = line_nl.rstrip()
                if state == self.State.Outside:
                    if line == "```cpp":
                        state = self.State.EnterCpp;
                        cpp_text = ""
                elif line == "```":
                    if state == self.State.InCpp:
                        if has_main(cpp_text):
                            self._cpp_file_num += 1
                            out_file_path = os.path.join(self._out_path, f"example-{self._cpp_file_num}.cpp")
                            print(" creating:", out_file_path)
                            with open(out_file_path, "w") as out_file:
                                out_file.write(f"// Example from: {md_path}\n")
                                out_file.write(cpp_text)
                    elif state == self.State.InCppSnipet:
                        self._cpp_example_num += 1
                        self._cpp_examples_text += f"\n// Example from: {md_path}\n"
                        self._cpp_examples_text += f"void example_{self._cpp_example_num}() {{\n"
                        self._cpp_examples_text += cpp_text
                        self._cpp_examples_text += "}\n"
                    state = self.State.Outside;
                else:
                    if state == self.State.EnterCpp:
                        state = self.State.InCpp if line.startswith("#include") else self.State.InCppSnipet
                    if state == self.State.InCppSnipet:
                        cpp_text += INDENT_TEXT
                    cpp_text += line_nl

    def output_common_cpp(self):
        if self._cpp_example_num > 0:
            # create main()
            self._cpp_examples_text += "\nint main() {\n"
            for num in range(1, self._cpp_example_num + 1):
                self._cpp_examples_text += f"{INDENT_TEXT}example_{num}();\n"
            self._cpp_examples_text += f"{INDENT_TEXT}return 0;\n"
            self._cpp_examples_text += "}\n"
            # save code
            out_file_path = os.path.join(self._out_path, "examples.cpp")
            print(" creating:", out_file_path)
            with open(out_file_path, "w") as out_file:
                out_file.write(self._cpp_examples_text)


if __name__ == "__main__":
    # Command line arguments
    if len(sys.argv) >= 3:
        out_path = sys.argv[1]
        md_files = sys.argv[2:]

        print("Output directory:", out_path)
        cpp_examples = CppExamples(out_path)
        for md_path in md_files:
            cpp_examples.extract_cpp_from_md_file(md_path)
        cpp_examples.output_common_cpp()
    else:
        app_name = os.path.basename(os.path.basename(__file__))
        print(f"Usage: {app_name} <output dir> <markdown file> ...",
            file=sys.stderr)
        sys.exit(1)
