"""Shared, persistent document catalogue. File identifiers never contain paths."""
import os, re, uuid, zipfile, io, shutil
from pathlib import Path

EXTENSIONS = {'.docx','.doc','.odt','.rtf','.txt','.xlsx','.xls','.ods','.pptx','.ppt','.odp'}
MAX_UPLOAD = 20 * 1024 * 1024
MAX_LIBRARY = int(os.environ.get('KNOWLEDGE_MAX_BYTES', str(512 * 1024 * 1024)))


def root(data):
    directory = Path(os.environ.get('KNOWLEDGE_DIR', str(data/'knowledge-base')))
    directory.mkdir(parents=True, exist_ok=True)
    return directory


def resolve(data, file_id):
    if file_id == '1':
        return data/'collaboration.docx'
    if not re.fullmatch(r'[0-9a-f]{32}', file_id):
        raise FileNotFoundError()
    directory = root(data)/file_id
    if directory.is_symlink() or not directory.is_dir():
        raise FileNotFoundError()
    files = [p for p in directory.iterdir() if not p.is_symlink() and p.is_file() and p.suffix.lower() in EXTENSIONS and not p.name.startswith('.')]
    if len(files) != 1:
        raise FileNotFoundError()
    return files[0]


def catalogue(data):
    records = []
    ids = ['1'] + [p.name for p in root(data).iterdir() if re.fullmatch(r'[0-9a-f]{32}', p.name)]
    for file_id in ids:
        try:
            path = resolve(data, file_id)
            stat = path.stat()
            records.append({'id':file_id, 'name':path.name, 'size':stat.st_size, 'modified':stat.st_mtime})
        except (FileNotFoundError, OSError):
            continue
    return sorted(records, key=lambda x:x['modified'], reverse=True)


def upload(data, name, content):
    if not name or len(name.encode('utf-8')) > 200 or any(c in name for c in '/\\\x00\r\n') or name.startswith('.'):
        raise ValueError('文件名无效')
    if Path(name).suffix.lower() not in EXTENSIONS:
        raise ValueError('暂不支持此文件类型')
    if not content or len(content) > MAX_UPLOAD:
        raise ValueError('请选择非空文件，最大 20 MB')
    if Path(name).suffix.lower() in {'.docx','.xlsx','.pptx','.odt','.ods','.odp'}:
        try:
            with zipfile.ZipFile(io.BytesIO(content)) as archive:
                if not {'[Content_Types].xml','mimetype'}.intersection(archive.namelist()):
                    raise ValueError('文档格式与扩展名不符')
        except zipfile.BadZipFile:
            raise ValueError('文档格式无效') from None
    records = catalogue(data)
    if len(records) >= 1000 or sum(x['size'] for x in records) + len(content) > MAX_LIBRARY or shutil.disk_usage(root(data)).free < len(content) + 32*1024*1024:
        raise ValueError('知识库空间不足，请联系管理员')
    file_id = uuid.uuid4().hex
    directory = root(data)/file_id
    directory.mkdir(mode=0o700)
    try:
        path = directory/name
        with path.open('xb') as stream:
            stream.write(content)
            stream.flush()
            os.fsync(stream.fileno())
    except Exception:
        shutil.rmtree(directory)
        raise
    return {'id':file_id, 'name':name, 'size':len(content)}
