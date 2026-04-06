import pytest
import subprocess

def run_chip_tool(*args, ignore_errors=False):
    """Executes chip-tool commands and returns the output."""
    cmd =[CHIP_TOOL_PATH] + list(args)
    print(f"\n[CHIP-TOOL] {' '.join(cmd)}")
    
    result = subprocess.run(cmd, capture_output=True, text=True)
    output = result.stdout + result.stderr
    
    if result.returncode != 0 and not ignore_errors:
        pytest.fail(f"chip-tool failed (Code {result.returncode}).\nOutput:\n{output}")
        
    return output