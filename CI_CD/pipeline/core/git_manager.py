# pipeline/core/git_manager.py
import os
import subprocess

class GitManager:
    def __init__(self, workdir):
        self.workdir = workdir

    def run_git_cmd(self, args):
        """Helper to run git commands in the project directory."""
        cmd =["git"] + args
        print(f"\n[GIT] {' '.join(cmd)}")
        result = subprocess.run(cmd, cwd=self.workdir, capture_output=False)
        if result.returncode != 0:
            raise RuntimeError(f"Git command failed: {' '.join(cmd)}")
        
    def check_repo_exists_and_valid(self):
        """Checks if the repository exists and is valid."""
        if not os.path.exists(self.workdir):
            print(f"❌ Directory {self.workdir} does not exist.")
            return False
        
        try:
            self.run_git_cmd(["rev-parse", "--is-inside-work-tree"])
            return True
        except RuntimeError:
            print(f"❌ Directory {self.workdir} is not a valid Git repository.")
            return False

    def clone(self, repo_url, branch="main"):
        self.check_repo_exists_and_valid()
        """Clones a fresh repository if the directory is empty."""
        print(f"📦 Cloning repository {repo_url} (Branch: {branch})...")
        cmd =["git", "clone", "-b", branch, "--recurse-submodules", repo_url, self.workdir]
        subprocess.run(cmd, check=True)
        

    def init_submodules(self):
        """Initializes and updates submodules for an existing repository."""
        print("🔄 Updating Git submodules...")
        self.run_git_cmd(["submodule", "update", "--init", "--recursive"])