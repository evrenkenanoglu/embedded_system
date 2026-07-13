import sys
import subprocess

def get_git_diff():
    """Retrieves staged and unstaged git diff compared to HEAD from the repository root."""
    try:
        repo_root = subprocess.check_output(
            ["git", "rev-parse", "--show-toplevel"], 
            text=True
        ).strip()
        diff = subprocess.check_output(
            ["git", "diff", "HEAD"], 
            cwd=repo_root, 
            text=True
        )
        return diff.strip()
    except subprocess.CalledProcessError as e:
        print(f"❌ Failed to get git diff: {e}")
        sys.exit(1)