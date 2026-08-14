#!/usr/bin/env python3
import os
import re
import sys

ROOT_DIR = os.path.abspath(os.path.join(os.path.dirname(__file__), "../.."))
DOC_DIR = os.path.join(ROOT_DIR, "doc")
BIN_DIR = os.path.join(DOC_DIR, "bin")
ARCH_DIR = os.path.join(DOC_DIR, "architecture")
BUG_DIR = os.path.join(ARCH_DIR, "bug")

# Set of allowed tools/programs for doc/bin
ALLOWED_BIN_DOCS = {
    "ar45.md", "ca45.md", "cc45.md", "cp45.md", "cvt_asm.md",
    "disk45.md", "ln45.md", "nm45.md", "objdump45.md", "CONFIGURATION.md"
}

def check_structure():
    errors = []

    # 1. Check doc/ root folder (only README.md and subdirs bin, architecture are allowed)
    for entry in os.scandir(DOC_DIR):
        if entry.is_file():
            if entry.name != "README.md":
                errors.append(f"File '{entry.name}' should not be in the doc/ root. Please move it to doc/bin/ or doc/architecture/.")
        elif entry.is_dir():
            if entry.name not in {"bin", "architecture"}:
                errors.append(f"Unexpected subdirectory '{entry.name}' in doc/. Only 'bin' and 'architecture' are allowed.")

    # 2. Check doc/bin folder structure
    if not os.path.exists(BIN_DIR):
        errors.append("doc/bin/ directory does not exist.")
    else:
        for entry in os.scandir(BIN_DIR):
            if entry.is_file():
                if entry.name not in ALLOWED_BIN_DOCS:
                    errors.append(f"File 'doc/bin/{entry.name}' is not a recognized program or configuration documentation. Allowed files: {', '.join(sorted(ALLOWED_BIN_DOCS))}")
            elif entry.is_dir():
                errors.append(f"Subdirectory 'doc/bin/{entry.name}' is not allowed inside doc/bin/.")

    # 3. Check doc/architecture folder structure
    if not os.path.exists(ARCH_DIR):
        errors.append("doc/architecture/ directory does not exist.")
    else:
        for entry in os.scandir(ARCH_DIR):
            if entry.is_file():
                if not entry.name.endswith(".md"):
                    errors.append(f"Non-markdown file 'doc/architecture/{entry.name}' is not allowed in doc/architecture/.")
            elif entry.is_dir():
                if entry.name != "bug":
                    errors.append(f"Unexpected subdirectory 'doc/architecture/{entry.name}' in doc/architecture/. Only 'bug/' is allowed.")

    # 4. Check doc/architecture/bug folder structure
    if os.path.exists(BUG_DIR):
        for entry in os.scandir(BUG_DIR):
            if entry.is_file():
                if not entry.name.endswith(".md"):
                    errors.append(f"Non-markdown file 'doc/architecture/bug/{entry.name}' is not allowed in doc/architecture/bug/.")
            elif entry.is_dir():
                errors.append(f"Subdirectory 'doc/architecture/bug/{entry.name}' is not allowed inside doc/architecture/bug/.")

    return errors

def validate_links(file_path):
    errors = []
    with open(file_path, "r", encoding="utf-8") as f:
        content = f.read()

    # Markdown links format: [link text](url) or ![image](url)
    links = re.findall(r'!?\[.*?\]\((.*?)\)', content)
    file_dir = os.path.dirname(file_path)

    for link in links:
        # Ignore external links, mailto links, and anchor-only links
        if link.startswith(("http://", "https://", "mailto:", "#")):
            continue

        # Strip anchor suffix if present
        link_file = link.split("#")[0]
        if not link_file:
            continue

        # Check if the target file exists relative to the current markdown file
        target_path = os.path.abspath(os.path.join(file_dir, link_file))
        if not os.path.exists(target_path):
            rel_source = os.path.relpath(file_path, ROOT_DIR)
            errors.append(f"Broken link in '{rel_source}': '{link}' (resolved to '{target_path}') does not exist.")

    return errors

def main():
    errors = check_structure()

    # Walk doc/ and check all markdown files for link sanity
    for root, dirs, files in os.walk(DOC_DIR):
        for file in files:
            if file.endswith(".md"):
                file_path = os.path.join(root, file)
                errors.extend(validate_links(file_path))

    if errors:
        print("Documentation validation FAILED with the following errors:", file=sys.stderr)
        for err in errors:
            print(f"  - {err}", file=sys.stderr)
        sys.exit(1)
    else:
        print("Documentation validation PASSED.")
        sys.exit(0)

if __name__ == "__main__":
    main()
