source "$(dirname "$0")/env.sh"

if [[ -z "$LLVM_CONFIG" ]]; then
    echo "'LLVM_CONFIG' not set."
    exit 1
fi

if [[ -z "$CMAKE_GENERATOR" ]]; then
    echo "'CMAKE_GENERATOR' not set."
    exit 1
fi

if [[ -z "$CMAKE_MODE" ]]; then
    echo "'CMAKE_MODE' not set."
    exit 1
fi

build_foulder="$(dirname "$0")/build"
if [[ -d "$build_foulder" ]]; then
    rm -rf "$build_foulder"
fi

llvm_cmake="$("$LLVM_CONFIG" --cmakedir)"
echo "-- LLVM CMake dir found: '$llvm_cmake'"

echo "-- Creating build files..."
mkdir "$build_foulder"

cmake -S . -B build -G "$CMAKE_GENERATOR" \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
    -DLLVM_DIR="$llvm_cmake"
