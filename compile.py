import subprocess, os

RECOMPILE_CMAKE: bool = True

def find_base_directory() -> str:
    return os.path.dirname(__file__)

def get_posix(path: str) -> str:
    return '/'.join(path.split('\\'))

def main() -> None:
    dirpath = find_base_directory()
    print(dirpath)

    build_fouler_path: str = os.path.join(dirpath, "build")
    if not os.path.exists(build_fouler_path):
        print(f"Could not find the build foulder: '{get_posix(dirpath)}/build/'")
        return
    
    subprocess.run(f"cd {dirpath}", shell=True)
    subprocess.run("cd build", shell=True)
    subprocess.run("cmake .. -G \"Ninja\"")
    subprocess.run("cmake --build . --config Debug", shell=True)

    # subprocess.run("", shell=True)

if __name__ == "__main__":
    main()
