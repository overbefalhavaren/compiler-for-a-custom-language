import os

RECOMPILE_CMAKE: bool = True
# CMAKE_GENERATOR: str = "Ninja" # "Visual Studio 17 2022" # Deprecated since it may break stuff if changed
COMPILE_DEBUG: bool = True

def find_base_directory() -> str:
    return os.path.dirname(__file__)

def get_posix(path: str) -> str:
    return '/'.join(path.split('\\'))

def main() -> None:
    dirpath: str = find_base_directory()

    build_fouler_path: str = os.path.join(dirpath, "build")
    if RECOMPILE_CMAKE and os.path.exists(build_fouler_path):
        os.remove(build_fouler_path)

    if RECOMPILE_CMAKE or not os.path.exists(build_fouler_path):
        # print(f"Could not find the build foulder: '{get_posix(dirpath)}/build/'")
        # return

        print("The 'build' module containing the cmake build files was not found. Creating one automatically.")
        os.makedirs(build_fouler_path)

    # print(build_fouler_path)

    mode: str = "Debug" if COMPILE_DEBUG else "Release"
    os.system(f"cmake -S . -B build")# -G \"Ninja\"")
            #   " -DCMAKE_C_COMPILER=clang-cl"
            #   " -DCMAKE_CXX_COMPILER=clang-cl")

    os.system(f"cmake --build build --config {mode}")

    # subprocess.run(["cmd",# 'cmake -S . -B build', "cmake --build build"])
    #                 f"echo cd {build_fouler_path}", 
    #                 "echo cmake .. -G \"Ninja\""
    #                 ])

    # print(build_fouler_path)

    # subprocess.run(['cmd',f"cd {build_fouler_path}"])#[f"cd {build_fouler_path}", "cmake .. -G \"Ninja\""])

    # subprocess.run(f"cd {dirpath}", shell=True)
    # subprocess.run("cmake .. -G \"Ninja\"", shell=True)
    # subprocess.run("cmake --build . --config Debug", shell=True)
    # subprocess.run("", shell=True)

if __name__ == "__main__":
    main()
