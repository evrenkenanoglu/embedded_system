from pathlib import Path

def get_prompts_dir():
    """Resolves the path to the prompts directory."""
    return Path(__file__).parent.parent.absolute() / "prompts"

def load_template_file(name):
    """Loads a markdown template file safely from the prompts directory."""
    prompts_dir = get_prompts_dir()
    file_path = prompts_dir / f"{name}.md"
    
    if not file_path.exists():
        print(f"⚠️ Prompt template '{name}.md' not found in {prompts_dir}. Skipping.")
        return ""
    
    with open(file_path, "r", encoding="utf-8") as f:
        return f.read().strip()

def load_multiple_templates(template_names):
    """Loads and combines multiple instruction prompt files."""
    combined_instructions = []
    for name in template_names:
        content = load_template_file(name)
        if content:
            combined_instructions.append(f"### Instruction Module: {name.upper()} ###\n{content}")
    
    if not combined_instructions:
        # Fallback to general review if no valid files loaded
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