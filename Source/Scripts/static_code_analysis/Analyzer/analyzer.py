from pathlib import Path
import subprocess
import json
from datetime import datetime
from .semgrep_output_format_handler import Semgrep_Output_Handler

class Analyzer:
    def __init__(self):
        self.script_dir = Path(__file__).parent.parent  # Go up to static_code_analysis dir
        self.reports_dir = self.script_dir / "reports"
        self.reports_dir.mkdir(exist_ok=True)
        
        # Initialize output handler
        self.output_handler = Semgrep_Output_Handler(self.reports_dir)

    def get_active_standards(self, standard_list):
        """Get list of active standards from standard_list"""
        active = [std for std in standard_list if std.is_active]
        print(f"🔍 Found {len(active)} active standards out of {len(standard_list)} total")
        return active

    def print_status(self, target_file, standard_list):
        """Print current analysis configuration"""
        # Ensure target_file is a Path object
        if not isinstance(target_file, Path):
            target_file = Path(target_file)
            
        print(f"🎯 Target: {target_file.name}")
        print(f"📁 File exists: {target_file.exists()}")
        print(f"📋 Standards: {len(standard_list)} loaded")
        
        print("📏 Available Standards:")
        for standard in standard_list:
            standard.print_status()

    def run_analysis(self, target_file, standard_list, output_format="json"):
        """Run semgrep analysis with active standards"""
        # Validate inputs
        if target_file is None:
            raise ValueError("Target file must be specified")
        
        if not standard_list:
            standard_list = []
        
        target_file = Path(target_file)
        
        if not target_file.exists():
            print(f"❌ Target file not found: {target_file}")
            return False

        active_standards = self.get_active_standards(standard_list)
        if not active_standards:
            print("❌ No active standards found!")
            print("💡 Available standards:")
            for i, std in enumerate(standard_list):
                print(f"   {i+1}. {std.name} (Active: {std.is_active})")
            return False

        # Execute pre-analysis
        self._execute_pre_analysis(active_standards, target_file)

        # Get rule files from active standards
        rule_files = []
        for standard in active_standards:
            if standard.is_available():
                rule_files.append(Path(standard.rule_file))
            else:
                print(f"⚠️  Rule file not found for {standard.name}: {standard.rule_file}")

        if not rule_files:
            print("❌ No valid rule files found in active standards!")
            return False

        # Build base semgrep command
        cmd = ["semgrep"]
        
        for rule_file in rule_files:
            cmd.extend(["--config", str(rule_file)])
        
        cmd.extend(["--no-git-ignore"])
        
        # Use output handler to run analysis
        success, result = self.output_handler.run_by_format(cmd, active_standards, target_file, output_format)
        
        if success and result:
            # Execute post-analysis
            self._execute_post_analysis(active_standards, result, target_file)
        
        return success

    # Keep legacy method for backward compatibility
    def run_analysis_legacy(self, target_file, standard_list, output_enabled=True):
        """Legacy method - use run_analysis with output_format instead"""
        if output_enabled:
            return self.run_analysis(target_file, standard_list, "json")
        else:
            return self.run_analysis(target_file, standard_list, "simple")

    def _execute_pre_analysis(self, active_standards, target_file):
        """Execute pre-analysis code from standard objects"""
        for standard in active_standards:
            if hasattr(standard, 'pre_analysis'):
                try:
                    print(f"📝 Running {standard.name} pre-analysis...")
                    standard.pre_analysis(target_file)
                except Exception as e:
                    print(f"⚠️  Error in {standard.name} pre_analysis: {e}")

    def _execute_post_analysis(self, active_standards, result, target_file):
        """Execute post-analysis code from standard objects"""
        for standard in active_standards:
            if hasattr(standard, 'post_analysis'):
                try:
                    print(f"📊 Running {standard.name} post-analysis...")
                    standard.post_analysis(target_file, result, self.reports_dir)
                except Exception as e:
                    print(f"⚠️  Error in {standard.name} post_analysis: {e}")

    # Legacy methods for backward compatibility
    def _run_simple_output(self, cmd, active_standards, target_file):
        """Legacy method - redirects to output handler"""
        success, result = self.output_handler.run_simple_output(cmd, active_standards, target_file)
        return success

    def _run_json_output(self, cmd, active_standards, target_file):
        """Legacy method - redirects to output handler"""
        success, result = self.output_handler.run_json_output(cmd, active_standards, target_file)
        return success

    def _show_summary(self, active_standards):
        """Legacy method - redirects to output handler"""
        self.output_handler.show_legacy_summary(active_standards)