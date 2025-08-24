import subprocess
import json
from pathlib import Path
from datetime import datetime

class Semgrep_Output_Handler:
    """Handles different output formats for Semgrep analysis"""
    
    def __init__(self, reports_dir):
        self.reports_dir = Path(reports_dir)
        self.reports_dir.mkdir(exist_ok=True)

    def _generate_report_filename(self, format_type, target_file=None, custom_prefix=None):
        """Generate timestamped filename for different report formats"""
        timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
        
        # Define format-specific patterns
        format_patterns = {
            "simple": ("simple_results", ".txt"),
            "json": ("results", ".json"),
            "sarif": ("results", ".sarif"),
            "junit": ("results", ".xml"),
            "gitlab": ("gl-sast-report", ".json")
        }
        
        if custom_prefix:
            base_name = custom_prefix
        elif format_type in format_patterns:
            base_name, _ = format_patterns[format_type]
        else:
            base_name = "results"
        
        if format_type in format_patterns:
            _, extension = format_patterns[format_type]
        else:
            extension = ".txt"

        if target_file:
            target_file = target_file.stem
        else:
            target_file = "analysis"
        
        filename = f"{base_name}__{target_file}__{timestamp}{extension}"
        return self.reports_dir / filename
    
    def run_by_format(self, cmd, active_standards, target_file, output_format):
        """Run analysis based on the specified output format"""
        output_format = output_format.lower()
        
        if output_format == "simple" or output_format == "text":
            return self.run_simple_output(cmd, active_standards, target_file)
        
        elif output_format == "json":
            return self.run_json_output(cmd, active_standards, target_file)
        
        elif output_format == "sarif":
            return self.run_sarif_output(cmd, active_standards, target_file)
        
        elif output_format == "junit":
            return self.run_junit_output(cmd, active_standards, target_file)
        
        elif output_format == "gitlab":
            return self.run_gitlab_output(cmd, active_standards, target_file)
        
        elif output_format == "both" or output_format == "all":
            return self.run_multiple_outputs(cmd, active_standards, target_file)
        
        else:
            print(f"❌ Unsupported output format: {output_format}")
            print("💡 Supported formats: simple, json, sarif, junit, gitlab, both")
            return False

    def run_simple_output(self, cmd, active_standards, target_file):
        """Run analysis with simple text output"""
        final_cmd = cmd + [str(target_file)]
        print(f"🚀 Running (Text): {' '.join(final_cmd)}")
        print()
        
        try:
            result = subprocess.run(
                final_cmd, capture_output=True, text=True,
                encoding="utf-8", errors="ignore"
            )
            
            print("📝 Analysis Results:")
            if result.stdout:
                print(result.stdout)
                
                # Save simple output to reports folder
                simple_report_file = self._generate_report_filename("simple", target_file)
                with open(simple_report_file, "w", encoding="utf-8") as f:
                    f.write(f"Simple Analysis Results for {target_file.name}\n")
                    f.write("=" * 50 + "\n\n")
                    f.write(result.stdout)
                    if result.stderr:
                        f.write("\n\nMessages:\n")
                        f.write(result.stderr)
                
                print(f"📄 Simple report saved: {simple_report_file}")
            else:
                print("No findings reported.")
            
            if result.stderr:
                print("\n📝 Messages:")
                print(result.stderr)
            
            return True, result
            
        except Exception as e:
            print(f"❌ Analysis failed: {e}")
            return False, None

    def run_json_output(self, cmd, active_standards, target_file):
        """Run analysis with JSON output"""
        json_report_file = self._generate_report_filename("json", target_file)
        
        final_cmd = cmd + [
            "--json",
            "--output", str(json_report_file),
            str(target_file)
        ]
        
        print(f"🚀 Running (JSON): {' '.join(final_cmd)}")
        print()
        
        try:
            result = subprocess.run(
                final_cmd, capture_output=True, text=True,
                encoding="utf-8", errors="ignore"
            )
            
            if result.stderr:
                print("📝 Messages:")
                print(result.stderr)
            
            # Show summary from the generated file
            self.show_summary_from_file(json_report_file, active_standards)
            
            print(f"📄 JSON Report: {json_report_file}")
            return True, result
            
        except Exception as e:
            print(f"❌ Analysis failed: {e}")
            return False, None

    def run_sarif_output(self, cmd, active_standards, target_file):
        """Run analysis with SARIF output"""
        sarif_report_file = self._generate_report_filename("sarif", target_file)
        
        final_cmd = cmd + [
            "--sarif",
            "--output", str(sarif_report_file),
            str(target_file)
        ]
        
        print(f"🚀 Running (SARIF): {' '.join(final_cmd)}")
        print()
        
        try:
            result = subprocess.run(
                final_cmd, capture_output=True, text=True,
                encoding="utf-8", errors="ignore"
            )
            
            if result.stderr:
                print("📝 Messages:")
                print(result.stderr)
            
            print(f"📄 SARIF Report: {sarif_report_file}")
            return True, result
            
        except Exception as e:
            print(f"❌ Analysis failed: {e}")
            return False, None

    def run_junit_output(self, cmd, active_standards, target_file):
        """Run analysis with JUnit XML output"""
        junit_report_file = self._generate_report_filename("junit", target_file)
        
        final_cmd = cmd + [
            "--junit-xml",
            "--output", str(junit_report_file),
            str(target_file)
        ]
        
        print(f"🚀 Running (JUnit): {' '.join(final_cmd)}")
        print()
        
        try:
            result = subprocess.run(
                final_cmd, capture_output=True, text=True,
                encoding="utf-8", errors="ignore"
            )
            
            if result.stderr:
                print("📝 Messages:")
                print(result.stderr)
            
            print(f"📄 JUnit Report: {junit_report_file}")
            return True, result
            
        except Exception as e:
            print(f"❌ Analysis failed: {e}")
            return False, None

    def run_gitlab_output(self, cmd, active_standards, target_file):
        """Run analysis with GitLab SAST output"""
        gitlab_report_file = self._generate_report_filename("gitlab", target_file)
        
        final_cmd = cmd + [
            "--gitlab-sast",
            "--output", str(gitlab_report_file),
            str(target_file)
        ]
        
        print(f"🚀 Running (GitLab): {' '.join(final_cmd)}")
        print()
        
        try:
            result = subprocess.run(
                final_cmd, capture_output=True, text=True,
                encoding="utf-8", errors="ignore"
            )
            
            if result.stderr:
                print("📝 Messages:")
                print(result.stderr)
            
            print(f"📄 GitLab Report: {gitlab_report_file}")
            return True, result
            
        except Exception as e:
            print(f"❌ Analysis failed: {e}")
            return False, None

    def run_multiple_outputs(self, cmd, active_standards, target_file):
        """Run analysis with multiple output formats by calling existing methods"""
        print(f"🚀 Running Multiple Output Formats:")
        print()
        
        # Define which formats to generate
        formats_to_run = [
            ("JSON", self.run_json_output),
            ("SARIF", self.run_sarif_output), 
            ("JUnit XML", self.run_junit_output),
            ("GitLab SAST", self.run_gitlab_output),
            ("Simple Text", self.run_simple_output)
        ]
        
        success_count = 0
        results = []
        last_result = None
        
        for format_name, format_method in formats_to_run:
            print(f"⚡ Generating {format_name} report...")
            
            try:
                success, result = format_method(cmd, active_standards, target_file)
                
                if success:
                    success_count += 1
                    print(f"   ✅ {format_name} completed")
                    results.append((format_name, True, result))
                    last_result = result
                else:
                    print(f"   ❌ {format_name} failed")
                    results.append((format_name, False, None))
                    
            except Exception as e:
                print(f"   ❌ {format_name} error: {e}")
                results.append((format_name, False, None))
            
            print()  # Add spacing between formats
        
        # Summary
        print(f"📊 MULTIPLE FORMAT SUMMARY:")
        print(f"   ✅ Successful: {success_count}/{len(formats_to_run)}")
        print(f"   📈 Success Rate: {success_count/len(formats_to_run)*100:.1f}%")
        print()
        
        # Show individual results
        for format_name, success, result in results:
            status = "✅" if success else "❌"
            print(f"   {status} {format_name}")
        
        return success_count > 0, last_result

    def show_summary_from_file(self, json_file, active_standards):
        """Show analysis summary from specific JSON file"""
        if not json_file.exists():
            print("⚠️  No JSON results file generated")
            return
        
        try:
            with open(json_file, "r", encoding="utf-8") as f:
                data = json.load(f)
            
            results = data.get("results", [])
            
            print("📊 ANALYSIS SUMMARY:")
            print(f"   📁 Total Issues: {len(results)}")
            
            # Count by severity
            severity_counts = {"ERROR": 0, "WARNING": 0, "INFO": 0}
            for result in results:
                severity = result.get("extra", {}).get("severity", "INFO")
                if severity in severity_counts:
                    severity_counts[severity] += 1
            
            print(f"   🔴 ERRORS: {severity_counts['ERROR']}")
            print(f"   🟡 WARNINGS: {severity_counts['WARNING']}")
            print(f"   🔵 INFO: {severity_counts['INFO']}")
            
            # Count by standard
            print(f"\n📋 BY STANDARD:")
            for standard in active_standards:
                count = sum(1 for r in results 
                           if standard.name.lower().replace(" ", "_") in r.get("check_id", "").lower())
                print(f"   • {standard.name}: {count} issues")
            
        except Exception as e:
            print(f"⚠️  Error reading results: {e}")