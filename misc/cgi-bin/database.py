#!/usr/bin/env python3
import cgi
import os
import cgitb

cgitb.enable()
print("Content-Type: text/html\n")

form = cgi.FieldStorage()
fileitem = form['uploaded_file']

os.write(2, b"abc123\n")

if fileitem.filename:
    # Save file to server
    filepath = os.path.join('/path/to/uploads', os.path.basename(fileitem.filename))
    with open(filepath, 'wb') as f:
        f.write(fileitem.file.read())
    print(f"File {fileitem.filename} uploaded successfully!")
else:
    print("No file uploaded.")

os.write(2, b"python script writing to stderr\n")
