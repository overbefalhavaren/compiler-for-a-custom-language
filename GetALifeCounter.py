import os

cxx_linecount: int = 0
cmake_linecount: int = 0
python_linecount: int = 0

def find_base_directory() -> str:
    return os.path.dirname(__file__)

def count_lines(path: str) -> int:
    with open(path, 'r') as f:
        return sum(1 for _ in f)
    
def recurse_count_dir(dir: str) -> None:
    global cxx_linecount, cmake_linecount, python_linecount

    for file in os.listdir(dir):
        if os.path.isdir(os.path.join(dir, file)):
            recurse_count_dir(os.path.join(dir, file))
        elif file.endswith(".hpp") or file.endswith("cpp"):
            cxx_linecount += count_lines(os.path.join(dir, file))
        elif file.endswith(".py"):
            python_linecount += count_lines(os.path.join(dir, file))
        elif file == "CmakeLists.txt":
            cmake_linecount += count_lines(os.path.join(dir, file))

def main() -> None:
    global cxx_linecount, cmake_linecount, python_linecount

    base: str = find_base_directory()
    for file in os.listdir(base):
        if file.endswith(".py"):
            python_linecount += count_lines(os.path.join(base, file))
        elif file == "CmakeLists.txt":
            cmake_linecount += count_lines(os.path.join(base, file))
    
    recurse_count_dir(os.path.join(base, "include"))
    recurse_count_dir(os.path.join(base, "src"))

    print("Project line count, not including tests.")
    print(f"Lines of C++: {cxx_linecount}")
    print(f"Lines of CMake: {cmake_linecount}")
    print(f"Lines of Python: {python_linecount}")
    print("Dude you need to get a life.")

if __name__ == "__main__":
    main()
