#!/usr/bin/env python3
# runner_mr.py

import os
import sys
import argparse
import subprocess
from pathlib import Path

def get_repo_root():
    """Retrieves the absolute path to the Git repository root."""
    try:
        return subprocess.check_output(["git", "rev-parse", "--show-toplevel"], encoding="utf-8").strip()
    except subprocess.CalledProcessError:
        return os.getcwd()

def detect_source_branch():
    """Detects source branch (branch to be merged) from CI env variables or defaults to HEAD."""
    if os.environ.get("CI_MERGE_REQUEST_SOURCE_BRANCH_NAME"):
        return f"origin/{os.environ['CI_MERGE_REQUEST_SOURCE_BRANCH_NAME']}"
    if os.environ.get("GITHUB_HEAD_REF"):
        return f"origin/{os.environ['GITHUB_HEAD_REF']}"
    return "HEAD"

def detect_target_branch():
    """Detects target branch (branch to merge into) from CI env variables or defaults to origin/main."""
    if os.environ.get("CI_MERGE_REQUEST_TARGET_BRANCH_NAME"):
        return f"origin/{os.environ['CI_MERGE_REQUEST_TARGET_BRANCH_NAME']}"
    if os.environ.get("GITHUB_BASE_REF"):
        return f"origin/{os.environ['GITHUB_BASE_REF']}"
    return "origin/main"

def main():
    parser = argparse.ArgumentParser(description="MR runner to fetch complete diff between branches")
    parser.add_argument(
        "--source-branch",
        help="Branch to be merged (defaults to auto-detect from CI, or 'HEAD')"
    )
    parser.add_argument(
        "--target-branch",
        help="Target branch to merge into (defaults to auto-detect from CI, or 'origin/main')"
    )
    parser.add_argument(
        "--debug",
        action="store_true",
        help="Enable verbose debug logging of changes, commits, and diff contents"
    )

    args, remaining_args = parser.parse_known_args()

    # Explicitly forward the --debug flag to main.py
    if args.debug:
        remaining_args.append("--debug")

    # 1. Resolve source and target branches
    repo_root = get_repo_root()
    source = args.source_branch or detect_source_branch()
    target = args.target_branch or detect_target_branch()
    compare_expr = f"{target}...{source}"
    
    print(f"🔍 Comparing changes in '{source}' (source) against '{target}' (target)...")

    # 2. Debug Logging (Outputs commits, files and branch metadata)
    if args.debug:
        print("\n" + "[DEBUG] " + "="*45)
        print("[DEBUG] LOCAL REPOSITORY COMPARISON METADATA")
        print("[DEBUG] " + "="*45)
        print(f"[DEBUG] Git Repo Root:  {repo_root}")
        print(f"[DEBUG] Target Branch:  {target}")
        print(f"[DEBUG] Source Branch:  {source}")
        print(f"[DEBUG] Diff Expression: {compare_expr}")
        
        # Log Commits
        try:
            commits_cmd = ["git", "log", "--oneline", compare_expr]
            commits = subprocess.check_output(commits_cmd, cwd=repo_root, encoding="utf-8").strip().splitlines()
            print(f"\n[DEBUG] Commits detected ({len(commits)}):")
            for commit in commits:
                print(f"  - {commit}")
        except subprocess.CalledProcessError as e:
            print(f"[DEBUG] Failed to fetch commits: {e}")

        # Log Files Affected
        try:
            files_cmd = ["git", "diff", "--name-only", compare_expr]
            files = subprocess.check_output(files_cmd, cwd=repo_root, encoding="utf-8").strip().splitlines()
            print(f"\n[DEBUG] Files changed ({len(files)}):")
            for f in files:
                print(f"  - {f}")
        except subprocess.CalledProcessError as e:
            print(f"[DEBUG] Failed to fetch changed files: {e}")
            
        print("[DEBUG] " + "="*45 + "\n")

    # 3. Forward execution to main.py with branch diff target
    main_script = Path(__file__).parent.absolute() / "main.py"
    cmd = [sys.executable, str(main_script), "--diff-branch", compare_expr] + remaining_args
    
    print(f"\n🚀 Forwarding to main.py:\n  {' '.join(cmd)}\n")
    result = subprocess.run(cmd)
    sys.exit(result.returncode)

if __name__ == "__main__":
    main()