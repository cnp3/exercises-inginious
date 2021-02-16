#!/usr/bin/env python3
import os
import sys
import subprocess
import traceback
import tempfile
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
username = os.environ['WEBDAV_USERNAME'].strip()
password = os.environ['WEBDAV_PASSWORD'].strip()

curl_push_config = """
user = "{}:{}"
upload-file = "{{}}"
url = "{{}}"

""".format(username, password)

curl_mv_config = """
user = "{}:{}"
request = "MOVE"
header = "Destination:{{}}"
url = "{{}}"

""".format(username, password)

curl_mkdir_config = """
user = "{}:{}"
request = "MKCOL"
url = "{{}}"

""".format(username, password)

curl_rmdir_config = """
user = "{}:{}"
request = "DELETE"
url = "{{}}"

""".format(username, password)

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

curl_config = ""

print("Creating %d directories" % len(dirnames))
for d in sorted(dirnames):
    curl_config += curl_mkdir_config.format(base_url, d)

print("Adding %d files" % len(pushes))
for f in pushes:
    curl_config += curl_push_config.format(f, urljoin(base_url, os.path.relpath(f, start=sys.argv[3])))

print("Moving %d files" % len(renames))
for old_path, new_path in renames:
    curl_config += curl_mv_config.format(urljoin(base_url, old_path), urljoin(base_url, new_path))

print("Deleting %d files" % len(deletes))
for f in deletes:
    curl_config += curl_rmdir_config.format(urljoin(base_url, os.path.relpath(f, start=sys.argv[3])))

print("Setting new remote commit ID")
curl_config += curl_push_config.format('.commit_id', urljoin(base_url, '.commit_id'))

config_filename = None
with tempfile.NamedTemporaryFile(mode='w',delete=False) as f:
    f.write(curl_config)
    config_filename = f.name

run('curl --fail-early -K {}'.format(config_filename))
