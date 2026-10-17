"""Import a server-side document into Office Lab's shared knowledge directory.
Run on cocoserver (or a development machine with office-lab kubectl access).
"""
import argparse, json, subprocess
from pathlib import Path
parser=argparse.ArgumentParser();parser.add_argument('file',type=Path);args=parser.parse_args()
if not args.file.is_file():parser.error('文件不存在')
if args.file.stat().st_size>20*1024*1024:parser.error('文件最大 20 MB')
code="import sys,json;from pathlib import Path;import file_open;print(json.dumps(file_open.upload(Path('/data'),sys.argv[1],sys.stdin.buffer.read()),ensure_ascii=False))"
subprocess.run(['kubectl','-n','office-lab','exec','-i','deploy/wopi','-c','wopi','--','python','-c',code,args.file.name],input=args.file.read_bytes(),check=True)
