#!/usr/bin/env python3
"""Run the checked-in CI shell gates in isolated containers from a committed snapshot.

GitHub-only CodeQL, dependency review, and action uploads stay on GitHub. Build,
integration, format, hygiene, and secrets scanning run the actual workflow commands.
"""

import argparse
from concurrent.futures import ThreadPoolExecutor
import itertools
import json
import os
from pathlib import Path
import re
import queue
import shutil
import subprocess
import sys
import tempfile
import time

import yaml

WORKFLOWS = ("test", "integration", "quality", "hygiene", "security")
SETUP_ACTIONS = (
    "actions/checkout@",
    "actions/setup-python@",
    "actions/cache@",
    "actions/upload-artifact@",
)
INSTALL_STEPS = {
    "Install documentation dependency",
    "Install coverage tool",
    "Install pre-commit",
    "Install Gitleaks",
    "Install container prerequisites",
    "Install Clang 22",
    "Install clang-tidy",
}


def expand_matrix(job):
    """Expand strategy.matrix the way GitHub does.

    An include entry whose axis values all match existing combinations adds
    its other keys to each of them; one that matches none becomes a new entry.
    """
    matrix = job.get("strategy", {}).get("matrix", {})
    axes = {key: value for key, value in matrix.items() if key not in ("include", "exclude")}
    entries = (
        [dict(zip(axes, values)) for values in itertools.product(*axes.values())] if axes else []
    )
    for extra in matrix.get("include", []):
        matching = [
            entry
            for entry in entries
            if all(entry.get(key) == value for key, value in extra.items() if key in axes)
        ]
        if matching and any(key in axes for key in extra):
            for entry in matching:
                entry.update({key: value for key, value in extra.items() if key not in axes})
        else:
            entries.append(dict(extra))
    if matrix.get("exclude"):
        raise ValueError("Local runner needs an explicit implementation for matrix.exclude")
    return entries or [{}]


def interpolate(value, matrix):
    def replace(match):
        expression = match.group(1).strip()
        if expression == "github.workspace":
            return "/workspace"
        if expression.startswith("matrix."):
            return str(matrix[expression.removeprefix("matrix.")])
        raise ValueError(f"Unsupported local workflow expression: {expression}")

    return re.sub(r"\$\{\{(.*?)\}\}", replace, str(value))


def included(condition, matrix):
    if condition is None:
        return True
    # Report uploads run even after a failed step on GitHub; locally they are skipped.
    if condition == "always()":
        return True
    match = re.fullmatch(r"matrix\.([\w-]+) (==|!=) '([^']*)'", condition)
    if not match:
        raise ValueError(f"Unsupported local step condition: {condition}")
    return (matrix.get(match[1], "") == match[3]) == (match[2] == "==")


def container_job():
    """Execute one job; its filesystem, process tree, and database are disposable."""
    job = json.loads(Path("/input/job.json").read_text())
    os.chdir("/workspace")
    git = [
        "git",
        "-c",
        "user.name=Local CI",
        "-c",
        "user.email=ci@example.invalid",
        "-c",
        "commit.gpgsign=false",
    ]
    subprocess.run(["git", "init", "-q"], check=True)
    if job["base"]:
        # The merge base becomes HEAD^1, the parent a pull request's merge commit
        # has on GitHub, so jobs that diff against HEAD^1 see the branch's changes.
        subprocess.run(["tar", "-xf", "/input/base.tar"], check=True)
        subprocess.run(["git", "add", "-f", "."], check=True)
        subprocess.run([*git, "commit", "-qm", f"Local CI base {job['base']}"], check=True)
        for entry in Path(".").iterdir():
            if entry.name == ".git":
                continue
            if entry.is_dir() and not entry.is_symlink():
                shutil.rmtree(entry)
            else:
                entry.unlink()
    subprocess.run(["tar", "-xf", "/input/source.tar"], check=True)
    subprocess.run(["git", "add", "-A", "-f", "."], check=True)
    subprocess.run(
        [*git, "commit", "--allow-empty", "-qm", f"Local CI snapshot of {job['revision']}"],
        check=True,
    )
    env = dict(os.environ, **job["env"])
    env.update(
        GITHUB_WORKSPACE="/workspace",
        CCACHE_DIR="/ccache",
        CCACHE_BASEDIR="/workspace",
        RUNNER_TEMP="/tmp",
        LUMINARI_LOCAL_CI="1",
    )
    database = None
    try:
        service = job.get("database")
        if service:
            subprocess.run(
                [
                    "mariadb-install-db",
                    "--no-defaults",
                    "--datadir=/tmp/mysql",
                    "--auth-root-authentication-method=normal",
                    "--skip-test-db",
                ],
                check=True,
                stdout=subprocess.DEVNULL,
                stderr=subprocess.STDOUT,
            )
            database = subprocess.Popen(
                [
                    "mariadbd",
                    "--no-defaults",
                    "--datadir=/tmp/mysql",
                    "--socket=/tmp/mysql.sock",
                    "--pid-file=/tmp/mysql.pid",
                    "--bind-address=127.0.0.1",
                    "--port=3306",
                    "--log-error=/tmp/mysql.log",
                ]
            )
            for _ in range(100):
                ready = subprocess.run(
                    [
                        "mariadb-admin",
                        "--no-defaults",
                        "--socket=/tmp/mysql.sock",
                        "-u",
                        "root",
                        "ping",
                    ],
                    stdout=subprocess.DEVNULL,
                    stderr=subprocess.DEVNULL,
                )
                if ready.returncode == 0:
                    break
                time.sleep(0.1)
            sql = (
                f"CREATE DATABASE {service['MARIADB_DATABASE']};"
                f"CREATE USER '{service['MARIADB_USER']}'@'%' IDENTIFIED BY "
                f"'{service['MARIADB_PASSWORD']}';"
                f"GRANT ALL ON {service['MARIADB_DATABASE']}.* TO '{service['MARIADB_USER']}'@'%';"
                f"ALTER USER 'root'@'localhost' IDENTIFIED BY '{service['MARIADB_ROOT_PASSWORD']}';"
            )
            subprocess.run(
                ["mariadb", "--no-defaults", "--socket=/tmp/mysql.sock", "-u", "root"],
                input=sql,
                text=True,
                check=True,
            )
        for step in job["steps"]:
            print(f"==> {step['name']}", flush=True)
            if step.get("setup"):
                for header in ("campaign", "mud_options", "vnums"):
                    target = Path(f"src/config/{header}.h")
                    if not target.exists():
                        target.write_bytes(Path(f"src/config/{header}.example.h").read_bytes())
                continue
            # Dependencies are installed in the image, including mixed install/test steps.
            command = "\n".join(
                line
                for line in step["run"].splitlines()
                if not line.strip().startswith(("sudo apt-get ", "python3 -m pip install "))
            )
            subprocess.run(
                ["bash", "-eo", "pipefail", "-c", command],
                check=True,
                env=dict(env, **step.get("env", {})),
            )
        subprocess.run(["ccache", "--show-stats"], env=env, check=True)
    finally:
        if database:
            database.terminate()
            database.wait(timeout=30)
        # Keep coverage outputs beside the job log when present.
        for artifact in Path(".").glob("coverage*"):
            if artifact.is_file():
                Path("/results", artifact.name).write_bytes(artifact.read_bytes())


def main():
    root = Path(__file__).resolve().parents[3]
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--image",
        default="luminari-ci:local-fast",
        help="image for jobs without a container: key; a job with container: "
        "IMAGE uses luminari-ci:local-IMAGE (colon replaced by a dash)",
    )
    parser.add_argument("--jobs", type=int, default=3, help="concurrent containers")
    parser.add_argument("--cpus", type=int, default=4, help="cores per container")
    parser.add_argument("--cache", type=Path, default=Path.home() / ".cache/luminari-ci/ccache")
    parser.add_argument("--results", type=Path)
    parser.add_argument("--timeout", type=int, default=45, help="minutes per container")
    parser.add_argument("--list", action="store_true")
    parser.add_argument("--job", help="run one job name from --list")
    parser.add_argument(
        "--base",
        default="origin/master",
        help="the snapshot's parent is the merge base of HEAD and this ref, as HEAD^1 is "
        "for a GitHub pull request (default: origin/master)",
    )
    args = parser.parse_args()
    revision = subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=root, text=True).strip()
    merge_base = subprocess.run(
        ["git", "merge-base", args.base, revision], cwd=root, capture_output=True, text=True
    )
    base = merge_base.stdout.strip() if merge_base.returncode == 0 else ""
    jobs = []
    for workflow_name in WORKFLOWS:
        content = subprocess.check_output(
            ["git", "show", f"{revision}:.github/workflows/{workflow_name}.yml"], cwd=root
        )
        workflow = yaml.safe_load(content)
        for name, job in workflow["jobs"].items():
            if workflow_name == "security" and name in ("codeql", "dependency-review"):
                continue
            for matrix in expand_matrix(job):
                label = f"{workflow_name}-{name}" + "".join(
                    f"-{value}"
                    for key, value in matrix.items()
                    if key in ("build", "cc", "compiler", "build_type")
                )
                env = {
                    key: interpolate(value, matrix)
                    for key, value in {**workflow.get("env", {}), **job.get("env", {})}.items()
                }
                steps = []
                for step in job["steps"]:
                    if not included(step.get("if"), matrix) or step["name"] in INSTALL_STEPS:
                        continue
                    action = step.get("uses", "")
                    if action == "./.github/actions/setup-build":
                        steps.append({"name": step["name"], "setup": True})
                    elif action.startswith(SETUP_ACTIONS):
                        continue
                    elif action:
                        raise ValueError(f"Unimplemented local action: {action}")
                    else:
                        steps.append(
                            {
                                "name": step["name"],
                                "run": interpolate(step["run"], matrix),
                                "env": {
                                    key: interpolate(value, matrix)
                                    for key, value in step.get("env", {}).items()
                                },
                            }
                        )
                image = interpolate(job.get("container", ""), matrix)
                jobs.append(
                    dict(
                        name=label,
                        env=env,
                        steps=steps,
                        revision=revision,
                        base=base,
                        image=f"luminari-ci:local-{image.replace(':', '-')}" if image else "",
                        database=job.get("services", {}).get("mariadb", {}).get("env"),
                    )
                )
    if args.list:
        print("\n".join(job["name"] for job in jobs))
        return
    if args.job:
        jobs = [job for job in jobs if job["name"] == args.job]
        if not jobs:
            parser.error("Unknown job; see --list")
    if args.jobs < 1 or args.cpus < 1:
        parser.error("--jobs and --cpus must be positive")
    cpus = sorted(os.sched_getaffinity(0))
    if args.jobs * args.cpus > len(cpus):
        parser.error("Requested concurrency exceeds available CPUs")
    args.cache.mkdir(parents=True, exist_ok=True)
    results = (args.results or Path(tempfile.mkdtemp(prefix="luminari-ci-results-"))).resolve()
    results.mkdir(parents=True, exist_ok=True)
    print(f"Commit {revision} (base {base or 'none'}); results: {results}", flush=True)
    with tempfile.TemporaryDirectory(prefix="luminari-ci-input-") as directory:
        runner = Path(directory, "run.py")
        runner.write_bytes(
            subprocess.check_output(
                ["git", "show", f"{revision}:scripts/ci/local/run.py"], cwd=root
            )
        )
        source = Path(directory, "source.tar")
        with source.open("wb") as output:
            subprocess.run(["git", "archive", revision], cwd=root, stdout=output, check=True)
        if base:
            with Path(directory, "base.tar").open("wb") as output:
                subprocess.run(["git", "archive", base], cwd=root, stdout=output, check=True)
        start = time.monotonic()
        cpu_groups = queue.Queue()
        for group in range(args.jobs):
            cpu_groups.put(cpus[group * args.cpus : (group + 1) * args.cpus])

        def run(index_job):
            _, job = index_job
            job_dir = results / job["name"]
            job_dir.mkdir(exist_ok=True)
            descriptor = Path(directory, job["name"] + ".json")
            descriptor.write_text(json.dumps(job))
            selected = cpu_groups.get()
            container = f"luminari-ci-{os.getpid()}-{job['name']}"
            command = [
                "docker",
                "run",
                "--rm",
                "--init",
                "--name",
                container,
                "--user",
                f"{os.getuid()}:{os.getgid()}",
                "--cpuset-cpus",
                ",".join(map(str, selected)),
                "--workdir",
                "/workspace",
                "--tmpfs",
                f"/workspace:exec,mode=0755,uid={os.getuid()},gid={os.getgid()}",
                "-v",
                f"{source}:/input/source.tar:ro",
                *(["-v", f"{Path(directory, 'base.tar')}:/input/base.tar:ro"] if base else []),
                "-v",
                f"{descriptor}:/input/job.json:ro",
                "-v",
                f"{runner}:/input/run.py:ro",
                "-v",
                f"{args.cache.resolve()}:/ccache",
                "-v",
                f"{job_dir}:/results",
                # The service runs inside the job container; GitHub container jobs
                # reach it as 'mariadb', so resolve that name to loopback.
                *(["--add-host", "mariadb:127.0.0.1"] if job["database"] else []),
                job["image"] or args.image,
                "python3",
                "/input/run.py",
                "--container-job",
            ]
            begin = time.monotonic()
            try:
                with (job_dir / "job.log").open("w") as log:
                    status = subprocess.run(
                        command, stdout=log, stderr=subprocess.STDOUT, timeout=args.timeout * 60
                    ).returncode
            except subprocess.TimeoutExpired:
                subprocess.run(
                    ["docker", "kill", container],
                    stdout=subprocess.DEVNULL,
                    stderr=subprocess.DEVNULL,
                )
                status = 124
            finally:
                cpu_groups.put(selected)
            result = dict(
                job=job["name"], status=status, seconds=round(time.monotonic() - begin, 2)
            )
            print(json.dumps(result), flush=True)
            return result

        with ThreadPoolExecutor(max_workers=args.jobs) as pool:
            outcomes = list(pool.map(run, enumerate(jobs)))
        summary = dict(revision=revision, seconds=round(time.monotonic() - start, 2), jobs=outcomes)
        (results / "summary.json").write_text(json.dumps(summary, indent=2) + "\n")
        print(f"Matrix finished in {summary['seconds']} s", flush=True)
        if any(job["status"] for job in outcomes):
            raise SystemExit(1)


if __name__ == "__main__":
    if sys.argv[1:] == ["--container-job"]:
        container_job()
    else:
        main()
