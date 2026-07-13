#!/usr/bin/env python3

import sys
from src.config import parse_args, DEFAULT_PROVIDERS, get_api_key
from src.git_utils import get_git_diff
from src.prompt_manager import (
    load_multiple_templates, 
    load_template_file, 
    read_code_files, 
    build_prompt
)
from src.providers import query_ai
from src.reporter import format_report, save_report

def main():
    args = parse_args()

    # Determine provider details and load credentials
    provider = args.provider
    model = args.model or DEFAULT_PROVIDERS[provider]["model"]
    api_key = get_api_key(provider, args.key)

    # 1. Gather Target Code Content (Files vs Git Diff)
    if args.files:
        print(f"📂 Preparing static file review for {len(args.files)} file(s)...")
        code_content = read_code_files(args.files)
        if not code_content:
            print("❌ No valid files were successfully read. Exiting.")
            sys.exit(1)
    else:
        print("🔍 No direct files supplied. Falling back to Git Diff...")
        diff = get_git_diff()
        if not diff:
            print("✅ No local changes found to review.")
            sys.exit(0)
        code_content = f"--- TARGET GIT DIFF FOR REVIEW ---\n```diff\n{diff}\n```"

    # 2. Compile instructions (Support multiple prompts)
    instructions = load_multiple_templates(args.templates)
    
    # 3. Load structured report template format
    report_template = load_template_file(args.report_template)
    if not report_template:
         print("⚠️ Report format template was missing. Defaulting to standard style.")
         report_template = "Provide response in plain, readable Markdown."

    # 4. Construct complete prompt payload
    prompt = build_prompt(instructions, code_content, report_template)

    # 5. Query LLM
    print(f"🚀 Querying {provider.upper()} (Model: {model})...")
    review_content = query_ai(provider, model, prompt, api_key, args.url)

    # 6. Output Processing
    report = format_report(review_content, format_type=args.format)
    save_report(report, output_file=args.output_file)

if __name__ == "__main__":
    main()