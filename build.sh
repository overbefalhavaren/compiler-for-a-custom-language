source "$(dirname "$0")/env.sh"

if [-n -d "$(dirname "$0")/build"]; then
    echo "-- Commencing setup..."
    if [-n bash "$(dirname "$0")/setup.sh"]; then
        exit 1
    fi
    echo "-- Setup successfull."
fi

echo "-- Starting compilation process..."
cmake --build build --config "$CMAKE_MODE"
