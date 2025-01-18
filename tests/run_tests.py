import subprocess
import os

script_dir = os.path.dirname(os.path.abspath(__file__))
class bcolors:
    HEADER = '\033[95m'
    OKBLUE = '\033[94m'
    OKCYAN = '\033[96m'
    OKGREEN = '\033[92m'
    WARNING = '\033[93m'
    FAIL = '\033[91m'
    ENDC = '\033[0m'
    BOLD = '\033[1m'
    UNDERLINE = '\033[4m'


class Tester:
    def __init__(self):
        self.total_programs = 0
        self.n_failed = 0
        self.failed_tests = []
        self.files = []
        self.expected_return_codes = []
        self.expected_stdouts = []
        self.expected_stderrs = []

    def add_test(self,filepath : str,expected_return_code : int = 0, expected_stdout : str = None, expected_stderr : str = None):
        self.total_programs += 1
        self.files.append(filepath)
        self.expected_return_codes.append(expected_return_code)
        self.expected_stdouts.append(expected_stdout)
        self.expected_stderrs.append(expected_stderr)

    def compile_program(self, pathname: str):
        try:
            abs_path = os.path.join(script_dir, pathname)  
            print(f"Compiling '{abs_path}' ...")
            result = subprocess.run(
                [os.path.join(script_dir, "../brick"), abs_path],
                capture_output=True, text=True)
            if result.returncode != 0:
                print(f"Compilation of '{pathname}' was not successful")
                return 0, result.stderr
            print(f"Compilation of '{pathname}' was successful")
            return 1, None
        except FileNotFoundError:
            return 0, f"File '{pathname}' not found"

    def run_program(self, pathname: str):
        try:
            abs_path = os.path.join(script_dir, pathname)  
            print(f"Executing '{abs_path}' ...")
            result = subprocess.run([ abs_path], capture_output=True, text=True)
            return result.returncode, result.stdout, result.stderr
        except FileNotFoundError:
            print(f"Error: Binary '{pathname}' not found")
            return -1, "", "Binary not found"
    
    def cleanup(self):
        subprocess.run(["bash", os.path.join(script_dir, "cleanup.sh")])

    def run_tests(self):
        n_successfully_compiled = 0
        for file in self.files:
            n,stderr = self.compile_program(file)
            n_successfully_compiled += n
            if stderr != None:
                print("The following error message occured: ")
                print("")
                print(bcolors.FAIL + stderr + bcolors.ENDC)
                print("")
                print(bcolors.FAIL + "Exiting Testing..." + bcolors.ENDC)
                exit(1)
        print(f"Compiled {n_successfully_compiled} / {len(self.files)} files successfully")        
        if n_successfully_compiled == len(self.files):
            print("\n" + bcolors.OKGREEN + "Successfully compiled all files!" + bcolors.ENDC+ "\n")
        for i in range(len(self.files)):
            ret_code,stdout,stderr = self.run_program(self.files[i][:-6])
            if ret_code != self.expected_return_codes[i] or \
            (self.expected_stdouts[i] != None and stdout != self.expected_stdouts[i]) or\
            (self.expected_stderrs[i] != None and stderr != self.expected_stderrs[i]):
                if ret_code != self.expected_return_codes[i]:
                    print(ret_code, " : ", self.expected_return_codes[i])
                self.n_failed += 1
                self.failed_tests.append(self.files[i])
                print(bcolors.FAIL + f"'{self.files[i]}' failed..." + bcolors.ENDC)
                continue
            print(bcolors.OKGREEN + f"'{self.files[i]}' passed..." + bcolors.ENDC)
        
            print("")
        if self.n_failed == 0:
            print(bcolors.OKGREEN + f"ALL {len(self.files)} TESTS PASSED!" + bcolors.ENDC)
            print("")
        else:
            print(bcolors.FAIL + f"{self.n_failed} TESTS FAILED!" + bcolors.ENDC)
            print("")
            print("Failed tests are:")
            for test in self.failed_tests:
                print(bcolors.FAIL + test + bcolors.ENDC)


tester = Tester()

tester.add_test("var_dec.brick",expected_return_code=3)
tester.add_test("var_set.brick",expected_return_code=13)
tester.add_test("global_vars.brick",expected_return_code=36)
tester.add_test("output.brick",expected_stdout="Hello, World!\0")
tester.add_test("fib.brick",expected_return_code=15)
tester.add_test("strings.brick",expected_stdout="Variant 1\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00variant 2")
tester.add_test("control.brick",expected_return_code=1)
tester.add_test("structs.brick",expected_return_code=57)
tester.add_test("include.brick",expected_return_code=144)
tester.add_test("bitshift.brick", expected_return_code=15)
tester.add_test("logic_test.brick",expected_return_code=3)

tester.run_tests()
if tester.n_failed == 0:
    tester.cleanup()