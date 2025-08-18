#!/usr/bin/env python3
"""
Fix encoding issues in source files
"""

import sys
from pathlib import Path

def fix_file_encoding():
    """Fix encoding issues in error_macros.h"""
    
    script_dir = Path(__file__).parent
    target_file = script_dir.parent.parent / "System" / "error_macros.h"
    
    print(f"🔧 Fixing encoding in: {target_file}")
    
    # Read file with different encodings to find the issue
    encodings_to_try = ['utf-8', 'cp1252', 'iso-8859-1', 'latin1']
    
    content = None
    working_encoding = None
    
    for encoding in encodings_to_try:
        try:
            with open(target_file, 'r', encoding=encoding) as f:
                content = f.read()
            working_encoding = encoding
            print(f"✅ Successfully read with {encoding}")
            break
        except UnicodeDecodeError as e:
            print(f"❌ Failed with {encoding}: {e}")
    
    if content is None:
        print("❌ Could not read file with any encoding!")
        return False
    
    # Look for problematic characters
    print(f"\n🔍 Analyzing content...")
    problematic_chars = []
    for i, char in enumerate(content):
        if ord(char) > 127:  # Non-ASCII character
            problematic_chars.append((i, ord(char), char))
    
    if problematic_chars:
        print(f"⚠️  Found {len(problematic_chars)} non-ASCII characters:")
        for pos, code, char in problematic_chars[:10]:  # Show first 10
            print(f"   Position {pos}: byte {code} ('{char}')")
    else:
        print("✅ No problematic characters found")
    
    # Create a cleaned version
    backup_file = target_file.with_suffix('.h.backup')
    
    try:
        # Backup original
        with open(backup_file, 'wb') as f:
            with open(target_file, 'rb') as original:
                f.write(original.read())
        print(f"💾 Backup saved: {backup_file}")
        
        # Write clean UTF-8 version
        clean_content = content.encode('utf-8', errors='ignore').decode('utf-8')
        with open(target_file, 'w', encoding='utf-8') as f:
            f.write(clean_content)
        
        print(f"✅ File cleaned and saved as UTF-8")
        return True
        
    except Exception as e:
        print(f"❌ Error fixing file: {e}")
        return False

if __name__ == "__main__":
    print("🛠️  File Encoding Fixer")
    print("=" * 30)
    
    success = fix_file_encoding()
    
    if success:
        print("\n✅ File encoding fixed!")
        print("💡 Now try running the analysis again")
    else:
        print("\n❌ Could not fix encoding")
    
    input("\nPress Enter to continue...")