#!/usr/bin/env python3
import re

# Read the file
with open('app/search/SearchValidator.cpp', 'r', encoding='utf-8') as f:
    content = f.read()

# Define the replacements
replacements = [
    (r'const QRegularExpression repeatedGreedy\(\s*R"\(\)"[^\)]*\);',
     r'static const QRegularExpression repeatedGreedy("\\([^)]*[\\.\\*\\+][^)]*\\)\\{[0-9]+,\\}");'),
    
    (r'if \(pattern\.contains\(repeatedGreedy\)\)',
     r'if (repeatedGreedy.match(pattern).hasMatch())'),
    
    (r'const QRegularExpression sequentialGreedy\(\s*R"\(\)"[^\)]*\);',
     r'static const QRegularExpression sequentialGreedy("\\([^)]*\\.\\*[^)]*\\)[^(]*\\.\\*[^(]*\\([^)]*\\.\\*[^)]*\\)");'),
    
    (r'if \(pattern\.contains\(sequentialGreedy\)\)',
     r'if (sequentialGreedy.match(pattern).hasMatch())'),
    
    (r'const QRegularExpression lookaroundQuant\(\s*R"\(\)"[^\)]*\);',
     r'static const QRegularExpression lookaroundQuant("\\(\\?[=!<][^)]*\\)[*+?{]");'),
    
    (r'if \(pattern\.contains\(lookaroundQuant\)\)',
     r'if (lookaroundQuant.match(pattern).hasMatch())'),
    
    (r'const QRegularExpression unicodeQuant\(\s*R"\(\)"[^\)]*\);',
     r'static const QRegularExpression unicodeQuant("\\\\[pP]\\{[^}]+\\}[*+]\\{[0-9]+,\\}");'),
    
    (r'if \(pattern\.contains\(unicodeQuant\)\)',
     r'if (unicodeQuant.match(pattern).hasMatch())'),
    
    (r'const QRegularExpression backrefExplosion\(\s*R"\(\)"[^\)]*\);',
     r'static const QRegularExpression backrefExplosion("\\\\[0-9]+[*+]\\{[0-9]+,\\}");'),
    
    (r'if \(pattern\.contains\(backrefExplosion\)\)',
     r'if (backrefExplosion.match(pattern).hasMatch())'),
]

# Apply replacements
for pattern, replacement in replacements:
    content = re.sub(pattern, replacement, content, flags=re.DOTALL)

# Write the file back
with open('app/search/SearchValidator.cpp', 'w', encoding='utf-8', newline='\n') as f:
    f.write(content)

print("File updated successfully!")

