from fastapi import FastAPI, Response, UploadFile, Form, File
from typing import Annotated
from fastapi.responses import FileResponse
from fastapi.staticfiles import StaticFiles
from contextlib import asynccontextmanager
import os
import json
import datetime
import binascii
import shutil

binary_path = "app.bin"

@asynccontextmanager
async def lifespan(app: FastAPI):
    try:
        with open("file.json", 'r') as file:
            app.file_info = json.load(file)
            if(~app.file_info):
                raise
    except:
        with open(binary_path, 'rb') as file:
            buf = file.read()
            app.file_info = {
                'file': binary_path,
                'crc32': binascii.crc32(buf),
                'length': os.path.getsize(binary_path),
                'force_upd': False
            }
    print("File info", app.file_info)
    yield
    with open("file.json", 'w') as file:
        json.dump(app.file_info, file)

app = FastAPI(lifespan=lifespan)

app.mount("/dashboard", StaticFiles(directory="page", html=True), name="page")

@app.head("/latest")
@app.get("/latest")
async def download():
    headers = {
        'X-CRC32': str(app.file_info['crc32']),
        'X-FORCEUPD': str(int(app.file_info['force_upd']))
    }
    return FileResponse(path=app.file_info['file'], headers=headers)

@app.post("/latest")
async def upload(file: Annotated[UploadFile, File()], force_upd: Annotated[bool, Form()]):
    with open(binary_path, 'wb') as dest_file:
        shutil.copyfileobj(file.file, dest_file)
    with open(binary_path, 'rb') as file:
        buf = file.read()
        app.file_info = {
            'file': binary_path,
            'crc32': binascii.crc32(buf),
            'length': os.path.getsize(binary_path),
            'force_upd': force_upd
        }
        print("Updated file:", app.file_info)