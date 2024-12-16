-----------------------------4148538522453418803633254034
Content-Disposition: form-data; name="fname"

x
-----------------------------4148538522453418803633254034
Content-Disposition: form-data; name="uploaded_file"; filename="database.py"
Content-Type: text/x-python

#!/usr/bin/env python3
import cgi
import os

print("Content-Type: text/html\n")

form = cgi.FieldStorage()
fileitem = form['uploaded_file']

if fileitem.filename:
    # Save file to server
    filepath = os.path.join('/path/to/uploads', os.path.basename(fileitem.filename))
    with open(filepath, 'wb') as f:
        f.write(fileitem.file.read())
    print(f"File {fileitem.filename} uploaded successfully!")
else:
    print("No file uploaded.")

-----------------------------4148538522453418803633254034--
