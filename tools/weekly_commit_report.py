#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import argparse
import subprocess
import sys
from collections import OrderedDict


def run_git(repo_path, args):
    command = ["git", "-C", repo_path] + args
    return subprocess.run(command, check=True, capture_output=True, text=True)


def detect_default_author(repo_path):
    try:
        result = run_git(repo_path, ["config", "user.name"])
        return result.stdout.strip() or None
    except subprocess.CalledProcessError:
        return None


def load_commits(repo_path, author=None, since=None, until=None, no_merges=True):
    cmd = [
        "log",
        "--date=format:%G-W%V",
        "--pretty=format:%ad%x09%h%x09%s",
    ]
    if no_merges:
        cmd.append("--no-merges")
    if author:
        cmd.append(f"--author={author}")
    if since:
        cmd.append(f"--since={since}")
    if until:
        cmd.append(f"--until={until}")

    result = run_git(repo_path, cmd)
    lines = [line for line in result.stdout.splitlines() if line.strip()]
    grouped = OrderedDict()
    for line in lines:
        week, sha, subject = line.split("\t", 2)
        grouped.setdefault(week, []).append((sha, subject))
    return grouped


def print_report(grouped):
    if not grouped:
        print("未找到符合条件的提交记录。")
        return
    for week, commits in grouped.items():
        print(f"{week} ({len(commits)} commits)")
        for sha, subject in commits:
            print(f"  - {sha} {subject}")


def parse_args():
    parser = argparse.ArgumentParser(description="按周统计提交内容")
    parser.add_argument(
        "--repo",
        default=".",
        help="仓库路径（默认当前目录）",
    )
    parser.add_argument(
        "--author",
        default=None,
        help="提交作者过滤（默认使用 git config user.name）",
    )
    parser.add_argument(
        "--since",
        default=None,
        help="开始时间（传给 git log --since，例如 2026-01-01 或 '4 weeks ago'）",
    )
    parser.add_argument(
        "--until",
        default=None,
        help="结束时间（传给 git log --until，例如 2026-12-31）",
    )
    parser.add_argument(
        "--include-merges",
        action="store_true",
        help="包含 merge 提交（默认不包含）",
    )
    return parser.parse_args()


def main():
    args = parse_args()
    author = args.author if args.author else detect_default_author(args.repo)
    try:
        grouped = load_commits(
            repo_path=args.repo,
            author=author,
            since=args.since,
            until=args.until,
            no_merges=not args.include_merges,
        )
    except subprocess.CalledProcessError as err:
        print(err.stderr.strip() or "git 命令执行失败。", file=sys.stderr)
        return 1

    if author:
        print(f"Author: {author}")
    print_report(grouped)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
