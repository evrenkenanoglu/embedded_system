from pathlib import Path
import subprocess
import json

class Analyzer:
    def __init__(self):
        self.script_dir = Path(__file__).parent.parent  # Go up to static_code_analysis dir
        self.reports_dir = self.script_dir / "reports"
        self.reports_dir.mkdir(exist_ok=True)

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

    def run_analysis(self, target_file, standard_list):
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

        # Build semgrep command
        cmd = ["semgrep"]
        
        for rule_file in rule_files:
            cmd.extend(["--config", str(rule_file)])
        
        cmd.extend(["--no-git-ignore"])
        
        # Determine output format
        if len(active_standards) == 1 and "simple" in active_standards[0].name.lower():
            cmd.append(str(target_file))
            return self._run_simple_output(cmd, active_standards, target_file)
        else:
            cmd.extend([
                "--json",
                "--output", str(self.reports_dir / "results.json"),
                str(target_file)
            ])
            return self._run_json_output(cmd, active_standards, target_file)

    def activate_standards(self, standard_list, standard_names):
        """Activate standards by name - improved matching"""
        activated = []
        for standard in standard_list:
            for requested_std in standard_names:
                # More flexible matching
                if (requested_std.lower() in standard.name.lower() or 
                    requested_std.lower() == "simple" and "simple" in standard.name.lower()):
                    standard.is_active = True
                    activated.append(standard.name)
                    print(f"🔵 Activated: {standard.name}")
                    break
        
        if not activated:
            print(f"⚠️  No standards activated from: {standard_names}")
            print("📋 Available standards:")
            for std in standard_list:
                print(f"   • {std.name}")

    def _execute_pre_analysis(self, active_standards, target_file):
        """Execute pre-analysis code from standard objects"""
        for standard in active_standards:
            if hasattr(standard, 'pre_analysis'):
                try:
                    print(f"📝 Running {standard.name} pre-analysis...")
                    standard.pre_analysis(target_file)
                except Exception as e:
                    print(f"⚠️  Error in {standard.name} pre_analysis: {e}")

    def _run_simple_output(self, cmd, active_standards, target_file):
        """Run analysis with simple text output"""
        print(f"🚀 Running: {' '.join(cmd)}")
        print()
        
        try:
            result = subprocess.run(
                cmd, capture_output=True, text=True,
                encoding="utf-8", errors="ignore"
            )
            
            print("📝 Analysis Results:")
            if result.stdout:
                print(result.stdout)
            else:
                print("No findings reported.")
            
            if result.stderr:
                print("\n📝 Messages:")
                print(result.stderr)
            
            # Execute post-analysis
            self._execute_post_analysis(active_standards, result, target_file)
            return True
            
        except Exception as e:
            print(f"❌ Analysis failed: {e}")
            return False

    def _run_json_output(self, cmd, active_standards, target_file):
        """Run analysis with JSON output"""
        print(f"🚀 Running: {' '.join(cmd)}")
        print()
        
        try:
            result = subprocess.run(
                cmd, capture_output=True, text=True,
                encoding="utf-8", errors="ignore"
            )
            
            if result.stderr:
                print("📝 Messages:")
                print(result.stderr)
            
            self._show_summary(active_standards)
            self._execute_post_analysis(active_standards, result, target_file)
            return True
            
        except Exception as e:
            print(f"❌ Analysis failed: {e}")
            return False

    def _execute_post_analysis(self, active_standards, result, target_file):
        """Execute post-analysis code from standard objects"""
        for standard in active_standards:
            if hasattr(standard, 'post_analysis'):
                try:
                    print(f"📊 Running {standard.name} post-analysis...")
                    standard.post_analysis(target_file, result, self.reports_dir)
                except Exception as e:
                    print(f"⚠️  Error in {standard.name} post_analysis: {e}")

    def _show_summary(self, active_standards):
        """Show analysis summary from JSON results"""
        results_file = self.reports_dir / "results.json"
        
        if not results_file.exists():
            print("⚠️  No results file generated")
            return
        
        try:
            with open(results_file, "r") as f:
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
            
            print(f"\n📄 Full report: {results_file}")
            
        except Exception as e:
            print(f"⚠️  Error reading results: {e}")