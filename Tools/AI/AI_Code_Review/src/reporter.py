import json
from pathlib import Path
from datetime import datetime

def format_report(review_content, format_type="markdown"):
    """Prepares the report structure based on the preferred format."""
    if format_type == "json":
        report_data = {
            "metadata": {
                "timestamp": datetime.now().isoformat(),
                "type": "AI Code Review Feedback"
            },
            "content": review_content
        }
        return json.dumps(report_data, indent=2)
    
    # Default Markdown style
    border = "=" * 45
    return f"{border}\n📝 AI CODE REVIEW REPORT\n{border}\n\n{review_content}"

def save_report(formatted_report, output_file=None, format_type="markdown"):
    """Outputs report to the console, and automatically saves it under the Out/ folder."""
    # Print to stdout
    print(formatted_report)
    
    # Resolve the project root and ensure the Out directory exists
    project_root = Path(__file__).parent.parent.absolute()
    out_dir = project_root / "Out"
    out_dir.mkdir(parents=True, exist_ok=True)
    
    # Resolve the output filepath
    if not output_file:
        ext = "json" if format_type == "json" else "md"
        timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
        output_path = out_dir / f"review_report_{timestamp}.{ext}"
    else:
        output_path = Path(output_file)
        if not output_path.is_absolute():
            # Force target file to be created inside the Out directory if given as a relative name
            output_path = out_dir / output_path
        output_path.parent.mkdir(parents=True, exist_ok=True)
        
    try:
        with open(output_path, "w", encoding="utf-8") as f:
            f.write(formatted_report)
        print(f"\n💾 Report successfully saved to: {output_path}")
    except IOError as e:
        print(f"❌ Failed to save file output: {e}")