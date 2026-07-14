# src/git_utils.py
import sys
import subprocess

def get_git_diff(compare_expression=None):
    """Retrieves the unified git diff. If compare_expression is provided, compares two branches."""
    try:
        repo_root = subprocess.check_output(
            ["git", "rev-parse", "--show-toplevel"], 
            encoding="utf-8"
        ).strip()
        
        cmd = ["git", "diff"]
        if compare_expression:
            cmd.append(compare_expression)
        else:
            cmd.append("HEAD")
            
        diff = subprocess.check_output(cmd, cwd=repo_root, encoding="utf-8")
        return diff.strip()
    except subprocess.CalledProcessError as e:
        print(f"❌ Failed to get git diff: {e}")
        sys.exit(1)