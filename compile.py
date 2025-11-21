import os, shutil

# Wether or not to compile in Debug mode. True = Debug, False = Release
COMPILE_DEBUG: bool = True

# Which compiler and cmake generator to use. Supported options are:
# "clang": Uses the clang++ compiler with the Ninja cmake generator. Might 
#          give error since the llvm library is usually compiled for msvc 
#          which is less strict. We use clang for nicer error messages.
# "msvc": Uses the MSVC compiler with the Visual Studio 17 2022 cmake 
#         generator. Use for production unless you've specifically made 
#         sure llvm is compiled using clang.
COMPILER: str = "msvc" # "clang" or "msvc"
 
def find_base_directory() -> str:
    return os.path.dirname(__file__)

def get_posix(path: str) -> str:
    return '/'.join(path.split('\\'))

def main() -> None:
    dirpath: str = find_base_directory()

    build_fouler_path: str = os.path.join(dirpath, "build")

    print("-- Removing old build filesb")
    shutil.rmtree(build_fouler_path)

    print("-- Creating build foulder")
    os.mkdir(build_fouler_path)
    
    mode: str = "Debug" if COMPILE_DEBUG else "Release"
    if COMPILER == "clang":
        print("-- Starting build using Ninja with clang and clang++")
        os.system("cmake -S . -B build -G \"Ninja\""
                  " -DCMAKE_C_COMPILER=clang"
                  " -DCMAKE_CXX_COMPILER=clang++")
    elif COMPILER == "msvc":
        print("-- Starting build using Visual Studio 17 2022 and msvc")
        os.system("cmake -S . -B build -G \"Visual Studio 17 2022\"")
    else:
        print(f"Can't start build because an invalid compiler was specified: \"{COMPILER}\"")
        return

    print("\n-- Starting compilation process...\n")
    os.system(f"cmake --build build --config {mode}")

if __name__ == "__main__":
    main()
