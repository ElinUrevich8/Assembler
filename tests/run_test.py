import unittest
from pathlib import Path

THIS_FILE = Path(__file__).resolve()
THIS_DIRECTORY = THIS_FILE.parent
assert THIS_DIRECTORY.name == "tests", "This script must be run from the 'tests' directory."
TESTS = THIS_DIRECTORY
CASES = TESTS / "cases"

def compile_assembler():
    """
    Compiles the assembler source code.
    This function assumes that the assembler source code is in the parent directory.
    """
    import subprocess

    SOURCE_CODE_DIRECTORY = THIS_DIRECTORY.parent
    assert (SOURCE_CODE_DIRECTORY / "Makefile").exists(), "Assembler source code directory must contain a Makefile."

    try:
        subprocess.run(
            ["make", "clean"],
            check=True,
            cwd=str(THIS_DIRECTORY.parent),  # Assuming the Makefile is in the parent directory
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE
        )
        subprocess.run(
            ["make"],
            check=True,
            cwd=str(THIS_DIRECTORY.parent),  # Assuming the Makefile is in the parent directory
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE
        )
    except subprocess.CalledProcessError as e:
        print("Compilation failed with error:\n" + e.stderr.decode())
        raise e


def execute_assembler(input_file, allow_failure=False):
    """
    Executes the assembler on the given input file.
    If allow_failure is True, do not raise on non-zero exit.
    Returns (returncode, stdout, stderr)
    """
    import subprocess
    try:
        result = subprocess.run(
            ["./assembler", str(input_file)],
            check=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE
        )
        return result.returncode, result.stdout, result.stderr
    except subprocess.CalledProcessError as e:
        if allow_failure:
            return e.returncode, e.stdout, e.stderr
        print("Assembler failed with error:\n" + e.stderr.decode())
        raise e

def cleanup_test():
    # iterate recursively on TESTS directory and delete any test.am file
    for file in TESTS.rglob("test.am"):
        try:
            file.unlink()
        except Exception as e:
            print("Failed to delete {}: {}".format(file, e))
            raise e



def run_test(test_name, test_directory, is_expected_failure=False):
    input_file_path = test_directory / "test.as"
    expected_output_file_path = test_directory / "expected.am"
    actual_output_file_path = test_directory / "test.am"

    cleanup_test()

    try:
        input_file = Path(input_file_path)

        if not input_file.exists():
            raise FileNotFoundError("Input file {} does not exist.".format(input_file))

        if not input_file.is_file():
            raise ValueError("Input path {} is not a file.".format(input_file))

        if not input_file.suffix == '.as':
            raise ValueError("Input file {} must have a .as extension.".format(input_file))

        expected_output_file = Path(expected_output_file_path)
        actual_output_file = Path(actual_output_file_path)

        if is_expected_failure:
            # Allow assembler to fail, capture stderr
            returncode, stdout, stderr = execute_assembler(input_file, allow_failure=True)
            # Assert that the assembler actually failed
            assert returncode != 0, "Test {}: Expected assembler to fail, but it succeeded.".format(test_name)
            expected_err_file = test_directory / "expected.err"
            if expected_err_file.exists():
                expected_err = expected_err_file.read_text().strip()
                actual_err = stderr.decode().strip()
                assert actual_err == expected_err, \
                    "Test {} failed: error output does not match expected.\n\nGot: {}\n\nExpected: {}".format(
                        test_name, actual_err, expected_err)
                return
            else:
                raise FileNotFoundError("Expected error file {} does not exist.".format(expected_err_file))
        else:
            # Success expected, fail if assembler fails
            returncode, stdout, stderr = execute_assembler(input_file, allow_failure=False)
            assert returncode == 0, "Test {}: Assembler failed unexpectedly.".format(test_name)
            actual_output_file_path = input_file.with_suffix('.am')
            if not actual_output_file_path.exists():
                raise FileNotFoundError("Actual output file {} does not exist.".format(actual_output_file_path))
            if not expected_output_file.exists():
                raise FileNotFoundError("Expected output file {} does not exist.".format(expected_output_file))
            assert actual_output_file_path.read_text().strip() == expected_output_file.read_text().strip(), \
                "Test {} failed: output does not match expected content.\n\nGot: {}\n\nExpected: {}".format(
                    test_name, actual_output_file_path.read_text().strip(), expected_output_file.read_text().strip())
    finally:
        cleanup_test()
def discover_test_cases():
    for test_dir in CASES.rglob("*"):
        test_as = test_dir / "test.as"
        if test_as.exists():
            expected_err_file = test_dir / "expected.err"
            is_expected_failure = expected_err_file.exists()
            test_name = str(test_dir.relative_to(CASES))
            yield test_name, test_dir, is_expected_failure

def make_test_function(test_name, test_directory, is_expected_failure):
    def test(self):
        run_test(test_name, test_directory, is_expected_failure)
    return test

class DynamicAssemblerTests(unittest.TestCase):
    pass

# Dynamically add test methods
for idx, (test_name, test_directory, is_expected_failure) in enumerate(discover_test_cases()):
    test_func = make_test_function(test_name, test_directory, is_expected_failure)
    test_func.__name__ = "test_{:09d}_{}".format(
        idx, test_name.replace('/', '_').replace(' ', '_')
    )
    setattr(DynamicAssemblerTests, test_func.__name__, test_func)

if __name__ == "__main__":
    compile_assembler()
    unittest.main()
else:
    print("This script is intended to be run directly, not imported as a module.")
    print("Please run it from the command line or an appropriate test runner.")