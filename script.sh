#!/bin/bash

echo "🔧 Fixing dyld cache issue..."

# 1. First, try updating the dyld shared cache
echo "=== Updating dyld shared cache ==="
sudo update_dyld_shared_cache -force

# 2. If that doesn't work, try resetting it
echo -e "\n=== Resetting dyld cache ==="
sudo rm -rf /private/var/db/dyld/*
sudo update_dyld_shared_cache -force

# 3. Reboot recommendation
echo -e "\n⚠️  IMPORTANT: You should reboot your Mac after running this script"
echo "The dyld cache issue requires a system restart to fully resolve."

# 4. Test with a simple program
echo -e "\n=== Testing with simple program ==="
cat > test_dyld.c << 'EOF'
#include <stdio.h>
int main() {
    printf("Hello from test\n");
    return 0;
}
EOF

clang -o test_dyld test_dyld.c
if ./test_dyld 2>/dev/null; then
    echo "✅ Simple test program works!"
else
    echo "❌ Still broken. You MUST reboot your Mac."
fi

rm -f test_dyld test_dyld.c

echo -e "\n📝 Next steps:"
echo "1. Reboot your Mac (essential!)"
echo "2. After reboot, try compiling shtick again:"
echo "   cd /path/to/shtick"
echo "   make clean"
echo "   make"
echo "   ./shtick"