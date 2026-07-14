from pathlib import Path

def get_prompts_dir():
    """Resolves the path to the prompts directory."""
    return Path(__file__).parent.parent.absolute() / "prompts"

def load_template_file(name_or_path):
    """Loads markdown templates. Supports direct file paths, directories, or names inside prompts/."""
    path = Path(name_or_path)
    
    # 1. Direct Markdown File Path
    if path.is_file() and path.suffix == ".md":
        with open(path, "r", encoding="utf-8") as f:
            return f.read().strip()
            
    # 2. Direct Directory Path (scans and merges all internal .md files)
    if path.is_dir():
        combined = []
        for md_file in sorted(path.glob("**/*.md")):
            with open(md_file, "r", encoding="utf-8") as f:
                combined.append(f"### From {md_file.name} ###\n" + f.read().strip())
        return "\n\n".join(combined)
        
    # 3. Standard Fallback to templates inside the prompts/ directory
    prompts_dir = get_prompts_dir()
    file_path = prompts_dir / f"{name_or_path}.md"
    if file_path.exists():
        with open(file_path, "r", encoding="utf-8") as f:
            return f.read().strip()
            
    # 4. Directory inside the prompts/ directory
    dir_path = prompts_dir / name_or_path
    if dir_path.is_dir():
        combined = []
        for md_file in sorted(dir_path.glob("**/*.md")):
            with open(md_file, "r", encoding="utf-8") as f:
                combined.append(f"### From {md_file.name} ###\n" + f.read().strip())
        return "\n\n".join(combined)
        
    print(f"⚠️ Prompt template '{name_or_path}' not found. Skipping.")
    return ""

def load_multiple_templates(template_names):
    """Loads and combines multiple instruction prompt files or directories."""
    combined_instructions = []
    for name in template_names:
        content = load_template_file(name)
        if content:
            combined_instructions.append(content)
    
    if not combined_instructions:
        fallback = load_template_file("general_review")
        return fallback if fallback else "Review code for performance and bugs."
        
    return "\n\n".join(combined_instructions)

def read_code_files(file_paths):
    """Reads specific codebase files and formats them into a clean string."""
    combined_code = []
    for path_str in file_paths:
        path = Path(path_str)
        if not path.exists():
            print(f"⚠️ Target code file not found: {path_str}. Skipping.")
            continue
        try:
            with open(path, "r", encoding="utf-8", errors="ignore") as f:
                content = f.read()
            combined_code.append(f"--- FILE CONTENT: {path_str} ---\n```\n{content}\n```")
        except Exception as e:
            print(f"⚠️ Could not read file '{path_str}': {e}")
            
    return "\n\n".join(combined_code)

def build_prompt(instructions, code_content, report_template):
    """Assembles instructions, code contents, and structural report templates into a single query."""
    prompt_parts = [
        "=== SYSTEM INSTRUCTIONS ===",
        instructions,
        "\n=== MANDATORY OUTPUT FORMAT ===",
        report_template,
        "\n=== CODE FOR REVIEW ===",
        code_content
    ]
    return "\n\n".join(prompt_parts)