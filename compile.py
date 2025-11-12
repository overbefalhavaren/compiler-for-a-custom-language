import subprocess, os

def get_posix(path: str) -> str:
    return '/'.join(path.split('\\'))

def main() -> None:
    dirpath = os.path.dirname(__file__)

    build_fouler_path: str = os.path.join(dirpath, "build")
    if not os.path.exists(build_fouler_path):
        print(f"Could not find the build foulder: '{get_posix(dirpath)}/build/'")
        return

if __name__ == "__main__":
    main()
