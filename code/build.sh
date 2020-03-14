echo "Building..."

CommonCompilerFlags="-g -ggdb -std=c++11 -msse4.1 -ffast-math -Wno-braced-scalar-init -Wno-format -Wno-writable-strings -Wno-switch -Wno-unused-value"
CommonDefines="-DCOMPILE_INTERNAL=1 -DCOMPILE_SLOW=1"
CommonLinkerFlags="-pthread -lX11 -ldl -lGL"

curDir=$(pwd)
buildDir="$curDir/../build"
dataDir="$curDir/../data"

pushd $buildDir > /dev/null

clang++ $CommonCompilerFlags $CommonDefines "$curDir/trapped.cpp" -o trapped $CommonLinkerFlags

popd > /dev/null
