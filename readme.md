
<div align="center">
  <img width="400" height="240" alt="gsr" src="https://github.com/user-attachments/assets/aca9f169-a3f1-4c30-9557-b99a930f8c13" />
</div>

---

**gsr** (gradescript runner) is a gradescript CLI tool UTK CS courses. It compiles your code, runs gradescripts, and displays failures side-by-side all in one step.

## Installation

```bash
./install.sh
```

## Usage

```bash
gsr [options] [class/lab] gradescript#
```

If you're in a recognized lab directory, the class/lab argument is optional:

```bash
cd ~/cs202/lab3
gsr 5          # runs gradescript 5 for cs202 lab3
```

To run all gradescripts:

```bash
gsr 0          # runs gradeall
```

## Options

| Flag | Description |
|------|-------------|
| `-a`, `-y`, `--all` | Show all lines, not just diffs |
| `-s`, `--single` | Show only the first diff |
| `-h`, `--help` | Show help |
| `-v`, `--version` | Show version |

## Directory Detection

gsr auto-detects your course and lab from your current path using these patterns:

- **Course**: `cs202`, `CS202`, or just `202`
- **Lab**: `lab3`, `Lab3`, `3`, `labA`, `LabA`, `a`

Examples of recognized paths:
- `~/cs202/lab3/`
- `~/CS302/Lab1/`
- `~/202/labA/`

## Supported Labs

- CS 202: Labs 0-9, A, B
- CS 302: Labs 1-2

## Author

James Garrick
