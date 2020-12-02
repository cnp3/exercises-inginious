#!/usr/bin/env python3
import os
import sys
import subprocess
import traceback
from urllib.parse import urljoin

if len(sys.argv) < 4:
    print("Usage: %s from_commit_id to_commit_id repo_subdirectory" % (sys.argv[0]), file=sys.stderr)
    exit(-1)

def run(cmd, stdout=sys.stdout, shell=False):
    if 'curl' in cmd and os.environ.get('DRY_RUN'):
        print(cmd)
        return
    return subprocess.run(cmd.split(' ') if not shell else cmd, check=True, universal_newlines=True, stdout=stdout, stderr=sys.stderr, shell=shell)

base_url = os.environ['WEBDAV_URL']

curl_push = 'curl -s --user {}:{} -T {{}} {{}}'.format(os.environ['WEBDAV_USERNAME'], os.environ['WEBDAV_PASSWORD'])
curl_mv = 'curl -s --user {}:{} -X MOVE --header "Destination:{{}}" {{}}'.format(os.environ['WEBDAV_USERNAME'], os.environ['WEBDAV_PASSWORD'])
curl_mkdir = 'curl -s --user {}:{} -X MKCOL {{}}'.format(os.environ['WEBDAV_USERNAME'], os.environ['WEBDAV_PASSWORD'])
curl_rmdir = 'curl -s --user {}:{} -X DELETE {{}}'.format(os.environ['WEBDAV_USERNAME'], os.environ['WEBDAV_PASSWORD'])

git_log = 'git diff --name-status {}..{} -- {} | grep -E "^[A-Z]([0-9]{{3}})?" | sort | uniq'

print("Preparing to push the following commits")
print(run('git log {}..{}'.format(sys.argv[1], sys.argv[2]), stdout=subprocess.PIPE).stdout.splitlines())
print(git_log.format(sys.argv[1], sys.argv[2], sys.argv[3]))
log = run(git_log.format(sys.argv[1], sys.argv[2], sys.argv[3]), stdout=subprocess.PIPE, shell=True).stdout.splitlines()

dirnames = set()
pushes = set()
renames = set()
deletes = set()

for l in log:
    status = l[0]
    if status in "ACM":
        _, path = l.split()
        dirname = os.path.dirname(os.path.relpath(path, start=sys.argv[3]))
        while dirname and dirname != "/":
            dirnames.add(dirname)
            dirname = os.path.dirname(dirname)
        pushes.add(path)
    elif status == "R":
        _, old_path, new_path = l.split()
        dirnames.add(os.path.dirname(new_path))
        renames.add((old_path, new_path))
    elif status == "D":
        _, path = l.split()
        deletes.add(path)
    else:
        print("Unsupported git file status %s" % status)

print("Creating %d directories" % len(dirnames))
for d in sorted(dirnames):
    run(curl_mkdir.format(urljoin(base_url, d)))

print("Adding %d files" % len(pushes))
for f in pushes:
    run(curl_push.format(f, urljoin(base_url, os.path.relpath(f, start=sys.argv[3]))))

print("Moving %d files" % len(renames))
for old_path, new_path in renames:
    run(curl_mv.format(urljoin(base_url, old_path), urljoin(base_url, new_path)))

print("Deleting %d files" % len(deletes))
for f in deletes:
    run(curl_rmdir.format(urljoin(base_url, os.path.relpath(f, start=sys.argv[3]))))

print("Setting new remote commit ID")
run(curl_push.format('.commit_id', urljoin(base_url, '.commit_id')))
