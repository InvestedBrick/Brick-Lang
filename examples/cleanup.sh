#!/bin/bash

# Delete all object files (*.o)
find . -type f -name "*.o" -exec rm -f {} +

# Delete all assembly files (*.s, *.asm)
find . -type f -name "*.s" -exec rm -f {} +
find . -type f -name "*.asm" -exec rm -f {} +

# Delete all executable files (files without an extension and marked as executable)
find . -type f -executable -not -name "*.*" -exec rm -f {} +

echo "Cleanup complete."
