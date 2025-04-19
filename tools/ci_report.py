import argparse
import logging
import os
import subprocess


logger = logging.getLogger(__name__)


def create_row(items: list[str]) -> str:
    return "| " + " | ".join(items) + " |"


def convert_to_markdown(input: str) -> str:
    lines = input.splitlines()
    columns = [it.strip() for it in lines[0].split("  ") if it]
    header = create_row(columns)
    div = create_row(["---"] + ["---:" for _ in columns[1:]])
    rows = [create_row(it.split())
            for it in lines[1:] if not it.startswith("---")]
    return "\n".join([header, div, *rows]) + "\n"


def coverage_report(build_dir: str, output_file: str):
    with os.scandir(build_dir) as it:
        profraw_files = sorted(entry.path for entry in it
                               if entry.is_file() and entry.name.endswith(".profraw"))
    if not profraw_files:
        logger.warning("No .profraw files found. Skipping coverage report")
        return
    profdata = os.path.join(build_dir, "coverage.profdata")
    subprocess.run(["llvm-profdata", "merge", "-sparse",
                   *profraw_files, "-o", profdata], check=True)
    executables = [it.removesuffix(".profraw") for it in profraw_files]
    result = subprocess.run(["llvm-cov", "report", *executables,
                             f"-instr-profile={profdata}"], capture_output=True, check=True)
    md_table = convert_to_markdown(result.stdout.decode())
    with open(output_file, "a") as f:
        logger.info(f"Writing coverage report to {output_file}")
        print(md_table, file=f)


def main():
    logging.basicConfig(level=logging.INFO)
    parser = argparse.ArgumentParser()
    parser.add_argument("build_dir", help="Path to the build directory")
    parser.add_argument("output_file", help="Output file for the report")
    args = parser.parse_args()
    coverage_report(args.build_dir, args.output_file)


if __name__ == "__main__":
    main()
