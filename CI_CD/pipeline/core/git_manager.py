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
        
    def clone(self, repo_url, branch="main"):
        """Clones a fresh repository or resets to HEAD if it already exists."""
        if os.path.exists(os.path.join(self.workdir, ".git")):
            print(f"🔄 Repository exists at {self.workdir}. Resetting to HEAD...")
            self.run_git_cmd(["fetch"])
            self.run_git_cmd(["pull"])
            self.run_git_cmd(["reset", "--hard", "HEAD"])
            self.run_git_cmd(["clean", "-fd"])
        else:
            print(f"📦 Cloning repository {repo_url} (Branch: {branch})...")
            cmd = ["git", "clone", "-b", branch, "--recurse-submodules", repo_url, self.workdir]
            subprocess.run(cmd, check=True)

    def init_submodules(self):
        """Initializes and updates submodules for an existing repository."""
        print("🔄 Updating Git submodules...")
        self.run_git_cmd(["submodule", "update", "--init", "--recursive"])